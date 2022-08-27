#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <emmintrin.h>

void transpose(uint32_t matrix[16]){
    uint32_t tmp[16];

    tmp[ 0] = matrix[ 0];
    tmp[ 1] = matrix[ 4];
    tmp[ 2] = matrix[ 8];
    tmp[ 3] = matrix[12];

    tmp[ 4] = matrix[ 1];
    tmp[ 5] = matrix[ 5];
    tmp[ 6] = matrix[ 9];
    tmp[ 7] = matrix[13];

    tmp[ 8] = matrix[ 2];
    tmp[ 9] = matrix[ 6];
    tmp[10] = matrix[10];
    tmp[11] = matrix[14];

    tmp[12] = matrix[ 3];
    tmp[13] = matrix[ 7];
    tmp[14] = matrix[11];
    tmp[15] = matrix[15];

    for (size_t i = 0; i < 16; i++) {
        matrix[i] = tmp[i];
    }
}

/*  This function rotates the diagonals of the input matrix such that
*   they end up in their respective row. This enables the use of simd
*   because each row consisting of 4 uint32_t's can be loaded directly
*   into one _m128i_u variable.
*/
void rotate_simd(uint32_t matrix[16]) {
    uint32_t tmp[16];

    tmp[ 0] = matrix[ 0];
    tmp[ 1] = matrix[ 5];
    tmp[ 2] = matrix[10];
    tmp[ 3] = matrix[15];
    tmp[ 4] = matrix[ 4];
    tmp[ 5] = matrix[ 9];
    tmp[ 6] = matrix[14];
    tmp[ 7] = matrix[ 3];
    tmp[ 8] = matrix[ 8];
    tmp[ 9] = matrix[13];
    tmp[10] = matrix[ 2];
    tmp[11] = matrix[ 7];
    tmp[12] = matrix[12];
    tmp[13] = matrix[ 1];
    tmp[14] = matrix[ 6];
    tmp[15] = matrix[11];

    for (size_t i = 0; i < 16; i++) {
        matrix[i] = tmp[i];
    }
}

/*  This is the inverse function to the rotate_simd function. It reverts
*   the matrix to a state before the initial rotation of the diagonals.
*/
void rotate_simd_rev(uint32_t matrix[16]) {
    uint32_t tmp[16];

    tmp[ 0] = matrix[ 0];
    tmp[ 1] = matrix[13];
    tmp[ 2] = matrix[10];
    tmp[ 3] = matrix[ 7];
    tmp[ 4] = matrix[ 4];
    tmp[ 5] = matrix[ 1];
    tmp[ 6] = matrix[14];
    tmp[ 7] = matrix[11];
    tmp[ 8] = matrix[ 8];
    tmp[ 9] = matrix[ 5];
    tmp[10] = matrix[ 2];
    tmp[11] = matrix[15];
    tmp[12] = matrix[12];
    tmp[13] = matrix[ 9];
    tmp[14] = matrix[ 6];
    tmp[15] = matrix[ 3];

    for (size_t i = 0; i < 16; i++) {
        matrix[i] = tmp[i];
    }
}

void print_matrix(uint32_t matrix[16]){
    for (size_t i = 0; i < 4; i++) {
        printf("%#010x %#010x %#010x %#010x \n", matrix[4*i], matrix[4*i + 1], matrix[4*i + 2], matrix[4*i + 3]);
    }
    printf("\n");
}

int mtr_equal(uint32_t m1[16], uint32_t m2[16]) {
    for (size_t i = 0; i < 16; i++) {
        if (m1[i] != m2[i]) {
            printf("Matrices don't match at index: %lu\n", i);
            printf("m1:\n");
            print_matrix(m1);
            printf("m2:\n");
            print_matrix(m2);
            return 1;
        }
    }

    printf("Matrices are equal\n");
    return 0;
}