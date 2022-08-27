#include <stdint.h>
#include <emmintrin.h>

#include "mtr_util.h"

/*
*  Structure adapted from v1.
*  We once again use row and column rounds to minimize loop iterations
*  and the need for tranposing.
*  Instead of actually tranposing, we mae use of the fact that every row/column
*  of entries only interacts with itself. This means that after a row round,
*  we can simply reinterpret the rows as column by switching the order of the
*  entries within one row. This allows us to use SIMD for both row and column rounds.
*  Since we only need to write back into the original array at the end, the performance
*  is greatly improved. (A more thorough explanation (incl. examples) can be found in the paper)
*/
void salsa20_core_v3(uint32_t output[16], const uint32_t input[16]) {
    for (size_t i = 0; i < 16; i++) {
        output[i] = input[i];
    }

    rotate_simd(output);
    __m128i_u* r_ptr = (__m128i_u*) output;

    __m128i_u r0 = _mm_loadu_si128(r_ptr);
    __m128i_u r1 = _mm_loadu_si128(r_ptr + 1);
    __m128i_u r2 = _mm_loadu_si128(r_ptr + 2);
    __m128i_u r3 = _mm_loadu_si128(r_ptr + 3);

    __m128i_u tmp;

    for (size_t i = 0; i < 10; i++) {
        // rows
        tmp = _mm_add_epi32(r3, r0);
        tmp = ROTL_SIMD(tmp, 7);
        r1 = _mm_xor_si128(r1, tmp);

        tmp = _mm_add_epi32(r0, r1);
        tmp = ROTL_SIMD(tmp, 9);
        r2 = _mm_xor_si128(r2, tmp);

        tmp = _mm_add_epi32(r1, r2);
        tmp = ROTL_SIMD(tmp, 13);
        r3 = _mm_xor_si128(r3, tmp);

        tmp = _mm_add_epi32(r2, r3);
        tmp = ROTL_SIMD(tmp, 18);
        r0 = _mm_xor_si128(r0, tmp);

        // pseudo-transpose
        // r0 stays the same
        r1 = _mm_shuffle_epi32(r1, 0x93);   // reinterprete as r3
        r2 = _mm_shuffle_epi32(r2, 0x4E);   // stays r2
        r3 = _mm_shuffle_epi32(r3, 0x39);   // reinterprete as r1

        // columns
        tmp = _mm_add_epi32(r1, r0);
        tmp = ROTL_SIMD(tmp, 7);
        r3 = _mm_xor_si128(r3, tmp);

        tmp = _mm_add_epi32(r0, r3);
        tmp = ROTL_SIMD(tmp, 9);
        r2 = _mm_xor_si128(r2, tmp);

        tmp = _mm_add_epi32(r3, r2);
        tmp = ROTL_SIMD(tmp, 13);
        r1 = _mm_xor_si128(r1, tmp);

        tmp = _mm_add_epi32(r2, r1);
        tmp = ROTL_SIMD(tmp, 18);
        r0 = _mm_xor_si128(r0, tmp);

        // pseudo-re-transpose
        r1 = _mm_shuffle_epi32(r1, 0x39);
        r2 = _mm_shuffle_epi32(r2, 0x4E);
        r3 = _mm_shuffle_epi32(r3, 0x93);
    }

    _mm_storeu_si128(r_ptr, r0);
    _mm_storeu_si128(r_ptr + 1, r1);
    _mm_storeu_si128(r_ptr + 2, r2);
    _mm_storeu_si128(r_ptr + 3, r3);

    rotate_simd_rev(output);

    for (size_t i = 0; i < 16; i++) {
        output[i] += input[i];
    }
}
