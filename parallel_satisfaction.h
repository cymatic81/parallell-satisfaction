/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: parallel_satisfaction.h   Author: Chapman     Created: 2026-11-21                                                     */
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README                             */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef PARALLEL_SATISFACTION_H
#define PARALLEL_SATISFACTION_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

#include "Support_Functions/ps_functions.h"

#ifndef PS_UINT128_DEFINED
#define PS_UINT128_DEFINED
typedef unsigned __int128 uint128_t; // typedef for casting 
#endif

#ifdef PS_ENABLE_VERIFY_HEADERS
  #include "Verify_Pipeline/verify_pipetest.h"
#endif

/* Prevent compiler inlining so every chipset invocation is preserved & measurable */
#if defined(__GNUC__) || defined(__clang__)
  #define PS_NOINLINE __attribute__((noinline))
#else
  #define PS_NOINLINE
#endif

/* X-Macro list of all Pseudo Chipset Operations & Logic Gates to track */
#define PS_CHIPSET_OP_LIST(X) \
    /* op */ \
    X(op_squareroot) X(op_squared) X(op_multiply) X(op_divide) X(op_add) X(op_sub) X(op_shfx) \
    /* simd */ \
    X(simd_bcast) X(simd_extract) X(simd_permute) X(simd_and_rax) X(simd_xor_rax) X(simd_carry_bridge) \
    /* gate */ \
    X(and) X(or) X(xor) X(not) X(add) X(sub) X(inc) X(dec) X(shf1) X(shfx) X(shfx8) X(pop1) X(pop0) \
    /* mov */ \
    X(mov) X(mov_v) X(mov_r) X(mov_vv) X(load) X(send)

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

/* Pseudo C level opertions containing chipset instructions(pass pointer '*' if mutating the mask in-place) */
PS_NOINLINE void chipset_squareroot(uint128_t *squareroot_mask, uint8_t width);
PS_NOINLINE void chipset_squared   (uint128_t *squared_mask,    uint8_t width);
PS_NOINLINE void chipset_multiply  (uint128_t *multiply_mask,   uint8_t width);
PS_NOINLINE void chipset_divide    (uint128_t *divide_mask,     uint8_t width);
PS_NOINLINE void chipset_add       (uint128_t *add_mask,        uint8_t width);
PS_NOINLINE void chipset_sub       (uint128_t *sub_mask,        uint8_t width);
PS_NOINLINE void chipset_shfx      (uint128_t *shfx_mask,       uint8_t width, uint8_t bit_offset);
//chipset pop needed?

/* Pseudo Data transfer Chipset instructions */
//un-needed layer??? 

/* Pseudo SIMD / Vector Layer Chipset Instructions (pass pointer '*' if mutating the vector/mask in-place) */
PS_NOINLINE void simd_broadcast_rax (uint128_t *vec_mask, uint64_t rax, uint8_t lane_width);                          // VPBROADCASTQ / VPBROADCASTD (broadcast RAX across all lanes)
PS_NOINLINE void simd_insert_lane   (uint128_t *vec_mask, uint64_t rax, uint8_t lane_idx, uint8_t lane_w);            // VPINSRQ / VPINSRD (RAX -> YMM lane via $MOV_V$)
PS_NOINLINE void simd_extract_lane  (const uint128_t *vec_mask, uint64_t *rax_out, uint8_t lane_idx, uint8_t lane_w); // VPEXTRQ / VMOVQ (YMM lane -> RAX via $MOV_r$)
PS_NOINLINE void simd_permute_lanes (uint128_t *vec_mask, uint8_t perm_mask, uint8_t lane_w);                         // VPERMQ / VPSHUFD (YMM <-> YMM lane reorder via $MOV_VV$)
PS_NOINLINE void simd_ymm_and_rax   (uint128_t *vec_mask, uint64_t rax, uint8_t lane_width);                          // VPAND (YMM & RAX across all packed lanes)
PS_NOINLINE void simd_ymm_xor_rax   (uint128_t *vec_mask, uint64_t rax, uint8_t lane_width);                          // VPXOR (YMM ^ RAX across all packed lanes)
PS_NOINLINE void simd_carry_bridge  (uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count);                    // VPSLLQ / VPOR (bridge carry/overflow bit from lane i to lane i+1)
PS_NOINLINE void simd_lane_inc      (uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count);                    // VPADDQ / VPADDD (parallel +1 increment across all lanes)
PS_NOINLINE void simd_lane_dec      (uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count);                    // VPSUBQ / VPSUBD (parallel -1 decrement across all lanes)
PS_NOINLINE void simd_lane_pop1     (uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count);                    // VPOPCNTDQ (count total '1' bits per lane via $POP1$)
PS_NOINLINE void simd_lane_pop0     (uint128_t *vec_mask, uint8_t lane_width, uint8_t lane_count);                    // VPOPCNTDQ + VPANDN (count total '0' bits per lane via $POP0$)

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

/* Bit-Width Masking Helper (clamps bus width 0..128 bits) */
static inline uint128_t ps_width_mask(uint8_t width) {
    if (width >= 128) return ~(uint128_t)0;
    if (width == 0)   return 0;
    return ((uint128_t)1 << width) - 1; }
/* Pseudo Logic Gates (continued) */
static inline uint128_t $POP1$(uint128_t reg1, uint8_t width) {   // can i tighten these two inlines up?         
    PS_COUNT_OP(pop1);
    uint128_t masked = reg1 & ps_width_mask(width);
    uint64_t  lo     = (uint64_t)masked;
    uint64_t  hi     = (uint64_t)(masked >> 64);
    return (uint128_t)(__builtin_popcountll(lo) + __builtin_popcountll(hi)); }
static inline uint128_t $POP0$(uint128_t reg1, uint8_t width) {   // can i tighten these two inlines up?   
    PS_COUNT_OP(pop0);
    if (width > 128) width = 128;
    uint128_t masked = reg1 & ps_width_mask(width);
    uint64_t  lo     = (uint64_t)masked;
    uint64_t  hi     = (uint64_t)(masked >> 64);
    uint8_t   ones   = (uint8_t)(__builtin_popcountll(lo) + __builtin_popcountll(hi));
    return (uint128_t)(width - ones); }

#endif // PARALLEL_SATISFACTION_H
