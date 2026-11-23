/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: verify_pipeline.c		Author: Chapman		Created: 2026-11-21																		*/
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README	  							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define PS_ENABLE_VERIFY_HEADERS
#include "../parallel_satisfaction.h"
#include <string.h>

static inline void verify_gate_dump(FILE *out, const char *label) {
#ifdef PS_GATE_COUNT
    if (!out) out = stdout;
    fprintf(out, "=== Chipset Op Tally [%s] ===\n", label ? label : "default");
    #define VERIFY_PRINT_COUNTER(name) do { \
        if (strcmp(#name, "simd_bcast") == 0) { \
            fprintf(out, "===simd layer===\n"); \
        } \
        if (strcmp(#name, "and") == 0) { \
            fprintf(out, "===ops to gate conversion===\n"); \
        } \
   char disp[64]; \
        if (strcmp(#name, "add") == 0 || strcmp(#name, "sub") == 0) { \
            snprintf(disp, sizeof(disp), "chipset_%s (tick/gate stack)", #name); \
        } else { \
            snprintf(disp, sizeof(disp), "chipset_%s", #name); \
        } \
        fprintf(out, "  %-30s : %llu\n", disp, \
                (unsigned long long)atomic_load_explicit(&(g_calls_chipset_##name), memory_order_relaxed)); \
    } while (0);
    PS_CHIPSET_OP_LIST(VERIFY_PRINT_COUNTER)
    #undef VERIFY_PRINT_COUNTER
#else
    (void)out;
    (void)label;
#endif
}

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

    printf("\n=== SIMD / VECTOR LAYER & POPCOUNT TALLY TESTS ===\n");
    ps_gate_reset();

    /* 1. Broadcast RAX across four 32-bit lanes (hits: simd_bcast, mov_v) */
    uint128_t simd_vec = 0;
    simd_broadcast_rax(&simd_vec, VEC_DEADBEEF, 32);
    int ok_bcast = (simd_vec == VEC128_DEADBEEF);

    /* 2. Insert CAFEBABE into lane 2 & extract lane 2 back to RAX (hits: simd_extract, mov_v, mov_r) */
    uint64_t extracted_rax = 0;
    simd_insert_lane(&simd_vec, VEC_CAFEBABE, 2, 32);
    simd_extract_lane(&simd_vec, &extracted_rax, 2, 32);
    int ok_ins_ext = (extracted_rax == VEC_CAFEBABE);

    /* 3. Permute 32-bit lanes: swap lane 0 <-> lane 2 via perm_mask 0b11_00_01_10 = 0xC6 (hits: simd_permute, mov_vv) */
    simd_permute_lanes(&simd_vec, 0xC6U, 32);
    uint64_t lane0_after_perm = 0;
    simd_extract_lane(&simd_vec, &lane0_after_perm, 0, 32);
    int ok_perm = (lane0_after_perm == VEC_CAFEBABE);

    /* 4. Vector AND & XOR with scalar RAX across 32-bit lanes (hits: simd_and_rax, simd_xor_rax) */
    uint128_t logic_vec = VEC128_ALL_ONES;
    simd_ymm_and_rax(&logic_vec, VEC_5A5A5A5A, 32);
    simd_ymm_xor_rax(&logic_vec, VEC_5A5A5A5A, 32);
    int ok_vec_logic = (logic_vec == VEC128_NULL);

    /* 5. Parallel lane INC & DEC across four 32-bit lanes (hits: inc, dec) */
    uint128_t inc_vec = VEC128_NULL;
    simd_lane_inc(&inc_vec, 32, 4);
    int ok_inc = ((uint64_t)inc_vec == 0x0000000100000001ULL);
    simd_lane_dec(&inc_vec, 32, 4);
    int ok_dec = (inc_vec == VEC128_NULL);

    /* 6. Cross-lane carry bridge: MSB of lane 0 (0x80000000) bridges into bit 0 of lane 1 (hits: simd_carry_bridge) */
    uint128_t carry_vec = (uint128_t)VEC_WALK_MSB;
    simd_carry_bridge(&carry_vec, 32, 4);
    uint64_t lane1_carried = 0;
    simd_extract_lane(&carry_vec, &lane1_carried, 1, 32);
    int ok_carry = (lane1_carried == 1ULL);

    /* 7. Lane POP1 & POP0 across four 32-bit lanes of 0x55555555 (16 ones, 16 zeros per lane) */
    uint128_t pop1_vec = VEC128_55555555;
    uint128_t pop0_vec = VEC128_55555555;
    simd_lane_pop1(&pop1_vec, 32, 4);
    simd_lane_pop0(&pop0_vec, 32, 4);
    int ok_pop = ((uint64_t)pop1_vec == 0x0000001000000010ULL &&
                  (uint64_t)pop0_vec == 0x0000001000000010ULL);

    printf("SIMD Suite: bcast[%s] ins/ext[%s] perm[%s] and/xor[%s] inc/dec[%s] carry[%s] pop1/0[%s]\n",
           ok_bcast ? "PASS" : "FAIL",
           ok_ins_ext ? "PASS" : "FAIL",
           ok_perm ? "PASS" : "FAIL",
           ok_vec_logic ? "PASS" : "FAIL",
           (ok_inc && ok_dec) ? "PASS" : "FAIL",
           ok_carry ? "PASS" : "FAIL",
           ok_pop ? "PASS" : "FAIL");
    verify_gate_dump(stdout, "SIMD Layer & Popcount");

    printf("\n=== CHIPSET MATH & LOGIC GATE TALLY TESTS ===\n");

    /* Test 1: (2 + 1) x 2 = 6 (shows 2 loads, 2 sends) */
    ps_gate_reset();
    uint128_t val = ((uint128_t)1 << 32) | 2;
    chipset_add(&val, 32);                 /* 2 + 1 = 3  (load: 1, send: 1) */

    val = ((uint128_t)2 << 32) | val;
    chipset_multiply(&val, 32);            /* 3 x 2 = 6  (load: 2, send: 2) */

    printf("Test 1 ((2 + 1) x 2 = 6): result = %llu [%s]\n",
           (unsigned long long)val,
           (val == 6) ? "PASS" : "FAIL");
    verify_gate_dump(stdout, "(2 + 1) x 2 = 6");

    /* Test 2: (20 - 6) / 2 = 7 (shows 2 loads, 2 sends) */
    ps_gate_reset();
    uint128_t dval = ((uint128_t)6 << 32) | 20;
    chipset_sub(&dval, 32);                /* 20 - 6 = 14 (load: 1, send: 1) */

    dval = ((uint128_t)2 << 32) | dval;
    chipset_divide(&dval, 32);             /* 14 / 2 = 7  (load: 2, send: 2) */

    printf("\nTest 2 ((20 - 6) / 2 = 7): result = %llu [%s]\n",
           (unsigned long long)dval,
           (dval == 7) ? "PASS" : "FAIL");
    verify_gate_dump(stdout, "(20 - 6) / 2 = 7");

    /* Test 3: Shift + Direct Gate & MOV Coverage (hits: op_shfx, shfx8, or, xor, dec, mov*) */
    ps_gate_reset();
    uint128_t s_lane = 0x05;
    chipset_shfx(&s_lane, 32, 8);          /* Left shift 8 bits  -> 0x0500 (load: 1, send: 1) */
    chipset_shfx(&s_lane, 32, 0x80U | 8U); /* Right shift 8 bits -> 0x0005 (load: 2, send: 2) */

    uint128_t g_or  = $OR$(0x0A, 0x05);    /* 0x0F (triggers or) */
    uint128_t g_xor = $XOR$(g_or, 0x0A);   /* 0x05 (triggers xor) */
    uint128_t g_dec = $DEC$(s_lane);       /* 4    (triggers dec) */

    /* Placeholder MOV coverage across all 4 transfer domains */
    uint128_t m_reg = $MOV$(g_dec);        /* reg -> reg */
    uint128_t m_vec = $MOV_V$(m_reg);      /* reg -> vector */
    m_vec           = $MOV_VV$(m_vec);     /* vector -> vector */
    uint128_t m_out = $MOV_r$(m_vec);      /* vector -> reg */

    printf("\nTest 3 (SHFX8 + Gates + MOV*): shfx = %llu, xor = %llu, mov = %llu [%s]\n",
           (unsigned long long)s_lane,
           (unsigned long long)g_xor,
           (unsigned long long)m_out,
           (s_lane == 5 && g_xor == 5 && m_out == 4) ? "PASS" : "FAIL");
    verify_gate_dump(stdout, "SHFX8 + Gates + MOV*");

    /* Test 4: 3^2 + 4^2 = 5^2 (shows 5 loads, 5 sends across 3 squares + 1 add + 1 sqrt) */
    ps_gate_reset();
    uint128_t a   = 3;
    uint128_t b   = 4;
    uint128_t rhs = 5;

    chipset_squared(&a, 32);               /* 3^2 = 9   (load: 1, send: 1) */
    chipset_squared(&b, 32);               /* 4^2 = 16  (load: 2, send: 2) */
    chipset_squared(&rhs, 32);             /* 5^2 = 25  (load: 3, send: 3) */

    uint128_t sum_sq = ((uint128_t)b << 32) | a;
    chipset_add(&sum_sq, 32);              /* 9 + 16 = 25 (load: 4, send: 4) */

    uint128_t hyp = sum_sq;
    chipset_squareroot(&hyp, 32);          /* sqrt(25) = 5 (load: 5, send: 5) */

    printf("\nTest 4 (3^2 + 4^2 = 5^2): %llu + %llu = %llu (sqrt = %llu) [%s]\n",
           (unsigned long long)a,
           (unsigned long long)b,
           (unsigned long long)sum_sq,
           (unsigned long long)hyp,
           (sum_sq == rhs && hyp == 5) ? "PASS" : "FAIL");
    verify_gate_dump(stdout, "3^2 + 4^2 = 5^2");

    return 0;
}
