#include "stddef.h"

#include "simd_types.h"

typedef struct { uint16_t x, y, score; } simd_fast_point_t;

void simd_fast_candidates(const simd_u8x16 pixels, int32_t rowstride, int32_t b, simd_u8x16 out);

// compute the score of a pixel ring using SAD
int32_t simd_fast_score(const simd_u8x16 ring, uint32_t center);

simd_fast_point_t * simd_fast12_detect(const uint8_t * pixels, int32_t w, int32_t h, int32_t stride, uint32_t b, size_t * ret_num_corners);
simd_fast_point_t * simd_fast12_detect_nonmax(const uint8_t * pixels, int32_t w, int32_t h, int32_t stride, uint32_t b, size_t * ret_num_corners);
