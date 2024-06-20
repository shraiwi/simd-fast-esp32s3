#include "stdint.h"

#define SIMD_DATATYPES \
    SIMD_DATATYPE(u8x16, uint8_t) \
    SIMD_DATATYPE(s8x16, int8_t) \
    SIMD_DATATYPE(u16x8, uint16_t) \
    SIMD_DATATYPE(s16x8, int16_t) \
    SIMD_DATATYPE(u32x4, uint32_t) \
    SIMD_DATATYPE(s32x4, int32_t) \
    SIMD_DATATYPE(f32x4, float)

#define SIMD_DATATYPE(NAME, TYPE) typedef TYPE __attribute__ ((aligned (16))) simd_##NAME[16 / sizeof(TYPE)];
SIMD_DATATYPES
#undef SIMD_DATATYPE

#define SIMD_DATATYPE(NAME, TYPE) simd_##NAME NAME;
typedef union { SIMD_DATATYPES } simd_data_t;
#undef SIMD_DATATYPE