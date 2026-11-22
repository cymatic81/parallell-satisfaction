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
    X(op_squareroot)          \
    X(op_squared)             \
    X(op_multiply)            \
    X(op_divide)              \
    X(op_add)                 \
    X(op_sub)                 \
    X(op_shfx)                \
    X(and)                    \
    X(or)                     \
    X(xor)                    \
    X(not)                    \
    X(add)                    \
    X(sub)                    \
    X(inc)                    \
    X(dec)                    \
    X(shf1)                   \
    X(shfx)                   \
    X(shfx8)                  \
    X(mov)                    \
    X(mov_v)                  \
    X(mov_r)                  \
    X(mov_vv)                 \
    X(load)                   \
    X(send)

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
PS_NOINLINE void chipset_squareroot(uint128_t *squareroot_mask, uint8_t width);
PS_NOINLINE void chipset_squared   (uint128_t *squared_mask,    uint8_t width);
PS_NOINLINE void chipset_multiply  (uint128_t *multiply_mask,   uint8_t width);
PS_NOINLINE void chipset_divide    (uint128_t *divide_mask,     uint8_t width);
PS_NOINLINE void chipset_add       (uint128_t *add_mask,        uint8_t width);
PS_NOINLINE void chipset_sub       (uint128_t *sub_mask,        uint8_t width);
PS_NOINLINE void chipset_shfx      (uint128_t *shfx_mask,       uint8_t width, uint8_t bit_offset);

/* Pseudo Logic Gates for readability and Count */
static inline uint128_t $AND$   (uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(and); return reg1 & reg2; }
static inline uint128_t $OR$    (uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(or);  return reg1 | reg2; }
static inline uint128_t $XOR$   (uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(xor); return reg1 ^ reg2; }
static inline uint128_t $NOT$   (uint128_t reg1)                 { PS_COUNT_OP(not); return ~reg1; }
static inline uint128_t $ADD$   (uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(add); return reg1 + reg2; } // counts add iterative gate stack as 1
static inline uint128_t $SUB$   (uint128_t reg1, uint128_t reg2) { PS_COUNT_OP(sub); return reg1 - reg2; } // counts sub iterative gate stack as 1
static inline uint128_t $INC$   (uint128_t reg1)  { PS_COUNT_OP(inc);   return reg1 + 1; }
static inline uint128_t $DEC$   (uint128_t reg1)  { PS_COUNT_OP(dec);   return reg1 - 1; }
static inline uint128_t $SHF1$  (uint128_t reg1)  { PS_COUNT_OP(shf1);  return reg1; }
static inline uint128_t $SHFX$  (uint128_t reg1)  { PS_COUNT_OP(shfx);  return reg1; }
static inline uint128_t $SHFX8$ (uint128_t reg1)  { PS_COUNT_OP(shfx8); return reg1; }
static inline uint128_t $MOV$   (uint128_t reg1)  { PS_COUNT_OP(mov);   return reg1; }
static inline uint128_t $MOV_V$ (uint128_t reg1)  { PS_COUNT_OP(mov_v); return reg1; } // move RAX to YMM
static inline uint128_t $MOV_r$ (uint128_t reg1)  { PS_COUNT_OP(mov_r); return reg1; } // move YMM to RAX
static inline uint128_t $MOV_VV$(uint128_t vec1)  { PS_COUNT_OP(mov_vv);return vec1; } // move YMM to YMM
static inline uint128_t $LOAD$  (uint128_t mem1)  { PS_COUNT_OP(load);  return mem1; } // likely redundant
static inline uint128_t $SEND$  (uint128_t reg1)  { PS_COUNT_OP(send);  return reg1; } // likely redundant


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
