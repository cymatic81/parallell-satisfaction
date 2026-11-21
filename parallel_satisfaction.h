/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: parallel_satisfaction.h   Author: Chapman     Created: 2026-11-21                                                     */
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README                             */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef PARALLEL_SATISFACTION_H
#define PARALLEL_SATISFACTION_H

/* 1. Standard C11 Base */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

/* 2. Support Functions & Core 2-Adic Evaluation Engine */
#include "Support_Functions/ps_functions.h"

/* 3. Verification Test Vectors (Optional: only loaded when building tests) */
#ifdef PS_ENABLE_VERIFY_HEADERS
  #include "Verify_Pipeline/verify_pipetest.h"
#endif

typedef unsigned __int128 uint128_t;

/* Prevent compiler inlining so every chipset invocation is preserved & measurable */
#if defined(__GNUC__) || defined(__clang__)
  #define PS_NOINLINE __attribute__((noinline))
#else
  #define PS_NOINLINE
#endif

/* X-Macro list of all Pseudo Chipset Operations & Logic Gates to track */
#define PS_CHIPSET_OP_LIST(X) \
    X(squareroot)             \
    X(squared)                \
    X(multiply)               \
    X(divide)                 \
    X(plus_1)                 \
    X(minus_1)                \
    X(and)                    \
    X(or)                     \
    X(xor)                    \
    X(not)

#ifdef PS_GATE_COUNT
  #include <stdatomic.h>

  #if defined(__GNUC__) || defined(__clang__)
    #define PS_WEAK_GLOBAL __attribute__((weak))
  #else
    #define PS_WEAK_GLOBAL
  #endif

  #define PS_DEF_COUNTER(name) PS_WEAK_GLOBAL atomic_uint_fast64_t g_calls_chipset_##name = 0;
  PS_CHIPSET_OP_LIST(PS_DEF_COUNTER)
  #undef PS_DEF_COUNTER

  #define PS_COUNT_OP(name) \
      atomic_fetch_add_explicit(&(g_calls_chipset_##name), 1, memory_order_relaxed)

  static inline void ps_gate_reset(void) {
      #define PS_RESET_COUNTER(name) \
          atomic_store_explicit(&(g_calls_chipset_##name), 0, memory_order_relaxed);
      PS_CHIPSET_OP_LIST(PS_RESET_COUNTER)
      #undef PS_RESET_COUNTER
  }

  static inline void ps_gate_dump(FILE *out, const char *label) {
      if (!out) out = stdout;
      fprintf(out, "=== Chipset Op Tally [%s] ===\n", label ? label : "default");
      #define PS_PRINT_COUNTER(name) \
          fprintf(out, "  chipset_%-12s : %llu\n", #name, \
                  (unsigned long long)atomic_load_explicit(&(g_calls_chipset_##name), memory_order_relaxed));
      PS_CHIPSET_OP_LIST(PS_PRINT_COUNTER)
      #undef PS_PRINT_COUNTER
  }
#else
  #define PS_COUNT_OP(name) ((void)0)
  static inline void ps_gate_reset(void) {}
  static inline void ps_gate_dump(FILE *out, const char *label) { (void)out; (void)label; }
#endif

/* Pseudo Chipset Operation Functions (pass pointer '*' if mutating the mask in-place) */
PS_NOINLINE void chipset_squareroot(uint128_t *squareroot_mask, uint8_t width, uint8_t calls);
PS_NOINLINE void chipset_squared   (uint128_t *squared_mask,    uint8_t width, uint8_t calls);
PS_NOINLINE void chipset_multiply  (uint128_t *multiply_mask,   uint8_t width, uint8_t calls);
PS_NOINLINE void chipset_divide    (uint128_t *divide_mask,     uint8_t width, uint8_t calls);
PS_NOINLINE void chipset_plus_1    (uint128_t *plus_1_mask,     uint8_t width, uint8_t calls);
PS_NOINLINE void chipset_minus_1   (uint128_t *minus_1_mask,    uint8_t width, uint8_t calls);

/* Pseudo Logic Gates for readability and Count */
static inline uint128_t $AND$(uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(and); return reg1 & reg2; }
static inline uint128_t $OR$ (uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(or);  return reg1 | reg2; }
static inline uint128_t $XOR$(uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(xor); return reg1 ^ reg2; }
static inline uint128_t $NOT$(uint128_t reg1)                 { PS_COUNT_OP(not); return ~reg1; }

static inline uint128_t ps_width_mask(uint8_t width) {
    if (width >= 128) return ~(uint128_t)0;
    if (width == 0)   return 0;
    return ((uint128_t)1 << width) - 1;
}

static inline uint128_t ps_gate_add(uint128_t a, uint128_t b) {
    while (b != 0) {
        uint128_t carry = $AND$(a, b) << 1;
        a = $XOR$(a, b);
        b = carry;
    }
    return a;
}

static inline uint128_t ps_gate_sub(uint128_t a, uint128_t b) {
    return ps_gate_add(a, ps_gate_add($NOT$(b), 1));
}


/* Pseudo Minimum Chipset Instructions for logic gate count 

PS_NOINLINE void ymm_and_rax();

PS_NOINLINE void ymm_xor_rax();

RAX ^ lane

RAX & lane

RBX ^ lane

RBX & lane

YMM ^ RAX

YMM & RAX

YMM inc/dec lane

YMM bit shift (bridge the carry)
*/

#endif // PARALLEL_SATISFACTION_H
