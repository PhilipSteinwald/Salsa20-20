#ifndef ECRYPT_H
#define ECRYPT_H

#include "ecrypt-sync.h"
#include <stdio.h>
#include <stdint.h>

void salsa20_wordtobyte(u8 output[64],const u32 input[16]);
void ECRYPT_keysetup(ECRYPT_ctx *x,const u8 *k,u32 kbits,u32 ivbits);
void ECRYPT_ivsetup(ECRYPT_ctx *x,const u8 *iv);
void ECRYPT_encrypt_bytes(ECRYPT_ctx *x,const u8 *m,u8 *c,u32 bytes);


#endif
