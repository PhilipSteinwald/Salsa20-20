#ifndef FILE_IO_H
#define FILE_IO_H

struct FileText {
    size_t len;
    uint8_t* str;
};

struct FileText* read_file(const char* path);

int write_file(const char* path, const uint8_t* string, const size_t len);

#endif
