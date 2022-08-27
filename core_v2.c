#include <aio.h>
#include <stdint.h>

#include "mtr_util.h"

#define SALSA_QROUND(a, b, c, d)(   \
  b ^= ROTATELEFT(a + d, 7),        \
  c ^= ROTATELEFT(b + a, 9),        \
  d ^= ROTATELEFT(c + b, 13),       \
  a ^= ROTATELEFT(d + c, 18))

/*
*   This core implementation uses SISD. It is different from v0 in that it
*   doesn't transpose after every iteration. Instead, the positions of the
*   elements are fixed. In the base implementation v0
*   the matrix is transposed after four quarter-rounds (1 full round).
*   As this would happen twice in here, we don't need to transpose at all,
*   as transposing twice yields the original matrix. This is still valid,
*   as the positions of the next four quarter rounds are adjusted accordingly.
*
*   The quarter round (macro) works by simply reordering the
*   same expressions as v0.
*/
void salsa20_core_v2(uint32_t output[16], const uint32_t input[16]) {
    for (size_t i = 0; i < 16; i++) {
        output[i] = input[i];
    }

    for (size_t i = 0; i < 10; i++) {
        SALSA_QROUND(output[ 0], output[ 4], output[ 8], output[12]);
        SALSA_QROUND(output[ 5], output[ 9], output[13], output[ 1]);
        SALSA_QROUND(output[10], output[14], output[ 2], output[ 6]);
        SALSA_QROUND(output[15], output[ 3], output[ 7], output[11]);

        SALSA_QROUND(output[ 0], output[ 1], output[ 2], output[ 3]);
        SALSA_QROUND(output[ 5], output[ 6], output[ 7], output[ 4]);
        SALSA_QROUND(output[10], output[11], output[ 8], output[ 9]);
        SALSA_QROUND(output[15], output[12], output[13], output[14]);
    }

    for (size_t i = 0; i < 16; i++) {
        output[i] += input[i];
    }
}
