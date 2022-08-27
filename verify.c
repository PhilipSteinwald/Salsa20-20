#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#include "core_v0.h"
#include "core_v1.h"
#include "core_v2.h"
#include "core_v3.h"
#include "crypt_v0.h"
#include "crypt_v1.h"
#include "mtr_util.h"
#include "reference/ecrypt-sync.h"
#include "reference/ecrypt.h"

int verify_core(){
    int failed = 0;

    //https://cr.yp.to/snuffle/salsafamily-20071225.pdf (taken from chapter 4.1)
    uint32_t verification_in[] = {
        0x61707865, 0x04030201, 0x08070605, 0x0c0b0a09,
        0x100f0e0d, 0x3320646e, 0x01040103, 0x06020905,
        0x00000007, 0x00000000, 0x79622d32, 0x14131211,
        0x18171615, 0x1c1b1a19, 0x201f1e1d, 0x6b206574
    };

    uint32_t verification_out[] = {
        0xb9a205a3, 0x0695e150, 0xaa94881a, 0xadb7b12c,
        0x798942d4, 0x26107016, 0x64edb1a4, 0x2d27173f,
        0xb1c7f1fa, 0x62066edc, 0xe035fa23, 0xc4496f04,
        0x2131e6b3, 0x810bde28, 0xf62cb407, 0x6bdede3d
    };

    uint32_t out[16];

    printf("Comparing v0 \x1B[1;36m	(sisd + transpose) \x1B[0m	and reference matrix...\n");
    salsa20_core_v0(out, verification_in);
    if (mtr_equal(out, verification_out)) {
        failed++;
    }
    printf("\n");

    printf("Comparing v1 \x1B[1;36m	(simd + transpose) \x1B[0m	and reference matrix...\n");
    salsa20_core_v1(out, verification_in);
    if (mtr_equal(out, verification_out)) {
        failed++;
    }
    printf("\n");

    printf("Comparing v2 \x1B[1;36m	(sisd + no transpose) \x1B[0m	and reference matrix...\n");
    salsa20_core_v2(out, verification_in);
    if (mtr_equal(out, verification_out)) {
        failed++;
    }
    printf("\n");

    printf("Comparing v3 \x1B[1;36m	(simd + no transpose) \x1B[0m	and reference matrix...\n");
    salsa20_core_v3(out, verification_in);
    if (mtr_equal(out, verification_out)) {
        failed++;
    }
    printf("\n\n");

    return failed;
}

int verify_crypt(){
    int failed = 0;
    
    u32 inp[16] = { 
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    ECRYPT_ctx m = {.input = {*inp}};

    u8 k_8[] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    
    uint32_t k_32[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    u8 iv[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    ECRYPT_keysetup(&m, k_8, 256, 0);
    ECRYPT_ivsetup(&m, iv);

    char* str = "This is an example text that will be encrypted.";
    size_t len = strlen(str);

    char mes[len+1];
    mes[len] = '\0';
    memcpy(mes, str, len);

    char cip[len+1];
    cip[len] = '\0';

    ECRYPT_encrypt_bytes(&m,(uint8_t*) mes, (uint8_t*) cip, len);

    salsa20_crypt_v0(len, (uint8_t*) cip, (uint8_t*) mes, k_32, 0, salsa20_core_v0);

    printf("Expected: %s\nActual:   %s\n", str, mes);

    if (!strcmp(str, mes)) {
        printf("Actual and expected strings are\x1B[1;36m equal\x1B[0m, v0-crypt is equivalent to the reference implementation\n\n" );
    } else {
        printf("Actual and expected strings are\x1B[1;31m not equal\x1B[0m! (v0)\n");
        failed++;
    }

    char ciph[len+1];
    ciph[len] = '\0';

    ECRYPT_keysetup(&m, k_8, 256, 0);

    ECRYPT_ivsetup(&m, iv);

    ECRYPT_encrypt_bytes(&m,(uint8_t*) mes, (uint8_t*) ciph, len);

    salsa20_crypt_v1(len, (uint8_t*) ciph, (uint8_t*) mes, k_32, 0, salsa20_core_v0);

    printf("Expected: %s\nActual: %s\n", str, mes);

    if (!strcmp(str, mes)) {
        printf("Actual and expected strings are\x1B[1;36m equal\x1B[0m, v1-crypt is equivalent to the reference implementation\n" );
    } else {
        printf("Actual and expected strings are\x1B[1;31m not equal\x1B[0m! (v1)\n");
        failed++;
    }
    
    return failed;
}
