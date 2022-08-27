#include <aio.h>
#include <stdint.h>

#include "mtr_util.h"

typedef void (*core_func)(uint32_t[16], const uint32_t[16]);

/*
*   This crypt implementation is a normal SISD implementation,
*   which creates all needed uint32_t variables to set up the
*   matrix for calling the given core method.
*/
void salsa20_crypt_v0(size_t mlen, const uint8_t msg[mlen], uint8_t cipher[mlen], uint32_t key[8], uint64_t iv, core_func core){
    uint64_t counter = 0;
    size_t n = 0;
    uint32_t cons[4] = { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };
    uint32_t iv32[2];
    iv32[0] = 0xffffffff & iv;
    iv32[1] = iv >> 32;

    /*  fills the 4x4 matrix represtented by an uint32_t array with the following variables:
    *   j\i    1:             2:            3:        4:
    *   1:  / cons[0]        key[0]        key[1]    key[2]  \
    *   2:  | key[3]         cons[1]       iv(low)  iv(high) |
    *   3:  | counter(low)  counter(high)  cons[2]   key[4]  |
    *   4:  \ key[5]         key[6]        key[7]    cons[3] /
    *   Author: Philip Steinwald
    */
    while(n < mlen){
        uint32_t c32[2];
        c32[0] = 0xffffffff & counter;
        c32[1] = counter >> 32;

        const uint32_t input[32] = {
            cons[0], key[0], key[1], key[2],
            key[3], cons[1], iv32[0], iv32[1],
            c32[0], c32[1], cons[2], key[4],
            key[5], key[6], key[7], cons[3]
        };
	    uint32_t output[32];

        core(output,input);

        int i=0;
        uint8_t out[64];
        //converting uint32_t[16] to uint8_t[64] to xor it with the message chars to get the cipher
        while(i<16){
            //IMPORTANT: the following order keeps little endian
            out[i * 4] = (output[i] & 0x000000ff);
            out[(i * 4) + 1] = (output[i] & 0x0000ff00) >> 8;
            out[(i * 4) + 2] = (output[i] & 0x00ff0000) >> 16;
            out[(i * 4) + 3] = (output[i] & 0xff000000) >> 24;
            i++;
        }

        i = 0;
        while(n + i < mlen && i < 64){
            cipher[n + i] = out[i] ^ msg[n + i];
            i++;
        }
        n += 64;
        //important to increase counter to get a different matrix when calling salsa20_core next time
        counter++;
    }
}
