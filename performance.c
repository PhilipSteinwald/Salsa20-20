#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef void (*core_func)(uint32_t[16], const uint32_t[16]);

typedef void (*crypt_func)(size_t mlen, const uint8_t[mlen], uint8_t[mlen], uint32_t[8], uint64_t, core_func);

/*
* The performance tests are implemented according to the Benchmarking video in Week 7
*/

void performance_core(uint64_t iter, core_func f, uint32_t output[16], const uint32_t input[16], const char* fname) {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint64_t i = 0; i < iter; i++) {
        f(output, input);
    }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);

    printf("%s took %f seconds to complete %ld iterations.\n", fname, time, iter);
}

void performance(uint64_t iter, crypt_func crypt, core_func core, size_t mlen, const uint8_t msg[mlen], uint8_t cipher[mlen], uint32_t key[8], uint64_t iv, const char* fname){
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint64_t i = 0; i < iter; i++) {
        crypt(mlen,msg,cipher,key,iv,core);
    }
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    double time = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
    double avg = time / iter;

    printf("%s took %f seconds to complete %ld iterations.\n", fname, time, iter);
    printf("Encryption of message (%lu bytes) took %f seconds on average.\n", mlen, avg);
}
