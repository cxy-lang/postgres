#ifndef POSTGRES_TYPES_H
#define POSTGRES_TYPES_H

#include <stdint.h>

typedef union Float4Conv {
    float f;
    uint32_t u;
} Float4Conv;

typedef union Float8Conv {
    double d;
    uint64_t ull64;
    int64_t ll64;
    struct {
        uint32_t u32_1;
        uint32_t u32_2;
    };
} Float8Conv;

#endif // POSTGRES_TYPES_H
