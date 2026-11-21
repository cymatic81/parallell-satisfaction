/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: chipset_functions.c		Author: Chapman		Created: 2026-11-21																		*/
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README	  							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "parallel_satisfaction.h"

PS_NOINLINE void chipset_squareroot(uint128_t *squareroot_mask, uint8_t width, uint8_t calls) {
    PS_COUNT_OP(squareroot);
    uint128_t wmask = ps_width_mask(width);
    uint128_t val   = $AND$(*squareroot_mask, wmask);

    for (uint8_t c = 0; c < calls; ++c) {
        uint128_t res = 0;
        uint128_t bit = (uint128_t)1 << 126;
        while (bit > val) bit >>= 2;

        while (bit != 0) {
            uint128_t trial = ps_gate_add(res, bit);
            if (val >= trial) {
                val = ps_gate_sub(val, trial);
                res = ps_gate_add(res >> 1, bit);
            } else {
                res >>= 1;
            }
            bit >>= 2;
        }
        val = $AND$(res, wmask);
    }
    *squareroot_mask = val;
}

PS_NOINLINE void chipset_squared(uint128_t *squared_mask, uint8_t width, uint8_t calls) {
    PS_COUNT_OP(squared);
    uint128_t wmask = ps_width_mask(width);
    uint128_t val   = $AND$(*squared_mask, wmask);

    for (uint8_t c = 0; c < calls; ++c) {
        uint128_t acc = 0;
        uint128_t a   = val;
        uint128_t b   = val;
        while (b != 0) {
            if ($AND$(b, 1)) acc = ps_gate_add(acc, a);
            a <<= 1;
            b >>= 1;
        }
        val = $AND$(acc, wmask);
    }
    *squared_mask = val;
}

PS_NOINLINE void chipset_multiply(uint128_t *multiply_mask, uint8_t width, uint8_t calls) {
    PS_COUNT_OP(multiply);
    uint128_t wmask = ps_width_mask(width);
    uint128_t val   = $AND$(*multiply_mask, wmask);

    uint128_t acc = 0;
    uint128_t m   = calls;
    while (m != 0) {
        if ($AND$(m, 1)) acc = ps_gate_add(acc, val);
        val <<= 1;
        m   >>= 1;
    }
    *multiply_mask = $AND$(acc, wmask);
}

PS_NOINLINE void chipset_divide(uint128_t *divide_mask, uint8_t width, uint8_t calls) {
    PS_COUNT_OP(divide);
    if (calls == 0) return;
    uint128_t wmask = ps_width_mask(width);
    uint128_t num   = $AND$(*divide_mask, wmask);
    uint128_t quot  = 0;

    while (num >= calls) {
        num  = ps_gate_sub(num, calls);
        quot = ps_gate_add(quot, 1);
    }
    *divide_mask = $AND$(quot, wmask);
}

PS_NOINLINE void chipset_plus_1(uint128_t *plus_1_mask, uint8_t width, uint8_t calls) {
    PS_COUNT_OP(plus_1);
    uint128_t wmask = ps_width_mask(width);
    uint128_t val   = $AND$(*plus_1_mask, wmask);

    for (uint8_t i = 0; i < calls; ++i) {
        val = $AND$(ps_gate_add(val, 1), wmask);
    }
    *plus_1_mask = val;
}

PS_NOINLINE void chipset_minus_1(uint128_t *minus_1_mask, uint8_t width, uint8_t calls) {
    PS_COUNT_OP(minus_1);
    uint128_t wmask = ps_width_mask(width);
    uint128_t val   = $AND$(*minus_1_mask, wmask);

    for (uint8_t i = 0; i < calls; ++i) {
        val = $AND$(ps_gate_sub(val, 1), wmask);
    }
    *minus_1_mask = val;
}
