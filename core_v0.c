#include <aio.h>
#include <stdint.h>

#include "mtr_util.h"

/* This core implementation uses SISD to generate the output
*  matrix. It works by executing the same 4 steps 20 times and
*  transposing the matrix each time afterwards.
*/
void salsa20_core_v0(uint32_t output[16], const uint32_t input[16]){
    // copy of all values from input to output;
    for (size_t i = 0; i < 16; i++) {
        output[i] = input[i];
    }

    /*  the 4x4 matrix is represtented by the output array in the following way:
    *   i\j      1:          2:        3:          4:
    *   1:  / output[0]  output[1]  output[2]  output[3]   \
    *   2:  | output[4]  output[5]  output[6]  output[7]   |
    *   3:  | output[8]  output[9]  output[10] output[11]  |
    *   4:  \ output[12] output[13] output[14]  output[15] /
    */
    for(size_t i = 0; i < 20; i++){
        //step1
        output[ 4] ^= ROTATELEFT(output[12] + output[ 0], 7);
        output[ 9] ^= ROTATELEFT(output[ 1] + output[ 5], 7);
        output[14] ^= ROTATELEFT(output[ 6] + output[10], 7);
        output[ 3] ^= ROTATELEFT(output[11] + output[15], 7);

        //step2
        output[ 8] ^= ROTATELEFT(output[ 0] + output[ 4], 9);
        output[13] ^= ROTATELEFT(output[ 5] + output[ 9], 9);
        output[ 2] ^= ROTATELEFT(output[10] + output[14], 9);
        output[ 7] ^= ROTATELEFT(output[15] + output[ 3], 9);

        //step3
        output[12] ^= ROTATELEFT(output[ 4] + output[ 8], 13);
        output[ 1] ^= ROTATELEFT(output[ 9] + output[13], 13);
        output[ 6] ^= ROTATELEFT(output[14] + output[ 2], 13);
        output[11] ^= ROTATELEFT(output[ 3] + output[ 7], 13);

        //step4
        output[ 0] ^= ROTATELEFT(output[ 8] + output[12], 18);
        output[ 5] ^= ROTATELEFT(output[13] + output[ 1], 18);
        output[10] ^= ROTATELEFT(output[ 2] + output[ 6], 18);
        output[15] ^= ROTATELEFT(output[ 7] + output[11], 18);

        transpose(output);
    }

    // final step: adding input matrix with the changed matrix
    //             to get the final output matrix
    for(size_t i = 0; i < 16; i++){
        output[i] += input[i];
    }
}
