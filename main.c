#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>

#include "core_v0.h"
#include "core_v1.h"
#include "core_v2.h"
#include "core_v3.h"

#include "crypt_v0.h"
#include "crypt_v1.h"

#include "fileio.h"
#include "performance.h"
#include "verify.h"

const char* usage_msg =
    "Usage: %s [options] f  Encrypts text in f and writes it to output file\n"
    "   or: %s -h           Show help message and exit\n"
    "   or: %s --help       Show help message and exit\n";

const char* help_msg =
    "Positional arguments:\n"
    "   f   The file that contains the raw text that has to be en-/decrypted.\n"
    "\n"
    "Optional arguments:\n"
    "   -V N      The version of the salsa20 crypting algorithm (default: V7 (simd crypt with optimized simd core))\n"
    "   -B N      If set run performance test (N iterations) for the salsa20_crypt implementation (includes _core)\n"
    "   -k N      The secret key for the crypting algorithm (default: 0)\n"
    "   -i N      The initialised vector (default: 0)\n"
    "   -o F      The file that the encrypted message will be stored to (default: \"crypt.txt\")\n"
    "   -c        If -B was also set run performance test exclusively for the salsa20_core implementation\n"
    "   -h        Show help message (this text) and exit\n"
    "   --help    Show help message (this text) and exit\n"
    "   --verify  Run functional tests for all core and crypt implemenetations\n";

void print_usage(const char* progname) {
    fprintf(stderr, usage_msg, progname, progname, progname);
}

void print_help(const char* progname) {
    print_usage(progname);
    fprintf(stderr, "\n%s", help_msg);
}

typedef void(* core_func)(uint32_t[16], const uint32_t[16]);
typedef void(* crypt_func)(size_t, const uint8_t[], uint8_t[], uint32_t[8], uint64_t, core_func);

crypt_func getCryptImpl(uint32_t version) {
    switch (version) {
        case 0:
        case 1:
        case 2:
        case 3:
            return salsa20_crypt_v0;
        case 4:
        case 5:
        case 6:
        case 7:
            return salsa20_crypt_v1;
        default:
            return NULL;
    }
}

core_func getCoreImpl(uint32_t version) {
    switch (version) {
        case 0:
        case 4:
            return salsa20_core_v0;
        case 1:
        case 5:
            return salsa20_core_v1;
        case 2:
        case 6:
            return salsa20_core_v2;
        case 3:
        case 7:
            return salsa20_core_v3;
        default:
            return NULL;
    }
}

const char* version_descriptions[] = {
    "V0 (Crypt_v0: SISD; Core_v0: simple)",
    "V1 (Crypt_v0: SISD; Core_v1: SIMD)",
    "V2 (Crypt_v0: SISD; Core_v2: no transpose)",
    "V3 (Crypt_v0: SISD; Core_v3: optimized SIMD)",
    "V4 (Crypt_v1: SIMD; Core_v0: simple)",
    "V5 (Crypt_v1: SIMD; Core_v1: SIMD)",
    "V6 (Crypt_v1: SIMD; Core_v2: no transpose)",
    "V7 (Crypt_v1: SIMD; Core_v3: optimized SIMD)",
};

const char* getVersionDescription(uint32_t version) {
    if (version > 7) {
        return NULL;
    }

    return version_descriptions[version];
}

/*
*   clear256, muladd256 and parse256 are helper methods for the key option parsing.
*   clear256 is simply used to set every value in the key array to 0. muladd256 sets
*   updates the key array accordingly. parse256 checks wheter the given value is a
*   valid hex or dezimal number. parse256 uses the other helper methods to achieve this.
*
*   implementation inspired by https://stackoverflow.com/questions/48716046/getting-a-128-bits-integer-from-command-line
*/
void clear256(uint32_t quad[8]) {
    quad[0] = quad[1] = quad[2] = quad[3] = quad[4] = quad[5] = quad[6] = quad[7] = 0;
}

uint32_t muladd256(uint32_t quad[8], const uint32_t mul, const uint32_t add) {
    uint64_t  temp = 0;
    temp = (uint64_t)quad[0] * (uint64_t)mul + add;
    quad[0] = temp;

    temp = (uint64_t)quad[1] * (uint64_t)mul + (temp >> 32);
    quad[1] = temp;

    temp = (uint64_t)quad[2] * (uint64_t)mul + (temp >> 32);
    quad[2] = temp;

    temp = (uint64_t)quad[3] * (uint64_t)mul + (temp >> 32);
    quad[3] = temp;

    temp = (uint64_t)quad[4] * (uint64_t)mul + (temp >> 32);
    quad[4] = temp;

    temp = (uint64_t)quad[5] * (uint64_t)mul + (temp >> 32);
    quad[5] = temp;

    temp = (uint64_t)quad[6] * (uint64_t)mul + (temp >> 32);
    quad[6] = temp;

    temp = (uint64_t)quad[7] * (uint64_t)mul + (temp >> 32);
    quad[7] = temp;

    return temp >> 32;
}

char* parse256(uint32_t quad[8], char *from) {
    if (!from) {
        errno = EINVAL;
        return NULL;
    }

    while (*from == '\t' || *from == '\n' || *from == '\v' || *from == '\f' || *from == '\r') {
        from++;
    }

    if (from[0] == '0' && (from[1] == 'x' || from[1] == 'X') &&
        ((from[2] >= '0' && from[2] <= '9') ||
         (from[2] >= 'A' && from[2] <= 'F') ||
         (from[2] >= 'a' && from[2] <= 'f'))) {
        // Hexadecimal
        from += 2;
        clear256(quad);
        while (1) {
            if (*from >= '0' && *from <= '9') {
                if (muladd256(quad, 16, *from - '0')) {
                    errno = ERANGE;
                    return NULL;
                }
            } else if (*from >= 'A' && *from <= 'F') {
                if (muladd256(quad, 16, *from - 'A' + 10)) {
                    errno = ERANGE;
                    return NULL;
                }
            } else if (*from >= 'a' && *from <= 'f') {
                if (muladd256(quad, 16, *from - 'a' + 10)) {
                    errno = ERANGE;
                    return NULL;
                }
            } else if (*from == '\0') {
                return from;
	        } else {
		        errno = EINVAL;
                return NULL;
	        }
	        from++;
	    }
    }

    if (from[0] >= '0' && from[0] <= '9') {
        // Decimal
        clear256(quad);
        while (1) {
            if (*from >= '0' && *from <= '9') {
                if (muladd256(quad, 10, *from - '0')) {
                    errno = ERANGE;
                    return NULL;
                }
                from++;
            } else if (*from == '\0'){
		        return from;
	        } else {
		        errno = EINVAL;
                return NULL;
	        }
	    }
    }
    
    // Not a recognized number.
    errno = EINVAL;
    return NULL;
}

/*  The main is responsible for parsing the options and running the program
 *   according to the chosen implementation and input.
 */
int main(int argc, char** argv) {
    char* progname = argv[0];
    uint32_t key[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    uint64_t iter = 0;      // number of iterations for performance test
    uint8_t run_perf = 0;   // performance test flag
    uint8_t run_core = 0;   // core exclusive performance test flag 
    int failed = 0;

    uint64_t iv = 0;        // default nonce
    uint32_t version = 7;   // default version 7 (Crypt_v1: SIMD; Core_v3: optimized SIMD)
    char* in_path = NULL;
    char* out_path = "crypt.txt";   // default path for output file

    int opt;
    char* endptr;

    /*  Option parsing generally functions the same way as the one presented in tutorial in week 05
    *   with the small adaptation to also support long options.
    *
    *   source: https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
    */
    while (1) {
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"verify", no_argument, 0, 'v'},
 	        { NULL, 0, NULL, 0}
        };

        int option_index = 0;
        opt = getopt_long (argc, argv, "V:B:k:i:o:hcv", long_options, &option_index);

        // Break out of option parsing when getopt_long returns -1 which indicates that the end of the arguments in argv was reached.
        if (opt == -1)
            break;

        switch (opt) {
            case 'V':
                // Tries to convert the <int> argument of -V to a unsigned long. Exit on failure.
                errno = 0;
                endptr = NULL;
                version = strtoul(optarg, &endptr, 0);

                if (endptr == argv[optind] || *endptr != '\0') {
                    fprintf(stderr, "-V: %s could not be converted to a uint32_t\n", optarg);
                    return EXIT_FAILURE;
                } else if (errno == ERANGE) {
                    fprintf(stderr, "-V: %s over- or underflows uint32_t\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'B':
                // Tries to convert the <int> argument of -B to a unsinged long. Exit on failure.
                errno = 0;
                endptr = NULL;
                iter = strtoul(optarg, &endptr, 0);
                run_perf = 1;

                if (endptr == argv[optind] || *endptr != '\0') {
                    fprintf(stderr, "-B: %s could not be converted to a uint64_t\n", optarg);
                    return EXIT_FAILURE;
                } else if (errno == ERANGE) {
                    fprintf(stderr, "-B: %s over- or underflows uint64_t\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'k':
                // Tries to convert the <int> argument of -k to a 256 bit unsinged integer. Exit on failure.
 		        // Decimal and hexadecimal representation are allowed as <int> argument.
 	    	    endptr = parse256(key, optarg);
 		        if (!endptr) {
                    switch (errno) {
                 	    case ERANGE:
                            fprintf(stderr, "-k: %s over- or underflows custom uint256_t\n", optarg);
                     	    break;
                 	    case EINVAL:
                            fprintf(stderr, "-k: %s not a nonnegative integer in decimal or hexadecimal notation\n", optarg);
                            break;
                 	    default:
                            fprintf(stderr, "-k: %s %s.\n", optarg, strerror(errno));
                     	    break;
                    }
 		            return EXIT_FAILURE;
                }
                break;
            case 'i':
                // Tries to convert the <int> argument of -i into a unsinged long long. Exit on failure.
                errno = 0;
                endptr = NULL;
                iv = strtoull(optarg, &endptr, 0);

                if (endptr == argv[optind] || *endptr != '\0') {
                    fprintf(stderr, "-i: %s could not be converted to a uint64_t\n", optarg);
                    return EXIT_FAILURE;
                } else if (errno == ERANGE) {
                    fprintf(stderr, "-i: %s over- or underflows uint64_t\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'o':
                // Sets the path for the output file to the argument of -o.
                out_path = optarg;
                break;
            case 'c':
                run_core = 1;
                break;
 	        case 'v':
                if (verify_core()) {
                    failed++;
                }
 		            
                if (verify_crypt()) {
                    failed++;
                }
                    
                if (!failed) {
                    printf("All functional tests passed!\n");
                    return EXIT_SUCCESS;
                } else {
                    printf("At least one functional test failed!\n");
                    return EXIT_FAILURE;
                }
            case 'h':
                // Prints the help message to the console and exits.
                print_help(progname);
                return EXIT_SUCCESS;
            default:
                // Unrecognized option detected. Exiting.
                print_usage(progname);
                return EXIT_FAILURE;
             break;
        }
    }

    if (optind == argc) {
        printf("%s: Missing positional argument -- 'f'\n", progname);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Set the path of the input file to the positional argument in argv.
    in_path = argv[optind];

    // Depending on the parsed version choose the correct implementation for salsa20_core and salsa20_crypt.
    if (version > 7) {
        fprintf(stderr, "There is no implementation V%u for the salsa20/20 algorithm.\n", version);
        return EXIT_FAILURE;
    }

    core_func core_impl = getCoreImpl(version);
    crypt_func crypt_impl = getCryptImpl(version);
    const char* version_description = getVersionDescription(version);

    /*  Read contents of file from in_path and convert string to uint8_t array.
    *   Memory which was allocated by 'read_file' has to be freed.
    */
    struct FileText* filetext;

    /*  Tries to read contents from file. If the method fails to return a valid pointer
    *   then some operation failed and an apporpriate error message was printed to stderr.
    *   If the operation succeded then we get a pointer to a FileText struct which contains
    *   another dynamically allocated pointer to a string. In case we do not need the struct
    *   anymore both have to be freed (string first then struct).
    */
    if (!(filetext = read_file(in_path))) {
        return EXIT_FAILURE;
    }

    uint8_t* cipher;

    /*  Tries to allocate memory for the en-/decrypted text. If the operation fails the
    *   memory which was allocated for the FileText struct has to be freed.
    */
    if (!(cipher = malloc(filetext->len))) {
        fprintf(stderr, "Could not allocate enough memory for cipher text\n");
        free(filetext->str);
        free(filetext);
        return EXIT_FAILURE;
    }

    /*  If iter was modified then run performance tests else run salsa20/20 algorithm and
    *   write encrypted message to outputfile.
    */
    if (!run_perf) {
        // Call salsa20_crypt to encrypt the message
        crypt_impl(filetext->len, filetext->str, cipher, key, iv, core_impl);

        // If write_file returns a non zero value, then writing to the file failed. In this case return EXIT_FAILURE.
        if (write_file(out_path, cipher, filetext->len) != 0) {
            free(filetext->str);
            free(filetext);
            free(cipher);
            return EXIT_FAILURE;
        }
    } else {
        if (run_core) {
            uint32_t diag[4] = { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };
            uint32_t* iv_ptr = (uint32_t*) &iv;
            uint32_t iv32_0 = *iv_ptr;
            uint32_t iv32_1 = *(iv_ptr + 1);

            uint32_t c32_0 = 0;
            uint32_t c32_1 = 0;

            uint32_t input[16] = {
                diag[0], key[0], key[1], key[2],
                key[3], diag[1], iv32_0, iv32_1,
                c32_0, c32_1, diag[2], key[4],
                key[5], key[6], key[7], diag[3]
            };
 	        uint32_t output[16];

            const char* core_descriptions[4] = {
                "Core_v0 (simple)",
                "Core_v1 (SIMD)",
                "Core_v2 (no transpose)",
                "Core_v3 (optimized SIMD)",
            };

            // Run performance test for core implementation
            performance_core(iter, core_impl, output, input, core_descriptions[version%4]);
        } else {

            // Run performance test for crypt implementation
            performance(iter, crypt_impl, core_impl, filetext->len, filetext->str, cipher, key, iv, version_description);
        }
    }

    free(filetext->str);
    free(filetext);
    free(cipher);
    return EXIT_SUCCESS;
}
