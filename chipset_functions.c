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



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* SIMD / Vector Layer Implementations                                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

PS_NOINLINE void simd_broadcast_rax(uint128_t *vec_mask, uint64_t rax, uint8_t lane_width) {
    PS_COUNT_OP(simd_bcast);
    if (lane_width == 0 || lane_width > 128) return;

    uint128_t lmask     = ps_width_mask(lane_width);
    uint128_t lane_val  = $AND$($MOV_V$((uint128_t)rax), lmask);
    uint128_t out_vec   = 0;
    uint8_t   num_lanes = (uint8_t)(128U / lane_width);

    for (uint8_t i = 0; i < num_lanes; ++i) {
        uint8_t   shift   = (uint8_t)(i * lane_width);
        uint128_t shifted = (shift == 0) ? lane_val
                          : ((shift & 7U) == 0) ? ($SHFX8$(lane_val) << shift)
                                                : ($SHFX$(lane_val) << shift);
        out_vec = $OR$(out_vec, shifted);
    }
    *vec_mask = $SEND$(out_vec); }

PS_NOINLINE void simd_insert_lane(uint128_t *vec_mask, uint64_t rax, uint8_t lane_idx, uint8_t lane_w) {
    if (lane_w == 0 || lane_w > 128) return;
    uint8_t max_lanes = (uint8_t)(128U / lane_w);
    if (lane_idx >= max_lanes) return;

    uint128_t vec       = $LOAD$(*vec_mask);
    uint128_t lmask     = ps_width_mask(lane_w);
    uint128_t lane_val  = $AND$($MOV_V$((uint128_t)rax), lmask);
    uint8_t   shift     = (uint8_t)(lane_idx * lane_w);

    uint128_t slot_mask = (shift == 0) ? lmask
                        : ((shift & 7U) == 0) ? ($SHFX8$(lmask) << shift)
                                              : ($SHFX$(lmask) << shift);
    uint128_t ins_bits  = (shift == 0) ? lane_val
                        : ((shift & 7U) == 0) ? ($SHFX8$(lane_val) << shift)
                                              : ($SHFX$(lane_val) << shift);
    vec = $OR$($AND$(vec, $NOT$(slot_mask)), ins_bits);
    *vec_mask = $SEND$(vec); }

PS_NOINLINE void simd_extract_lane(const uint128_t *vec_mask, uint64_t *rax_out, uint8_t lane_idx, uint8_t lane_w) {
    PS_COUNT_OP(simd_extract);
    if (lane_w == 0 || lane_w > 64) return;
    uint8_t max_lanes = (uint8_t)(128U / lane_w);
    if (lane_idx >= max_lanes) return;

    uint128_t vec     = $LOAD$(*vec_mask);
    uint128_t lmask   = ps_width_mask(lane_w);
    uint8_t   shift   = (uint8_t)(lane_idx * lane_w);
    uint128_t shifted = (shift == 0) ? vec
                      : ((shift & 7U) == 0) ? ($SHFX8$(vec) >> shift)
                                            : ($SHFX$(vec) >> shift);
    uint128_t lane    = $MOV_r$($AND$(shifted, lmask));
    *rax_out = (uint64_t)$SEND$(lane); }

PS_NOINLINE void simd_permute_lanes(uint128_t *vec_mask, uint8_t perm_mask, uint8_t lane_w) {
    PS_COUNT_OP(simd_permute);
    if (lane_w == 0 || lane_w > 64) return;
    uint8_t num_lanes = (uint8_t)(128U / lane_w);
    if (num_lanes > 4) num_lanes = 4; /* 2-bit selector per lane in 8-bit perm_mask */

    uint128_t src   = $MOV_VV$($LOAD$(*vec_mask));
    uint128_t lmask = ps_width_mask(lane_w);
    uint128_t out   = 0;

    for (uint8_t dst_i = 0; dst_i < num_lanes; ++dst_i) {
        uint8_t   src_i   = (uint8_t)((perm_mask >> (dst_i * 2U)) & 0x03U) % num_lanes;
        uint8_t   r_shift = (uint8_t)(src_i * lane_w);
        uint8_t   l_shift = (uint8_t)(dst_i * lane_w);

        uint128_t extracted = (r_shift == 0) ? src
                            : ((r_shift & 7U) == 0) ? ($SHFX8$(src) >> r_shift)
                                                    : ($SHFX$(src) >> r_shift);
        extracted = $AND$(extracted, lmask);
        uint128_t placed    = (l_shift == 0) ? extracted
                            : ((l_shift & 7U) == 0) ? ($SHFX8$(extracted) << l_shift)
                                                    : ($SHFX$(extracted) << l_shift);
        out = $OR$(out, placed);
    }
    *vec_mask = $SEND$(out); }

PS_NOINLINE void simd_ymm_and_rax(uint128_t *vec_mask, uint64_t rax, uint8_t lane_width) {
    PS_COUNT_OP(simd_and_rax);
    if (lane_width == 0 || lane_width > 128) return;

    uint128_t vec       = $LOAD$(*vec_mask);
    uint128_t lmask     = ps_width_mask(lane_width);
    uint128_t lane_val  = $AND$($MOV_V$((uint128_t)rax), lmask);
    uint128_t bcast     = 0;
    uint8_t   num_lanes = (uint8_t)(128U / lane_width);

    for (uint8_t i = 0; i < num_lanes; ++i) {
        uint8_t   shift   = (uint8_t)(i * lane_width);
        uint128_t shifted = (shift == 0) ? lane_val
                          : ((shift & 7U) == 0) ? ($SHFX8$(lane_val) << shift)
                                                : ($SHFX$(lane_val) << shift);
        bcast = $OR$(bcast, shifted);
    }
    *vec_mask = $SEND$($AND$(vec, bcast)); }

PS_NOINLINE void simd_ymm_xor_rax(uint128_t *vec_mask, uint64_t rax, uint8_t lane_width) {
    PS_COUNT_OP(simd_xor_rax);
    if (lane_width == 0 || lane_width > 128) return;

    uint128_t vec       = $LOAD$(*vec_mask);
    uint128_t lmask     = ps_width_mask(lane_width);
    uint128_t lane_val  = $AND$($MOV_V$((uint128_t)rax), lmask);
    uint128_t bcast     = 0;
    uint8_t   num_lanes = (uint8_t)(128U / lane_width);

    for (uint8_t i = 0; i < num_lanes; ++i) {
        uint8_t   shift   = (uint8_t)(i * lane_width);
        uint128_t shifted = (shift == 0) ? lane_val
                          : ((shift & 7U) == 0) ? ($SHFX8$(lane_val) << shift)
                                                : ($SHFX$(lane_val) << shift);
        bcast = $OR$(bcast, shifted);
    }
    *vec_mask = $SEND$($XOR$(vec, bcast)); }

PS_NOINLINE void simd_carry_bridge(uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count) {
    PS_COUNT_OP(simd_carry_bridge);
    if (lane_width == 0 || lane_count < 2 || (uint16_t)lane_width * lane_count > 128U) return;

    uint128_t vec   = $LOAD$(*vec_mask);
    uint128_t lmask = ps_width_mask(lane_width);

    /* Bridge MSB (bit lane_width - 1) of lane i into bit 0 of lane i + 1 */
    for (uint8_t i = 0; i + 1 < lane_count; ++i) {
        uint8_t   src_shift = (uint8_t)(i * lane_width);
        uint8_t   dst_shift = (uint8_t)((i + 1U) * lane_width);

        uint128_t lane_i    = (src_shift == 0) ? vec
                            : ((src_shift & 7U) == 0) ? ($SHFX8$(vec) >> src_shift)
                                                      : ($SHFX$(vec) >> src_shift);
        lane_i = $AND$(lane_i, lmask);
        uint128_t msb_bit   = (lane_width > 1) ? $AND$($SHFX$(lane_i) >> (lane_width - 1U), 1)
                                               : $AND$(lane_i, 1);

        if (msb_bit != 0) {
            uint128_t dst_lane = (dst_shift & 7U) == 0 ? ($SHFX8$(vec) >> dst_shift)
                                                       : ($SHFX$(vec) >> dst_shift);
            dst_lane = $AND$($ADD$($AND$(dst_lane, lmask), msb_bit), lmask);
            uint128_t slot_mask = (dst_shift & 7U) == 0 ? ($SHFX8$(lmask) << dst_shift)
                                                        : ($SHFX$(lmask) << dst_shift);
            uint128_t placed    = (dst_shift & 7U) == 0 ? ($SHFX8$(dst_lane) << dst_shift)
                                                        : ($SHFX$(dst_lane) << dst_shift);
            vec = $OR$($AND$(vec, $NOT$(slot_mask)), placed);
        }
    }
    *vec_mask = $SEND$(vec); }

PS_NOINLINE void simd_lane_inc(uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count) {
    if (lane_width == 0 || (uint16_t)lane_width * lane_count > 128U) return;

    uint128_t vec   = $LOAD$(*vec_mask);
    uint128_t lmask = ps_width_mask(lane_width);
    uint128_t out   = 0;

    for (uint8_t i = 0; i < lane_count; ++i) {
        uint8_t   shift = (uint8_t)(i * lane_width);
        uint128_t lane  = (shift == 0) ? vec
                        : ((shift & 7U) == 0) ? ($SHFX8$(vec) >> shift)
                                              : ($SHFX$(vec) >> shift);
        lane = $AND$($INC$($AND$(lane, lmask)), lmask);
        uint128_t placed = (shift == 0) ? lane
                         : ((shift & 7U) == 0) ? ($SHFX8$(lane) << shift)
                                               : ($SHFX$(lane) << shift);
        out = $OR$(out, placed);
    }
    *vec_mask = $SEND$(out); }

PS_NOINLINE void simd_lane_dec(uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count) {
    if (lane_width == 0 || (uint16_t)lane_width * lane_count > 128U) return;

    uint128_t vec   = $LOAD$(*vec_mask);
    uint128_t lmask = ps_width_mask(lane_width);
    uint128_t out   = 0;

    for (uint8_t i = 0; i < lane_count; ++i) {
        uint8_t   shift = (uint8_t)(i * lane_width);
        uint128_t lane  = (shift == 0) ? vec
                        : ((shift & 7U) == 0) ? ($SHFX8$(vec) >> shift)
                                              : ($SHFX$(vec) >> shift);
        lane = $AND$($DEC$($AND$(lane, lmask)), lmask);
        uint128_t placed = (shift == 0) ? lane
                         : ((shift & 7U) == 0) ? ($SHFX8$(lane) << shift)
                                               : ($SHFX$(lane) << shift);
        out = $OR$(out, placed);
    }
    *vec_mask = $SEND$(out); }

PS_NOINLINE void simd_lane_pop1(uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count) {
    if (lane_width == 0 || (uint16_t)lane_width * lane_count > 128U) return;

    uint128_t vec   = $LOAD$(*vec_mask);
    uint128_t lmask = ps_width_mask(lane_width);
    uint128_t out   = 0;

    for (uint8_t i = 0; i < lane_count; ++i) {
        uint8_t   shift = (uint8_t)(i * lane_width);
        uint128_t lane  = (shift == 0) ? vec
                        : ((shift & 7U) == 0) ? ($SHFX8$(vec) >> shift)
                                              : ($SHFX$(vec) >> shift);
        uint128_t ones  = $AND$($POP1$(lane, lane_width), lmask);
        uint128_t placed = (shift == 0) ? ones
: ((shift & 7U) == 0) ? ($SHFX8$(ones) << shift)
: ($SHFX$(ones) << shift);
out = $OR$(out, placed);
}
*vec_mask = $SEND$(out); }

PS_NOINLINE void simd_lane_pop0(uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count) {
if (lane_width == 0 || (uint16_t)lane_width * lane_count > 128U) return;
uint128_t vec   = $LOAD$(*vec_mask);
uint128_t lmask = ps_width_mask(lane_width);
uint128_t out   = 0;
for (uint8_t i = 0; i < lane_count; ++i) {
uint8_t   shift = (uint8_t)(i * lane_width);
uint128_t lane  = (shift == 0) ? vec
: ((shift & 7U) == 0) ? ($SHFX8$(vec) >> shift)
: ($SHFX$(vec) >> shift);
uint128_t zeros = $AND$($POP0$(lane, lane_width), lmask);
uint128_t placed = (shift == 0) ? zeros
: ((shift & 7U) == 0) ? ($SHFX8$(zeros) << shift)
: ($SHFX$(zeros) << shift);
out = $OR$(out, placed);
}
*vec_mask = $SEND$(out); }