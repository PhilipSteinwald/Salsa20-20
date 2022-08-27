#include <aio.h>
#include <stdint.h>
#include <emmintrin.h>

typedef void (*core_func)(uint32_t[16], const uint32_t[16]);

/*  This crypt implementation uses SIMD to encrypt the message. It generates
*   64 crypting bytes with the given core_func which are then used to encrypt
*   the message bytes. The encrypting is done in 16 byte blocks by loading
*   16 bytes of the message and 16 bytes of a salsa20 block into _m128i_u
*   variables which are then xor'ed to create the encrypted message. Whenever
*   all 64 encrypting bytes are used up a new salsa20 block is generated after
*   incrementing the counter which ensures a distinct block is created.
*/
void salsa20_crypt_v1(size_t mlen, const uint8_t msg[mlen], uint8_t cipher[mlen], uint32_t key[8], uint64_t iv, core_func core) {
    // Salsa20 counter variable gets initialized as a uint64 for easier incrementation
    uint64_t counter = 0;

    // Constant on the diagonal
    uint32_t diag[4] = { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };

    // Cast 64bit iv to 32bit pointer to access the two different uint32's
    uint32_t* iv_ptr = (uint32_t*) &iv;
    uint32_t iv0 = *iv_ptr;
    uint32_t iv1 = *(iv_ptr + 1);

    // Pointers for current position in msg and cipher text.
    uint8_t* msg_ptr = (uint8_t*) msg;
    uint8_t* cip_ptr = (uint8_t*) cipher;

    // Seperate index variable to access bytes when we will encrypt the residual bytes that can not fit into _m128 variables
    size_t cur_index = 0;

    while (cur_index < mlen) {
        // Cast counter to uint32 to access the two fields
        uint32_t* c_ptr = (uint32_t*) &counter;
        uint32_t c0 = *c_ptr;
        uint32_t c1 = *(c_ptr + 1);

        // Create intial input matrix
        uint32_t input[16] = {
            diag[0], key[0], key[1], key[2],
            key[3], diag[1], iv0, iv1,
            c0, c1, diag[2], key[4],
            key[5], key[6], key[7], diag[3]
        };

        // Apply salsa20 core to input matrix
        uint32_t output[16];
        core(output, input);

        // Cast output matrix to uint8 array which acts as a counter for current position in output matrix
        uint8_t* key_byte_stream = (uint8_t*) output;
        size_t i = 0;

        /*  Each salsa20 core generates 64 key bytes that can be used for encryption. When
        *   encrypting 16 bytes at once via SIMD we can iterate at most 4 times over the
        *   output matrix to use up all of those bytes for encryption.
        */
        while (i < 4) {
            /*  We must never read over the bounds of the msg/cipher array. That's why we check
            *   if the next 16 bytes are within the array bounds. If that is the case then we can
            *   use SIMD to encrypt 16 bytes at once else we have to encrypt the residual bytes
            *   via SISD.
            */
            if (cur_index + 15 < mlen) {
                // Load 16 bytes from msg and key stream to xor them and store the result in cipher
                __m128i_u msg_vec = _mm_loadu_si128((__m128i_u*) msg_ptr);
                __m128i_u key_stream_vec = _mm_loadu_si128((__m128i_u*) key_byte_stream);
                _mm_storeu_si128((__m128i_u*) cip_ptr, _mm_xor_si128(msg_vec, key_stream_vec));

                // Increment all the pointers/variables by 16 because we used up 16 bytes for encryption
                key_byte_stream += 16;
                msg_ptr += 16;
                cip_ptr += 16;
                cur_index += 16;
                i++;
            } else {
                // SISD for residual bytes
                while (cur_index < mlen) {
                    cipher[cur_index] = msg[cur_index] ^ *key_byte_stream;
                    key_byte_stream++;
                    cur_index++;
                }
                // Break out of loop even if 'i < 4' holds because we have dealt with the last few bytes of the msg
                break;
            }
        }

        // Incrementing the counter variable ensures that salsa20_core generates a different matrix
        counter++;
    }
}
