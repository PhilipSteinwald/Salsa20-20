#ifndef MTR_UTIL_H
#define MTR_UTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <emmintrin.h>

// Rotates 32 bit integer (a) by (b) number of bits
#define ROTATELEFT(a, b) (((a) << (b) | ((a) >> (32 - (b)))))

// Rotates 4 32 bit integers in a 128 bit variable (a) by (b) number of bits each
#define ROTL_SIMD(a, b) (_mm_or_si128(_mm_slli_epi32((a), (b)), _mm_srli_epi32((a), (32 - (b)))))

void transpose();

void rotate_simd(uint32_t matrix[16]);

void rotate_simd_rev (uint32_t matrix[16]);

int mtr_equal(uint32_t m1[16], uint32_t m2[16]);

void print_matrix(uint32_t matrix[16]);

#endif
