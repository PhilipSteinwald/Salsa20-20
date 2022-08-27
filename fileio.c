#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

struct FileText {
    size_t len;
    uint8_t* str;
};

/*  General structure was taken from the tutrial in week 07.
*   We have adapted the function to read the contents of the file into a struct
*   which contains an array for the string and a field for the number of bytes in
*   that array. This is done because the process of encrypting a message may generate
*   '\0' bytes which would lead to the encrypted char* to be a shorter message than the
*   original string. This would make it impossible to recover the original message by
*   applying the same algorithm to the encrypted message.
*
*   Important: On success the function returns a pointer to a dynamically allocated struct
*   which also contains a dynamically allocated array to to the string. In case the struct
*   is no longer needed the allocated memory has to be freed in the correct order:
*       - First free struct.str
*       - Then free struct itself
*/
struct FileText* read_file(const char* path) {
    FILE* file = NULL;
    struct stat statbuf;

    if (!(file = fopen(path, "r"))) {
        fprintf(stderr, "Error opening file, no such file: %s\n", path);
        return NULL;
    }

    if (fstat(fileno(file), &statbuf)) {
        fprintf(stderr, "Error retrieving file stats: %s\n", path);
        fclose(file);
        return NULL;
    }

    if (!S_ISREG(statbuf.st_mode) || statbuf.st_size <= 0) {
        fprintf(stderr, "Not a regular file or invalide size for file: %s\n", path);
        fclose(file);
        return NULL;
    }

    struct FileText* msg = NULL;

    if (!(msg = malloc(sizeof(struct FileText)))) {
        fprintf(stderr, "Could not allocate enough memory for struct: %s\n", path);
        fclose(file);
        return NULL;
    }

    msg->len = statbuf.st_size;
    msg->str = NULL;

    if (!(msg->str = malloc(statbuf.st_size))) {
        fprintf(stderr, "Could not allocate enough memory for raw text: %s\n", path);
        free(msg);
        fclose(file);
        return NULL;
    }

    if (!fread(msg->str, 1, statbuf.st_size, file)) {
        fprintf(stderr, "Error reading contents from file: %s\n", path);
        free(msg->str);
        free(msg);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return msg;
}

/*
*   Structure was once again taken from tutorial 7.
*   Given a path and a string of a certain length,
*   we try to write the string to the file with the given path.
*   If the file does not exist, it is created. If for some reason,
*   the file path is corrupted or we do not have correct access rights,
*   the process is aborted and file closed.
*
*   Author: Liv Märtens
*/
int write_file(const char* path, const uint8_t* string, const size_t len) {
    int suc = EXIT_SUCCESS;
    FILE* file;

    if (!(file = fopen(path, "w"))) {
        fprintf(stderr, "Error opening file: %s\n", path);
        return EXIT_FAILURE;
    }

    if (!fwrite(string, 1, len, file)) {
        fprintf(stderr, "Error writing to file: %s\n", path);
        suc = EXIT_FAILURE;
    }

    fclose(file);
    return suc;
}
