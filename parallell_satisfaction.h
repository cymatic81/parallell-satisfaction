#ifndef PARALLELL_SATISFACTION_H
#define PARALLELL_SATISFACTION_H

#include <stdio.h>
#include <stdint.h>

#define binary_tail_hexa(reg, upper) \
    binary_tail_hexa_impl((uint64_t)(reg), (uint8_t)(sizeof(reg) * 8), (upper), (char[48]){0})

static inline const char *binary_tail_hexa_impl(uint64_t val, uint8_t bits, int upper, char *buf) {
    val &= (bits >= 64) ? UINT64_MAX : ((1ULL << bits) - 1ULL);

    char top[16] = {0}, bot[16] = {0};
    for (int i = 0; i < 8; i++) {
        top[i * 2] = ((val >> (bits - 1 - i)) & 1ULL) ? '1' : '0';
        bot[i * 2] = ((val >> (7 - i)) & 1ULL) ? '1' : '0';
        if (i < 7) {
            top[i * 2 + 1] = ' ';
            bot[i * 2 + 1] = ' ';
        }
    }

    if (bits <= 16) {
        snprintf(buf, 48, "%s %s", top, bot);
    } else {
        int mid_digits = (bits - 16) / 4;
        uint64_t mid_mask = (1ULL << (bits - 16)) - 1ULL;
        uint64_t mid_val = (val >> 8) & mid_mask;
        snprintf(buf, 48, upper ? "%s %0*llX %s" : "%s %0*llx %s",
                 top, mid_digits, (unsigned long long)mid_val, bot);
    }
    return buf;
}

#endif /* BIN_TAIL_H */
