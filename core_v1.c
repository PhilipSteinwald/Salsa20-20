#include <stdint.h>
#include <emmintrin.h>

#include "mtr_util.h"

void rotate_simd_transpose(uint32_t matrix[16]) {
    uint32_t tmp[16];

    tmp[ 0] = matrix[ 0];
    tmp[ 1] = matrix[ 1];
    tmp[ 2] = matrix[ 2];
    tmp[ 3] = matrix[ 3];

    tmp[ 4] = matrix[13];
    tmp[ 5] = matrix[14];
    tmp[ 6] = matrix[15];
    tmp[ 7] = matrix[12];

    tmp[ 8] = matrix[10];
    tmp[ 9] = matrix[11];
    tmp[10] = matrix[ 8];
    tmp[11] = matrix[ 9];

    tmp[12] = matrix[ 7];
    tmp[13] = matrix[ 4];
    tmp[14] = matrix[ 5];
    tmp[15] = matrix[ 6];

    for (size_t i = 0; i < 16; i++) {
        matrix[i] = tmp[i];
    }
}

/*  This core implementation uses SIMD in order to generate the output
*   matrix. It uses the fact that every salsa20 quarter round modifies
*   the values on one diagonal depending on the two diagonals above it.
*   We reshuffle the matrix in a way such that each diagonal corresponds
*   to one row in the matrix. This approach allows us to load each row
*   (originally diagonal) conisisting of 4 uint32_t's into a seperate
*   m128i_u variable which are then used to do 4 simultaneous operations.
*   After each round the results are written back into the original
*   matrix which then gets rotated back to its original state and
*   transposed for the next round. Combining the rotation and tranposing
*   of the matrix after each round further reduces the amount of
*   operations needed.
*/
void salsa20_core_v1(uint32_t output[16], const uint32_t input[16]) {
    for (size_t i = 0 ; i < 16; i++) {
        output[i] = input[i];
    }

    // Rotate the matrix for SIMD
    rotate_simd(output);

    for (size_t i = 0; i < 20; i++) {
        __m128i_u* ptr = (__m128i_u*) output;

        // load each row into a seperate __m128i_u variable
        __m128i_u z0 = _mm_loadu_si128(ptr);
        __m128i_u z1 = _mm_loadu_si128(ptr + 1);
        __m128i_u z2 = _mm_loadu_si128(ptr + 2);
        __m128i_u z3 = _mm_loadu_si128(ptr + 3);

        __m128i_u tmp;

        /*  Each step adds the two rows above the one that has to be modified
        *   and rotates the result by a specific number of bits. The temporary
        *   value then gets xor'ed with the row that has to be modified.
        */
        // Row 1
        tmp = _mm_add_epi32(z3, z0);
        tmp = ROTL_SIMD(tmp, 7);
        z1 = _mm_xor_si128(z1, tmp);

        // Row 2
        tmp = _mm_add_epi32(z0, z1);
        tmp = ROTL_SIMD(tmp, 9);
        z2 = _mm_xor_si128(z2, tmp);

        // Row 3
        tmp = _mm_add_epi32(z1, z2);
        tmp = ROTL_SIMD(tmp, 13);
        z3 = _mm_xor_si128(z3, tmp);

        // Row 0
        tmp = _mm_add_epi32(z2, z3);
        tmp = ROTL_SIMD(tmp, 18);
        z0 = _mm_xor_si128(z0, tmp);

        // Write back rows into output matrix
        _mm_storeu_si128(ptr, z0);
        _mm_storeu_si128(ptr + 1, z1);
        _mm_storeu_si128(ptr + 2, z2);
        _mm_storeu_si128(ptr + 3, z3);

        // Custom transpose that combines the rotation and transposing of the matrix
        rotate_simd_transpose(output);
    }
    
    // Correct rotation of the diagonals
    rotate_simd_rev(output);


    for (size_t i = 0; i < 16; i++) {
        output[i] += input[i];
    }
}