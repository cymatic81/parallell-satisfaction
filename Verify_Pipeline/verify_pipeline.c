/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: verify_pipeline.c		Author: Chapman		Created: 2026-11-21																		*/
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README	  							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define PS_ENABLE_VERIFY_HEADERS
#include "../parallel_satisfaction.h"

int main(void) {
    printf("=== 32-BIT VECTOR MASKS (UPPERCASE) ===\n");
    for (size_t i = 0; i < TEST_VECTOR_COUNT; i++) {
        printf("%s : %s\n",
               TEST_VECTORS_32[i].name,
               binary_tail_hexa(TEST_VECTORS_32[i].mask, 1));
    }

    printf("\n=== STANDARD UINT CASTS (LOWERCASE) ===\n");
    printf("uint64_t    : %s\n", binary_tail_hexa(VEC_DEAD_CAFE_64, 0));
    printf("uint32_t    : %s\n", binary_tail_hexa((uint32_t)VEC_DEADBEEF, 0));
    printf("uint16_t    : %s\n", binary_tail_hexa((uint16_t)VEC_CAFEBABE, 0));
    printf("uint8_t     : %s\n", binary_tail_hexa((uint8_t)VEC_DEADBEEF, 0));

    printf("\n=== ARBITRARY BIT-LENGTH TRUNCATIONS (DEADBEEF / CAFEBABE) ===\n");
    for (size_t i = 0; i < TEST_TRUNC_COUNT; i++) {
        printf("%s : %s\n",
               TEST_TRUNC_VECTORS[i].label,
               binary_tail_hexa_bits(TEST_TRUNC_VECTORS[i].val,
                                     TEST_TRUNC_VECTORS[i].bits,
                                     TEST_TRUNC_VECTORS[i].upper));
    }

    printf("\n=== XPLANE_2ADIC_EVAL_SAFE: ALL 2-INPUT COMBINATIONS (32-BIT & TRUNCATED) ===\n");
    size_t total_inputs = TEST_VECTOR_COUNT + TEST_TRUNC_COUNT;

    for (size_t i = 0; i < total_inputs; i++) {
        const char *name1 = (i < TEST_VECTOR_COUNT)
                            ? TEST_VECTORS_32[i].name
                            : TEST_TRUNC_VECTORS[i - TEST_VECTOR_COUNT].label;
        uint64_t in1 = (i < TEST_VECTOR_COUNT)
                       ? (uint64_t)TEST_VECTORS_32[i].mask
                       : (TEST_TRUNC_VECTORS[i - TEST_VECTOR_COUNT].val &
                          ((TEST_TRUNC_VECTORS[i - TEST_VECTOR_COUNT].bits >= 64)
                           ? UINT64_MAX
                           : ((1ULL << TEST_TRUNC_VECTORS[i - TEST_VECTOR_COUNT].bits) - 1ULL)));

        for (size_t j = 0; j < total_inputs; j++) {
            const char *name2 = (j < TEST_VECTOR_COUNT)
                                ? TEST_VECTORS_32[j].name
                                : TEST_TRUNC_VECTORS[j - TEST_VECTOR_COUNT].label;
            uint64_t in2 = (j < TEST_VECTOR_COUNT)
                           ? (uint64_t)TEST_VECTORS_32[j].mask
                           : (TEST_TRUNC_VECTORS[j - TEST_VECTOR_COUNT].val &
                              ((TEST_TRUNC_VECTORS[j - TEST_VECTOR_COUNT].bits >= 64)
                               ? UINT64_MAX
                               : ((1ULL << TEST_TRUNC_VECTORS[j - TEST_VECTOR_COUNT].bits) - 1ULL)));

            twoadic_eval node = {
                .Real          = in1,
                .ShearRollover = 0ULL,
                .Imaginary     = 0ULL,
                .Flags         = 0ULL
            };

            xplane_2adic_eval_safe(&node, XPLANE_RUN_DEFAULT, in2);

            printf("[%s x %s] IN1: 0x%016llX | IN2: 0x%016llX -> OUT: 0x%016llX | %s\n",
                   name1,
                   name2,
                   (unsigned long long)in1,
                   (unsigned long long)in2,
                   (unsigned long long)node.Real,
                   binary_tail_hexa(node.Real, 1));
        }
    }
    printf("\n=== CHIPSET MATH & LOGIC GATE TALLY TESTS ===\n");

    /* Test 1: 3^2 + 4^2 = 5^2 (via sqrt(3^2 + 4^2) == 5) */
    ps_gate_reset();
    uint128_t a   = 3;
    uint128_t b   = 4;
    uint128_t rhs = 5;

    chipset_squared(&a, 32, 1);         /* a = 3^2 = 9 */
    chipset_squared(&b, 32, 1);         /* b = 4^2 = 16 */
    chipset_squared(&rhs, 32, 1);       /* rhs = 5^2 = 25 */

    uint128_t sum_sq = ps_gate_add(a, b); /* 9 + 16 = 25 */
    uint128_t hyp    = sum_sq;
    chipset_squareroot(&hyp, 32, 1);    /* sqrt(25) = 5 */

    printf("Test 1 (3^2 + 4^2 = 5^2): %llu + %llu = %llu (sqrt = %llu) [%s]\n",
           (unsigned long long)a,
           (unsigned long long)b,
           (unsigned long long)sum_sq,
           (unsigned long long)hyp,
           (sum_sq == rhs && hyp == 5) ? "PASS" : "FAIL");
    ps_gate_dump(stdout, "3^2 + 4^2 = 5^2");

    /* Test 2: (2 + 1) x 2 = 6 */
    ps_gate_reset();
    uint128_t val = 2;
    chipset_plus_1(&val, 32, 1);        /* 2 + 1 = 3 */
    chipset_multiply(&val, 32, 2);      /* 3 x 2 = 6 */

    printf("\nTest 2 ((2 + 1) x 2 = 6): result = %llu [%s]\n",
           (unsigned long long)val,
           (val == 6) ? "PASS" : "FAIL");
    ps_gate_dump(stdout, "(2 + 1) x 2 = 6");

    return 0;
}
/*
#ifdef PS_GATE_COUNT
  #define PS_DEF_STORAGE(name) atomic_uint_fast64_t g_calls_##name = 0;
  PS_GATE_LIST(PS_DEF_STORAGE)
  #undef PS_DEF_STORAGE

  void ps_gate_reset(void) {
      #define PS_DO_RESET(name) atomic_store_explicit(&g_calls_##name, 0, memory_order_relaxed);
      PS_GATE_LIST(PS_DO_RESET)
      #undef PS_DO_RESET
    }

  void ps_gate_dump(FILE *out, const char *label) {
      if (!out) return;
      fprintf(out, "=== GATE TELEMETRY [%s] ===\n", label ? label : "RUN");
      #define PS_DO_DUMP(name) fprintf(out, "%-16s : %llu\n", #name, \
          (unsigned long long)atomic_load_explicit(&g_calls_##name, memory_order_relaxed));
      PS_GATE_LIST(PS_DO_DUMP)
      #undef PS_DO_DUMP
      fprintf(out, "\n");
      fflush(out); /* Ensures lines hit disk immediately even if you Ctrl+C 
    } 
  FILE *log_fp = fopen("gate_tally.txt", "w"); /* Or "a" to append across runs 

/* Inside your prompt/test loop: 
ps_gate_reset();
/* ... run chipset operations ... 
ps_gate_dump(log_fp, "Query 1"); /* Writes to gate_tally.txt 
ps_gate_dump(stdout, "Query 1"); /* Optional: mirrors to terminal 

if (log_fp) fclose(log_fp);

*/ 
//#endif
