/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: ps_functions.c		Author: Chapman		Created: 2026-11-21																		*/
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README	  							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "ps_functions.h"
#include <stdio.h>
#include <stdarg.h>

/*<<<<<Printf Helper>>>>>*/
const char *binary_tail_hexa_impl(uint64_t val, uint8_t bits, int upper, char *buf) {
    if (bits == 0) {
        buf[0] = '\0';
        return buf;
    }
    if (bits > 64) bits = 64;

    val &= (bits == 64) ? UINT64_MAX : ((1ULL << bits) - 1ULL);

    if (bits < 16) {
        for (int i = 0; i < bits; i++) {
            buf[i * 2] = ((val >> (bits - 1 - i)) & 1ULL) ? '1' : '0';
            if (i < bits - 1) buf[i * 2 + 1] = ' ';
        }
        buf[bits * 2 - 1] = '\0';
        return buf;
    }

    char top[16] = {0}, bot[16] = {0};
    for (int i = 0; i < 8; i++) {
        top[i * 2] = ((val >> (bits - 1 - i)) & 1ULL) ? '1' : '0';
        bot[i * 2] = ((val >> (7 - i)) & 1ULL) ? '1' : '0';
        if (i < 7) {
            top[i * 2 + 1] = ' ';
            bot[i * 2 + 1] = ' ';
        }
    }

    if (bits == 16) {
        snprintf(buf, 48, "%s %s", top, bot);
    } else {
        int mid_digits = (bits - 16 + 3) / 4;
        uint64_t mid_mask = (1ULL << (bits - 16)) - 1ULL;
        uint64_t mid_val = (val >> 8) & mid_mask;
        snprintf(buf, 48, upper ? "%s %0*llX %s" : "%s %0*llx %s",
                 top, mid_digits, (unsigned long long)mid_val, bot);
    }
    return buf;
}

/*<<<<<_xplane_2adic_eval_>>>>>*/
void xplane_2adic_eval(twoadic_eval* node, twoadic_opt_t options, ...) {
    if (!node) return;

    /* HARD REGISTER GATE: Evaluate initial Rollover Shear limits (Bits 0-3) first */
    uint64_t rollover_shear = node->Flags & XPLANE_MASK_SHEAR_COUNT;
    if (rollover_shear >= 9) {
        if (!(options & XPLANE_OPT_ACCURACY)) {
            node->Real  = XPLANE_UNINITIALIZED;
            node->Flags = XPLANE_MASK_QUALITY_F;
            return;
        }
    }

    /* 1. VA_ARG ONBOARDING AND REGISTER FLAGS CYCLING */
    uint64_t t1 = 0ULL;
    uint64_t t2 = 0ULL;

    va_list args;
    va_start(args, options);
    
    uint64_t custom_t2 = va_arg(args, uint64_t);
    
    uint64_t flag_arg;
    while ((flag_arg = va_arg(args, uint64_t)) != XPLANE_OMIT_PARAM) {
        node->Flags |= flag_arg; 
    }
    
    va_end(args);

    /* 2. DYNAMIC OPERATIONAL OVERRIDE: Sync option enums to RDX register structures */
    if (options & XPLANE_OPT_EXIT_INF_NEG) node->Flags |= XPLANE_EXIT_INF_NEG;
    if (options & XPLANE_OPT_EXIT_INF_POS) node->Flags |= XPLANE_EXIT_INF_POS;

    /* 3. RULE 7: EVALUATE TRANSFINITE SHEAR FROM OPERATIONS AFTER INITIAL ROLLOVER SAFETY */
    uint64_t operations_shear = (node->Flags & XPLANE_MASK_MULTI_SHEAR) >> 15;
    uint64_t total_shear_profile = rollover_shear + operations_shear;
    
    if (total_shear_profile >= 9) {
        if (!(options & XPLANE_OPT_ACCURACY)) {
            node->Real  = XPLANE_UNINITIALIZED;
            node->Flags = XPLANE_MASK_QUALITY_F;
            return;
        }
    }

    if (custom_t2 != XPLANE_OMIT_PARAM) {
        t1 = node->Real;
        t2 = custom_t2;
    } else {
        t1 = node->Real & 0x5555555555555555ULL; 
        t2 = node->Real & 0xAAAAAAAAAAAAAAAAULL;
    }

    /* 4. OPTIONAL MODIFIER ROUTING: DISCARD RESIDUE TANGENT MODE */
    if (options & XPLANE_DISCARD_RESIDUE) {
        node->Real = node->Imaginary;
        node->Flags &= ~XPLANE_MASK_QUALITY_F;
        return; 
    }

    /* 5. RULE 4 PHASE INVERSION (Extract (i) count from Bits 5-14) */
    uint64_t winding = (node->Flags & XPLANE_MASK_I_FLAG_COUNT) >> 5;
    if (winding & 1) {
        t1 = ~t1;
        t2 = ~t2;
    }

    /* 6. UNROLLED BINARY EVAL SWEEP */
    uint64_t low = 0ULL, high = 0xFFFFFFFFFFFFFFFFULL, mid = 0ULL;
    for (int i = 0; i < 64; i++) {
        mid = low + ((high - low) >> 1);
        if (mid == t1) { t1 = mid; break; }
        if (mid < t1)  { low = mid + 1ULL; }
        else           { if (mid == 0ULL) break; high = mid - 1ULL; }
    }
    if (t1 != mid) t1 = low;

    /* 7. RULE 9 COLLAPSE & MULTIPLICITY EVALUATION */
    if (t1 == 1ULL) {
        node->Flags |= XPLANE_FLAG_EXIT_POS;
        if (node->Flags & XPLANE_EXIT_INF_POS) {
            node->Real = XPLANE_INF;
        } else {
            node->Real = t1;
        }
    } else if (t1 == (~0ULL)) {
        node->Flags |= XPLANE_FLAG_EXIT_NEG;
        if (node->Flags & XPLANE_EXIT_INF_NEG) {
            node->Real = 0ULL; 
        } else {
            node->Real = t1;
        }
    } else {
        node->Flags |= XPLANE_FLAG_EXIT_REAL;
        node->Real = (options & XPLANE_NO_POOLING) ? 0ULL : (t1 | t2);
    }
}
