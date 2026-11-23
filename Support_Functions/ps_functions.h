/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: ps_functions.h		Author: Chapman		Created: 2026-11-21																		*/
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README	  							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef PS_FUNCTIONS_H
#define PS_FUNCTIONS_H

#include <stdio.h>
#include <stdint.h>

#define XPLANE_UNINITIALIZED        0xDEADBEEFDEADBEEFULL
#define XPLANE_INF                  0x7FFFFFFFFFFFFFFFULL
#define XPLANE_OMIT_PARAM           0xFFFFFFFFFFFFFFFFULL 
#define XPLANE_MASK_QUALITY_F       0x8000000000000000ULL 

/* Rule 9 Structural Exit Vectors */
#define XPLANE_FLAG_EXIT_REAL       0x0000000000000100ULL 
#define XPLANE_FLAG_EXIT_POS        0x0000000000000200ULL 
#define XPLANE_FLAG_EXIT_NEG        0x0000000000000400ULL 

/* Exact RDX Hard Register Masks */
#define XPLANE_MASK_SHEAR_COUNT       0x0000000FULL 
#define XPLANE_MASK_I_FLAG_COUNT      0x00007FE0ULL  
#define XPLANE_MASK_MULTI_SHEAR       0x07FF8000ULL  
#define XPLANE_MASK_EXIT_MULTIPLICITY 0x18000000ULL  
#define XPLANE_MASK_ROT_OUT           0x60000000ULL  
#define XPLANE_MASK_ROT_IN            0xC0000000ULL  

#define XPLANE_EXIT_INF_NEG           0x08000000ULL  
#define XPLANE_EXIT_INF_POS           0x10000000ULL  

typedef enum {
    XPLANE_RUN_DEFAULT        = 0x00, 
    XPLANE_OPT_ACCURACY       = 0x02,  // 128bit-67bit accuracy toggle.
    XPLANE_NO_POOLING         = 0x04,  // Xplane Bypass.
    XPLANE_DISCARD_RESIDUE    = 0x08,  // Abstractly: take the tangent.
    XPLANE_OPT_EXIT_INF_NEG   = 0x10,  // -1 Infinity Exit
    XPLANE_OPT_EXIT_INF_POS   = 0x20   // +1 Infinity Exit
} twoadic_opt_t;

typedef struct {            //from here down may have to be upgraded to uint128
    uint64_t Real;          
    uint64_t ShearRollover; 
    uint64_t Imaginary;     
    uint64_t Flags;         
} twoadic_eval;

void xplane_2adic_eval(twoadic_eval* node, twoadic_opt_t options, ...);

#define XPLANE_CAST_U64_1(a)          ((uint64_t)(a))
#define XPLANE_CAST_U64_2(a, b)       ((uint64_t)(a)), ((uint64_t)(b))
#define XPLANE_CAST_U64_3(a, b, c)    ((uint64_t)(a)), ((uint64_t)(b)), ((uint64_t)(c))
#define XPLANE_GET_MACRO(_1,_2,_3,NAME,...) NAME
#define XPLANE_ARGS(...) XPLANE_GET_MACRO(__VA_ARGS__, XPLANE_CAST_U64_3, XPLANE_CAST_U64_2, XPLANE_CAST_U64_1)(__VA_ARGS__)

/* Safe call wrapper: automatically widens any uint8_t/16/32 inputs to uint64_t */
#define xplane_2adic_eval_safe(node, opts, ...) \
    xplane_2adic_eval((node), (opts), XPLANE_ARGS(__VA_ARGS__), XPLANE_OMIT_PARAM)

const char *binary_tail_hexa_impl(uint64_t val, uint8_t bits, int upper, char *buf);

    /* Auto-detects bit width from the casted integer type (8, 16, 32, or 64 bits) */
#define binary_tail_hexa(val, upper) \
    binary_tail_hexa_impl((uint64_t)(val), (uint8_t)(sizeof(val) * 8), (upper), (char[48]){0})

/* Explicit arbitrary bit-width macro (1 to 64 bits) */
#define binary_tail_hexa_bits(val, bits, upper) \
    binary_tail_hexa_impl((uint64_t)(val), (uint8_t)(bits), (upper), (char[48]){0})    

#endif /* PS_FUNCTIONS_H */
