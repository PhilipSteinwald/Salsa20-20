#ifndef SALSA20_CRYPT_V0_H
#define SALSA20_CRYPT_V0_H

#include <aio.h>
#include <stdint.h>

typedef void (*core_func)(uint32_t[16], const uint32_t[16]);

void salsa20_crypt_v0(size_t mlen, const uint8_t msg[], uint8_t cipher[], uint32_t key[8], uint64_t iv, core_func core);

#endif  // SALSA20_CRYPT_V0_H
