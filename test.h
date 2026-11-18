#ifndef TEST_H
#define TEST_H

#include <stdint.h>

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

#endif /* TEST_H */
