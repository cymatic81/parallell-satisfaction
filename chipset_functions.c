/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: chipset_functions.c		Author: Chapman		Created: 2026-11-21																		*/
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README	  							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "parallel_satisfaction.h"

PS_NOINLINE void chipset_squareroot(uint128_t *squareroot_mask, uint8_t width) {
    PS_COUNT_OP(op_squareroot);
    uint128_t wmask = ps_width_mask(width);
    uint128_t val   = $AND$($LOAD$(*squareroot_mask), wmask);

    /* Start highest even power-of-two bit within 'width' (avoids scanning dead upper bits) */
    uint8_t top_even = (width >= 2) ? (uint8_t)((width - 1) & ~1U) : 0;
    uint128_t res    = 0;
    uint128_t bit    = (width > 0) ? ($SHFX$((uint128_t)1) << top_even) : 0;

    while (bit > val) {
        bit = $SHF1$($SHF1$(bit)) >> 2;
    }

    while (bit != 0) {
        uint128_t trial = $ADD$(res, bit);
        if (val >= trial) {
            val = $SUB$(val, trial);
            res = $ADD$($SHF1$(res) >> 1, bit);
        } else {
            res = $SHF1$(res) >> 1;
        }
        bit = $SHF1$($SHF1$(bit)) >> 2;
    }

    *squareroot_mask = $SEND$($AND$(res, wmask));
}

PS_NOINLINE void chipset_squared(uint128_t *squared_mask, uint8_t width) {
    PS_COUNT_OP(op_squared);
    uint128_t wmask = ps_width_mask(width);
    uint128_t val   = $AND$($LOAD$(*squared_mask), wmask);

    uint128_t acc = 0;
    uint128_t a   = val;
    uint128_t b   = val;

    while (b != 0) {
        if ($AND$(b, 1)) {
            acc = $ADD$(acc, a);
        }
        a = $SHF1$(a) << 1;
        b = $SHF1$(b) >> 1;
    }

    *squared_mask = $SEND$($AND$(acc, wmask));
}

PS_NOINLINE void chipset_multiply(uint128_t *multiply_mask, uint8_t width) {
    PS_COUNT_OP(op_multiply);
    uint8_t   lane_w = (width > 64) ? 64 : width;
    uint128_t wmask  = ps_width_mask(lane_w);
    uint128_t raw    = $LOAD$(*multiply_mask);

    /* Unpack Operand A (low lane) and Operand B (high lane) */
    uint128_t a = $AND$(raw, wmask);
    uint128_t b = $AND$($SHFX$(raw) >> lane_w, wmask);

    uint128_t acc = 0;
    while (b != 0) {
        if ($AND$(b, 1)) {
            acc = $ADD$(acc, a);
        }
        a = $SHF1$(a) << 1;
        b = $SHF1$(b) >> 1;
    }

    *multiply_mask = $SEND$($AND$(acc, wmask));
}

PS_NOINLINE void chipset_divide(uint128_t *divide_mask, uint8_t width) {
    PS_COUNT_OP(op_divide);
    uint8_t   lane_w = (width > 64) ? 64 : width;
    uint128_t wmask  = ps_width_mask(lane_w);
    uint128_t raw    = $LOAD$(*divide_mask);

    /* Unpack Dividend A (low lane) and Divisor B (high lane) */
    uint128_t num = $AND$(raw, wmask);
    uint128_t den = $AND$($SHFX$(raw) >> lane_w, wmask);

    if (den == 0) {
        *divide_mask = $SEND$(0);
        return;
    }

    uint128_t quot = 0;
    while (num >= den) {
        num  = $SUB$(num, den);
        quot = $INC$(quot);
    }

    *divide_mask = $SEND$($AND$(quot, wmask));
}

PS_NOINLINE void chipset_add(uint128_t *add_mask, uint8_t width) {
    PS_COUNT_OP(op_add);
    uint8_t   lane_w = (width > 64) ? 64 : width;
    uint128_t wmask  = ps_width_mask(lane_w);
    uint128_t raw    = $LOAD$(*add_mask);

    /* Unpack Operand A (low lane) and Operand B (high lane) */
    uint128_t a = $AND$(raw, wmask);
    uint128_t b = $AND$($SHFX$(raw) >> lane_w, wmask);

    uint128_t sum = $ADD$(a, b);
    *add_mask = $SEND$($AND$(sum, wmask));
}

PS_NOINLINE void chipset_sub(uint128_t *sub_mask, uint8_t width) {
    PS_COUNT_OP(op_sub);
    uint8_t   lane_w = (width > 64) ? 64 : width;
    uint128_t wmask  = ps_width_mask(lane_w);
    uint128_t raw    = $LOAD$(*sub_mask);

    /* Unpack Operand A (low lane) and Operand B (high lane) */
    uint128_t a = $AND$(raw, wmask);
    uint128_t b = $AND$($SHFX$(raw) >> lane_w, wmask);

    /* Two's-complement subtraction: A + (~B + 1) masked to lane width */
    uint128_t inv_b = $AND$($NOT$(b), wmask);
    uint128_t neg_b = $AND$($INC$(inv_b), wmask);
    uint128_t diff  = $ADD$(a, neg_b);

    *sub_mask = $SEND$($AND$(diff, wmask));
}

PS_NOINLINE void chipset_shfx(uint128_t *shfx_mask, uint8_t width, uint8_t bit_offset) {
    PS_COUNT_OP(op_shfx);
    uint128_t wmask     = ps_width_mask(width);
    uint128_t val       = $AND$($LOAD$(*shfx_mask), wmask);
    uint8_t   dir_right = (bit_offset & 0x80U) != 0; /* Bit 7: 0 = left (<<), 1 = right (>>) */
    uint8_t   shift_amt = (uint8_t)(bit_offset & 0x7FU);

    if (shift_amt == 0 || shift_amt >= width) {
        *shfx_mask = $SEND$((shift_amt == 0) ? val : 0);
        return;
    }

    if (shift_amt == 1) {
        val = dir_right ? ($SHF1$(val) >> 1) : ($SHF1$(val) << 1);
    } else if ((shift_amt & 7U) == 0) {
        /* Byte-aligned shift (multiple of 8 bits) */
        val = dir_right ? ($SHFX8$(val) >> shift_amt) : ($SHFX8$(val) << shift_amt);
    } else {
        /* Arbitrary bit-offset shift */
        val = dir_right ? ($SHFX$(val) >> shift_amt) : ($SHFX$(val) << shift_amt);
    }

    *shfx_mask = $SEND$($AND$(val, wmask));
}
