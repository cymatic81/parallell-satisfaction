/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* File: verify_pipetest.h		Author: Chapman		Created: 2026-11-21																		*/
/* License: Copyright (CC BY-NC) Attribution-NonCommercial-ShareAlike 4.0 International. See README	  							*/
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef VERIFY_PIPETEST_H
#define VERIFY_PIPETEST_H

#include <stdint.h>
#include <stddef.h>

/* Standard 32-bit test vector masks */
#define VEC_ZERO        ((uint32_t)0x00000000U)
#define VEC_ALL_ONES    ((uint32_t)0xFFFFFFFFU)
#define VEC_ALT_01      ((uint32_t)0x55555555U)
#define VEC_ALT_10      ((uint32_t)0xAAAAAAAAU)
#define VEC_WALK_MSB    ((uint32_t)0x80000000U)
#define VEC_WALK_LSB    ((uint32_t)0x00000001U)
#define VEC_BYTE_LANES  ((uint32_t)0xFF00FF00U)
#define VEC_NIBBLE_MASK ((uint32_t)0x0F0F0F0FU)
#define VEC_DEADBEEF    ((uint32_t)0xDEADBEEFU)
#define VEC_CAFEBABE    ((uint32_t)0xCAFEBABEU)

/* Combined 64-bit source for >32-bit truncations */
#define VEC_DEAD_CAFE_64 (((uint64_t)VEC_DEADBEEF << 32) | (uint64_t)VEC_CAFEBABE)

static const struct {
    const char *name;
    uint32_t mask;
} TEST_VECTORS_32[] = {
    { "ZERO       ", VEC_ZERO        },
    { "ALL_ONES   ", VEC_ALL_ONES    },
    { "ALT_01     ", VEC_ALT_01      },
    { "ALT_10     ", VEC_ALT_10      },
    { "WALK_MSB   ", VEC_WALK_MSB    },
    { "WALK_LSB   ", VEC_WALK_LSB    },
    { "BYTE_LANES ", VEC_BYTE_LANES  },
    { "NIBBLE_MASK", VEC_NIBBLE_MASK },
    { "DEADBEEF   ", VEC_DEADBEEF    },
    { "CAFEBABE   ", VEC_CAFEBABE    }
};

#define TEST_VECTOR_COUNT (sizeof(TEST_VECTORS_32) / sizeof(TEST_VECTORS_32[0]))

/* Arbitrary bit-length truncations of DEADBEEF and CAFEBABE */
static const struct {
    const char *label;
    uint64_t val;
    uint8_t bits;
    int upper;
} TEST_TRUNC_VECTORS[] = {
    { "DEADBEEF 1-bit ", VEC_DEADBEEF,     1,  1 },
    { "CAFEBABE 4-bit ", VEC_CAFEBABE,     4,  1 },
    { "DEADBEEF 7-bit ", VEC_DEADBEEF,     7,  1 },
    { "CAFEBABE 12-bit", VEC_CAFEBABE,     12, 1 },
    { "DEADBEEF 15-bit", VEC_DEADBEEF,     15, 1 },
    { "CAFEBABE 20-bit", VEC_CAFEBABE,     20, 0 },
    { "DEADBEEF 24-bit", VEC_DEADBEEF,     24, 1 },
    { "DEADCAFE 48-bit", VEC_DEAD_CAFE_64, 48, 0 }
};

#define TEST_TRUNC_COUNT (sizeof(TEST_TRUNC_VECTORS) / sizeof(TEST_TRUNC_VECTORS[0]))

#endif /* VERIFY_PIPETEST_H */
