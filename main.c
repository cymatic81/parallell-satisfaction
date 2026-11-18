#include <stdio.h>
#include <stdint.h>
#include "parallell_satisfaction.h"
#include "test.h"

int main(void) {
    printf("=== 32-BIT VECTOR MASKS (UPPERCASE) ===\n");
    for (size_t i = 0; i < TEST_VECTOR_COUNT; i++) {
        printf("%s : %s\n",
               TEST_VECTORS_32[i].name,
               binary_tail_hexa(TEST_VECTORS_32[i].mask, 1));
    }

    printf("\n=== ARBITRARY UINT CASTS & LOWERCASE ===\n");
    printf("uint64_t    : %s\n", binary_tail_hexa((uint64_t)0x0123456789ABCDEFULL, 0));
    printf("uint32_t    : %s\n", binary_tail_hexa((uint32_t)VEC_DEADBEEF, 0));
    printf("uint16_t    : %s\n", binary_tail_hexa((uint16_t)0xCAFE, 0));
    printf("uint8_t     : %s\n", binary_tail_hexa((uint8_t)0xA5, 0));

    return 0;
}
