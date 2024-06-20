#include "stddef.h"

#include "simd_types.h"

typedef struct { uint16_t x, y; int32_t score; } simd_fast_point_t;

void simd_fast_candidates(const simd_u8x16 pixels, uint32_t width, uint32_t b, simd_u8x16 out);
void simd_fast_candidates2(const simd_u8x16 pixels, int32_t width, int32_t b, simd_u8x16 out);
int32_t simd_fast_compute(uint32_t center, simd_u8x16 ring, uint32_t b);
uint32_t simd_fast_compute2(const simd_u8x16 centers, const simd_u8x16 rings[16], uint32_t b, simd_u8x16 scores);
//int32_t simd_fast_compute3(uint32_t center, const simd_u8x16 ring, uint32_t b);
int32_t simd_fast_compute3(uint32_t center, const simd_u8x16 pixels, uint32_t b, simd_u8x16 debug[8]);

// compute the score of a pixel ring using SAD
int32_t simd_fast_score(const simd_u8x16 ring, uint32_t center);

simd_fast_point_t * simd_fast12_detect(const uint8_t * pixels, int32_t w, int32_t h, uint32_t b, size_t * ret_num_corners);