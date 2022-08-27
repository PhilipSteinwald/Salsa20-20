#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <stdint.h>

typedef void (*core_func)(uint32_t[16], const uint32_t[16]);

typedef void (*crypt_func)(size_t mlen, const uint8_t[mlen], uint8_t[mlen], uint32_t[8], uint64_t, core_func);

void performance_core(uint64_t iter, core_func f, uint32_t output[16], const uint32_t input[16], const char* fname);

void performance(uint64_t iter, crypt_func crypt, core_func core, size_t mlen, const uint8_t msg[mlen], uint8_t cipher[mlen], uint32_t key[8], uint64_t iv, const char* fname);

#endif
