#include "simd_fast.h"

#include "string.h"
#include "stdio.h"
#include "stdbool.h"

#include "esp_log.h"

#include "fast.h"

static const char * TAG = "simd_fast";
static const size_t INITIAL_CAPACITY = 256;

#define min(x, y) (x)<(y)?(x):(y)
#define max(x, y) (x)>(y)?(x):(y)


static inline bool simd_fast_12_is_corner(const uint8_t * p, const int32_t * offs, int32_t b);
static inline bool simd_fast_12_is_corner_ring(const uint8_t * p, const int32_t * offs, int32_t b, simd_u8x16 ring);

void simd_fast_compute_offs(int32_t * offs, int32_t stride) {
    offs[0] = 0 + stride * 3;
    offs[1] = 1 + stride * 3;
    offs[2] = 2 + stride * 2;
    offs[3] = 3 + stride * 1;
    offs[4] = 3 + stride * 0;
    offs[5] = 3 + stride * -1;
    offs[6] = 2 + stride * -2;
    offs[7] = 1 + stride * -3;
    offs[8] = 0 + stride * -3;
    offs[9] = -1 + stride * -3;
    offs[10] = -2 + stride * -2;
    offs[11] = -3 + stride * -1;
    offs[12] = -3 + stride * 0;
    offs[13] = -3 + stride * 1;
    offs[14] = -2 + stride * 2;
    offs[15] = -1 + stride * 3;
}

simd_fast_point_t * simd_fast12_detect(const uint8_t * pixels, int32_t w, int32_t h, int32_t stride, uint32_t b, size_t * ret_num_corners) { 
    int32_t offs[16];
    simd_fast_compute_offs(offs, w);

    size_t raw_points_capacity = INITIAL_CAPACITY;

    simd_fast_point_t * raw_points = malloc(sizeof(simd_fast_point_t) * raw_points_capacity);
    (*ret_num_corners) = 0;

    simd_u8x16 candidates, ring;

    for (int32_t y = 3; y < h - 3; y++) {
        for (int32_t x = 0; x < w; x += 16) {
            const uint8_t * p = pixels + x + y * stride;
            simd_fast_candidates(p, stride, b, candidates);

            int32_t start = max(3 - x, 0);
            int32_t end = min(w - 3 - x, 16);

            for (int32_t i = start; i < end; i++) {
                if (candidates[i] > 2 
                    && simd_fast_12_is_corner_ring(p + i, offs, b, ring)) {

                    simd_fast_point_t * pt = &raw_points[*ret_num_corners];
                    
                    pt->x = x+i;
                    pt->y = y;
                    pt->score = (uint16_t)simd_fast_score(ring, p[i]);

                    if (++(*ret_num_corners) == raw_points_capacity) {
                        raw_points_capacity *= 2;
                        raw_points = realloc(raw_points, sizeof(raw_points[0]) * raw_points_capacity);
                    }
                }
            }
        }
    }

    return raw_points;
}

simd_fast_point_t * simd_fast12_detect_nonmax(const uint8_t * pixels, int32_t w, int32_t h, int32_t stride, uint32_t b, size_t * ret_num_corners) {
    int32_t offs[16];
    simd_fast_compute_offs(offs, stride);

    size_t raw_points_capacity = INITIAL_CAPACITY;

    simd_fast_point_t * raw_points = malloc(sizeof(simd_fast_point_t) * raw_points_capacity);
    size_t raw_points_len = 0;

    simd_u8x16 candidates, ring;

    int32_t row_starts[h];

    for (int32_t y = 3; y < h - 3; y++) {
        row_starts[y] = -1;
        for (int32_t x = 0; x < w; x += 16) {
            const uint8_t * p = pixels + x + y * stride;
            simd_fast_candidates(p, stride, b, candidates);

            int32_t start = max(3 - x, 0);
            int32_t end = min(w - 3 - x, 16);

            for (int32_t i = start; i < end; i++) {
                if (candidates[i] > 2 
                    && simd_fast_12_is_corner_ring(p + i, offs, b, ring)) {

                    simd_fast_point_t * pt = &raw_points[raw_points_len];
                    
                    pt->x = x+i;
                    pt->y = y;
                    pt->score = (uint16_t)simd_fast_score(ring, p[i]);
                    if (row_starts[y] == -1) row_starts[y] = raw_points_len;

                    if (++raw_points_len == raw_points_capacity) {
                        raw_points_capacity *= 2;
                        raw_points = realloc(raw_points, sizeof(raw_points[0]) * raw_points_capacity);
                    }
                }
            }
        }
    }

    size_t idx_above = 0, idx_below = 0;
    int32_t last_row = raw_points[raw_points_len-1].y;

    *ret_num_corners = 0;
    simd_fast_point_t * nonmax_points = malloc(sizeof(simd_fast_point_t) * raw_points_len);

    for (size_t i = 0; i < raw_points_len; i++) {
        simd_fast_point_t * pt = &raw_points[i];

        // dont include the point if left point has higher score
        if (i > 0
            && raw_points[i-1].x == pt->x - 1 
            && raw_points[i-1].y == pt->y 
            && raw_points[i-1].score >= pt->score) continue;
        
        // dont include the point if right point has a higher score
        if (i < (raw_points_len - 1)
            && raw_points[i + 1].x == pt->x + 1 
            && raw_points[i + 1].y == pt->y
            && raw_points[i + 1].score >= pt->score) continue;
        
        // if there is a point above the target
        if (pt->y != 0 && row_starts[pt->y - 1] != -1) {
            if (raw_points[idx_above].y < pt->y - 1) 
                idx_above = row_starts[pt->y - 1];

            // find next point that's above the current point
            for (;
                raw_points[idx_above].y < pt->y 
                && raw_points[idx_above].x < pt->x - 1; 
                idx_above++) {}

            for (size_t j = idx_above; raw_points[j].y < pt->y && raw_points[j].x <= pt->x + 1; j++) {
                uint16_t x = raw_points[j].x;
                if ((x == pt->x - 1 || x == pt->x || x == pt->x + 1)
                    && raw_points[j].score >= pt->score) goto skip_point;
            }
        }

        // if there is a point below the target
        if (pt->y != last_row && row_starts[pt->y + 1] != -1 && idx_below < raw_points_len) {
            if (raw_points[idx_below].y < pt->y + 1)
                idx_below = row_starts[pt->y + 1];

            // find next point that's directly below the current point
            for (; 
                idx_below < raw_points_len 
                && raw_points[idx_below].y == pt->y + 1 
                && raw_points[idx_below].x < pt->x - 1; 
                idx_below++) {}

            for (size_t j = idx_below; 
                j < raw_points_len 
                && raw_points[j].y == pt->y + 1 
                && raw_points[j].x <= pt->x + 1; 
                j++) {
                uint16_t x = raw_points[j].x;
                if ((x == pt->x - 1 || x == pt->x || x == pt->x + 1)
                    && raw_points[j].score >= pt->score) goto skip_point;
            }
        }

        nonmax_points[(*ret_num_corners)++] = raw_points[i];
        skip_point:
    }

    free(raw_points);

    return nonmax_points;
}

static inline bool simd_fast_12_is_corner_ring(const uint8_t * p, const int32_t * offs, int32_t b, simd_u8x16 ring) {		
	int32_t thresh_h = *p + b;
	int32_t thresh_l = *p - b;
    if((ring[0] = p[offs[0]]) > thresh_h)
        if((ring[1] = p[offs[1]]) > thresh_h)
        if((ring[2] = p[offs[2]]) > thresh_h)
        if((ring[3] = p[offs[3]]) > thresh_h)
        if((ring[4] = p[offs[4]]) > thresh_h)
            if((ring[5] = p[offs[5]]) > thresh_h)
            if((ring[6] = p[offs[6]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
                if((ring[9] = p[offs[9]]) > thresh_h)
                if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                {}
                else
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
        else if((ring[4] = p[offs[4]]) < thresh_l)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[5] = p[offs[5]]) < thresh_l)
            if((ring[6] = p[offs[6]]) < thresh_l)
            if((ring[7] = p[offs[7]]) < thresh_l)
                if((ring[9] = p[offs[9]]) < thresh_l)
                if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                    if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if((ring[3] = p[offs[3]]) < thresh_l)
        if((ring[15] = p[offs[15]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[4] = p[offs[4]]) < thresh_l)
            if((ring[5] = p[offs[5]]) < thresh_l)
            if((ring[6] = p[offs[6]]) < thresh_l)
                if((ring[8] = p[offs[8]]) < thresh_l)
                if((ring[9] = p[offs[9]]) < thresh_l)
                if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                    if((ring[12] = p[offs[12]]) < thresh_l)
                    if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            if((ring[4] = p[offs[4]]) < thresh_l)
            if((ring[5] = p[offs[5]]) < thresh_l)
            if((ring[6] = p[offs[6]]) < thresh_l)
            if((ring[7] = p[offs[7]]) < thresh_l)
                if((ring[8] = p[offs[8]]) < thresh_l)
                if((ring[9] = p[offs[9]]) < thresh_l)
                if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                    if((ring[12] = p[offs[12]]) < thresh_l)
                    if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
        if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[4] = p[offs[4]]) < thresh_l)
            if((ring[5] = p[offs[5]]) < thresh_l)
            if((ring[6] = p[offs[6]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
                if((ring[9] = p[offs[9]]) < thresh_l)
                if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                    if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if((ring[2] = p[offs[2]]) < thresh_l)
        if((ring[6] = p[offs[6]]) > thresh_h)
        if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    if((ring[4] = p[offs[4]]) > thresh_h)
                    if((ring[5] = p[offs[5]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if((ring[6] = p[offs[6]]) < thresh_l)
        if((ring[4] = p[offs[4]]) < thresh_l)
            if((ring[5] = p[offs[5]]) < thresh_l)
            if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
                if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    {}
                    else
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        if((ring[6] = p[offs[6]]) > thresh_h)
        if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    if((ring[4] = p[offs[4]]) > thresh_h)
                    if((ring[5] = p[offs[5]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if((ring[6] = p[offs[6]]) < thresh_l)
        if((ring[4] = p[offs[4]]) < thresh_l)
            if((ring[5] = p[offs[5]]) < thresh_l)
            if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
                if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    {}
                    else
                    if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if((ring[1] = p[offs[1]]) < thresh_l)
        if((ring[5] = p[offs[5]]) > thresh_h)
        if((ring[6] = p[offs[6]]) > thresh_h)
        if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    if((ring[4] = p[offs[4]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if((ring[2] = p[offs[2]]) > thresh_h)
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    if((ring[4] = p[offs[4]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if((ring[5] = p[offs[5]]) < thresh_l)
        if((ring[4] = p[offs[4]]) < thresh_l)
        if((ring[6] = p[offs[6]]) < thresh_l)
            if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[3] = p[offs[3]]) < thresh_l)
                if((ring[2] = p[offs[2]]) < thresh_l)
                    {}
                else
                    if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else
        if((ring[5] = p[offs[5]]) > thresh_h)
        if((ring[6] = p[offs[6]]) > thresh_h)
        if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                else
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    if((ring[4] = p[offs[4]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if((ring[2] = p[offs[2]]) > thresh_h)
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    if((ring[4] = p[offs[4]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if((ring[5] = p[offs[5]]) < thresh_l)
        if((ring[4] = p[offs[4]]) < thresh_l)
        if((ring[6] = p[offs[6]]) < thresh_l)
            if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[3] = p[offs[3]]) < thresh_l)
                    if((ring[2] = p[offs[2]]) < thresh_l)
                    {}
                    else
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    {}
                    else
                    return false;
                else
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
    else if((ring[0] = p[offs[0]]) < thresh_l)
        if((ring[1] = p[offs[1]]) > thresh_h)
        if((ring[5] = p[offs[5]]) > thresh_h)
        if((ring[4] = p[offs[4]]) > thresh_h)
        if((ring[6] = p[offs[6]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[3] = p[offs[3]]) > thresh_h)
                if((ring[2] = p[offs[2]]) > thresh_h)
                    {}
                else
                    if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if((ring[5] = p[offs[5]]) < thresh_l)
        if((ring[6] = p[offs[6]]) < thresh_l)
        if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    if((ring[4] = p[offs[4]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if((ring[2] = p[offs[2]]) < thresh_l)
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    if((ring[4] = p[offs[4]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else if((ring[1] = p[offs[1]]) < thresh_l)
        if((ring[2] = p[offs[2]]) > thresh_h)
        if((ring[6] = p[offs[6]]) > thresh_h)
        if((ring[4] = p[offs[4]]) > thresh_h)
            if((ring[5] = p[offs[5]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
                if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    {}
                    else
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if((ring[6] = p[offs[6]]) < thresh_l)
        if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    if((ring[4] = p[offs[4]]) < thresh_l)
                    if((ring[5] = p[offs[5]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if((ring[2] = p[offs[2]]) < thresh_l)
        if((ring[3] = p[offs[3]]) > thresh_h)
        if((ring[15] = p[offs[15]]) < thresh_l)
            if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[4] = p[offs[4]]) > thresh_h)
            if((ring[5] = p[offs[5]]) > thresh_h)
            if((ring[6] = p[offs[6]]) > thresh_h)
                if((ring[8] = p[offs[8]]) > thresh_h)
                if((ring[9] = p[offs[9]]) > thresh_h)
                if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                    if((ring[12] = p[offs[12]]) > thresh_h)
                    if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            if((ring[4] = p[offs[4]]) > thresh_h)
            if((ring[5] = p[offs[5]]) > thresh_h)
            if((ring[6] = p[offs[6]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
                if((ring[8] = p[offs[8]]) > thresh_h)
                if((ring[9] = p[offs[9]]) > thresh_h)
                if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                    if((ring[12] = p[offs[12]]) > thresh_h)
                    if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if((ring[3] = p[offs[3]]) < thresh_l)
        if((ring[4] = p[offs[4]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[5] = p[offs[5]]) > thresh_h)
            if((ring[6] = p[offs[6]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
                if((ring[9] = p[offs[9]]) > thresh_h)
                if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                    if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if((ring[4] = p[offs[4]]) < thresh_l)
            if((ring[5] = p[offs[5]]) < thresh_l)
            if((ring[6] = p[offs[6]]) < thresh_l)
            if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
                if((ring[9] = p[offs[9]]) < thresh_l)
                if((ring[10] = p[offs[10]]) < thresh_l)
                if((ring[11] = p[offs[11]]) < thresh_l)
                {}
                else
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
        else
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
        if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[4] = p[offs[4]]) > thresh_h)
            if((ring[5] = p[offs[5]]) > thresh_h)
            if((ring[6] = p[offs[6]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
                if((ring[9] = p[offs[9]]) > thresh_h)
                if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                    if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        if((ring[6] = p[offs[6]]) > thresh_h)
        if((ring[4] = p[offs[4]]) > thresh_h)
            if((ring[5] = p[offs[5]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
                if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    if((ring[3] = p[offs[3]]) > thresh_h)
                    {}
                    else
                    if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if((ring[6] = p[offs[6]]) < thresh_l)
        if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    if((ring[4] = p[offs[4]]) < thresh_l)
                    if((ring[5] = p[offs[5]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        if((ring[5] = p[offs[5]]) > thresh_h)
        if((ring[4] = p[offs[4]]) > thresh_h)
        if((ring[6] = p[offs[6]]) > thresh_h)
            if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
                if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[3] = p[offs[3]]) > thresh_h)
                    if((ring[2] = p[offs[2]]) > thresh_h)
                    {}
                    else
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    {}
                    else
                    return false;
                else
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if((ring[5] = p[offs[5]]) < thresh_l)
        if((ring[6] = p[offs[6]]) < thresh_l)
        if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                else
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    if((ring[4] = p[offs[4]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if((ring[2] = p[offs[2]]) < thresh_l)
                    if((ring[3] = p[offs[3]]) < thresh_l)
                    if((ring[4] = p[offs[4]]) < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
    else
        if((ring[4] = p[offs[4]]) > thresh_h)
        if((ring[5] = p[offs[5]]) > thresh_h)
        if((ring[6] = p[offs[6]]) > thresh_h)
        if((ring[7] = p[offs[7]]) > thresh_h)
            if((ring[8] = p[offs[8]]) > thresh_h)
            if((ring[9] = p[offs[9]]) > thresh_h)
            if((ring[10] = p[offs[10]]) > thresh_h)
            if((ring[11] = p[offs[11]]) > thresh_h)
                if((ring[12] = p[offs[12]]) > thresh_h)
                if((ring[3] = p[offs[3]]) > thresh_h)
                if((ring[2] = p[offs[2]]) > thresh_h)
                if((ring[1] = p[offs[1]]) > thresh_h)
                    {}
                else
                    if((ring[13] = p[offs[13]]) > thresh_h)
                    {}
                    else
                    return false;
                else
                if((ring[13] = p[offs[13]]) > thresh_h)
                    if((ring[14] = p[offs[14]]) > thresh_h)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                if((ring[13] = p[offs[13]]) > thresh_h)
                if((ring[14] = p[offs[14]]) > thresh_h)
                    if((ring[15] = p[offs[15]]) > thresh_h)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else if((ring[4] = p[offs[4]]) < thresh_l)
        if((ring[5] = p[offs[5]]) < thresh_l)
        if((ring[6] = p[offs[6]]) < thresh_l)
        if((ring[7] = p[offs[7]]) < thresh_l)
            if((ring[8] = p[offs[8]]) < thresh_l)
            if((ring[9] = p[offs[9]]) < thresh_l)
            if((ring[10] = p[offs[10]]) < thresh_l)
            if((ring[11] = p[offs[11]]) < thresh_l)
                if((ring[12] = p[offs[12]]) < thresh_l)
                if((ring[3] = p[offs[3]]) < thresh_l)
                if((ring[2] = p[offs[2]]) < thresh_l)
                if((ring[1] = p[offs[1]]) < thresh_l)
                    {}
                else
                    if((ring[13] = p[offs[13]]) < thresh_l)
                    {}
                    else
                    return false;
                else
                if((ring[13] = p[offs[13]]) < thresh_l)
                    if((ring[14] = p[offs[14]]) < thresh_l)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                if((ring[13] = p[offs[13]]) < thresh_l)
                if((ring[14] = p[offs[14]]) < thresh_l)
                    if((ring[15] = p[offs[15]]) < thresh_l)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else
        return false;
    
    return true;
}



// from fast_12.c, reformatted
static inline bool simd_fast_12_is_corner(const uint8_t * p, const int32_t * offs, int32_t b) {		
    int32_t thresh_h = *p + b;
    int32_t thresh_l = *p - b;
    if(p[offs[0]] > thresh_h)
        if(p[offs[1]] > thresh_h)
        if(p[offs[2]] > thresh_h)
        if(p[offs[3]] > thresh_h)
        if(p[offs[4]] > thresh_h)
            if(p[offs[5]] > thresh_h)
            if(p[offs[6]] > thresh_h)
            if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
                if(p[offs[9]] > thresh_h)
                if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                {}
                else
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
        else if(p[offs[4]] < thresh_l)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if(p[offs[8]] < thresh_l)
            if(p[offs[5]] < thresh_l)
            if(p[offs[6]] < thresh_l)
            if(p[offs[7]] < thresh_l)
                if(p[offs[9]] < thresh_l)
                if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                    if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    if(p[offs[15]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if(p[offs[3]] < thresh_l)
        if(p[offs[15]] > thresh_h)
            if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if(p[offs[7]] < thresh_l)
            if(p[offs[4]] < thresh_l)
            if(p[offs[5]] < thresh_l)
            if(p[offs[6]] < thresh_l)
                if(p[offs[8]] < thresh_l)
                if(p[offs[9]] < thresh_l)
                if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                    if(p[offs[12]] < thresh_l)
                    if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            if(p[offs[4]] < thresh_l)
            if(p[offs[5]] < thresh_l)
            if(p[offs[6]] < thresh_l)
            if(p[offs[7]] < thresh_l)
                if(p[offs[8]] < thresh_l)
                if(p[offs[9]] < thresh_l)
                if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                    if(p[offs[12]] < thresh_l)
                    if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
        if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if(p[offs[7]] < thresh_l)
            if(p[offs[4]] < thresh_l)
            if(p[offs[5]] < thresh_l)
            if(p[offs[6]] < thresh_l)
            if(p[offs[8]] < thresh_l)
                if(p[offs[9]] < thresh_l)
                if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                    if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    if(p[offs[15]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if(p[offs[2]] < thresh_l)
        if(p[offs[6]] > thresh_h)
        if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    if(p[offs[3]] > thresh_h)
                    if(p[offs[4]] > thresh_h)
                    if(p[offs[5]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if(p[offs[6]] < thresh_l)
        if(p[offs[4]] < thresh_l)
            if(p[offs[5]] < thresh_l)
            if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
                if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                    if(p[offs[3]] < thresh_l)
                    {}
                    else
                    if(p[offs[14]] < thresh_l)
                    if(p[offs[15]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        if(p[offs[6]] > thresh_h)
        if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    if(p[offs[3]] > thresh_h)
                    if(p[offs[4]] > thresh_h)
                    if(p[offs[5]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if(p[offs[6]] < thresh_l)
        if(p[offs[4]] < thresh_l)
            if(p[offs[5]] < thresh_l)
            if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
                if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    if(p[offs[3]] < thresh_l)
                    {}
                    else
                    if(p[offs[15]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if(p[offs[1]] < thresh_l)
        if(p[offs[5]] > thresh_h)
        if(p[offs[6]] > thresh_h)
        if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    if(p[offs[3]] > thresh_h)
                    if(p[offs[4]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if(p[offs[2]] > thresh_h)
                    if(p[offs[3]] > thresh_h)
                    if(p[offs[4]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if(p[offs[5]] < thresh_l)
        if(p[offs[4]] < thresh_l)
        if(p[offs[6]] < thresh_l)
            if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[3]] < thresh_l)
                if(p[offs[2]] < thresh_l)
                    {}
                else
                    if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    if(p[offs[15]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else
        if(p[offs[5]] > thresh_h)
        if(p[offs[6]] > thresh_h)
        if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                if(p[offs[15]] > thresh_h)
                    {}
                else
                    if(p[offs[3]] > thresh_h)
                    if(p[offs[4]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if(p[offs[2]] > thresh_h)
                    if(p[offs[3]] > thresh_h)
                    if(p[offs[4]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if(p[offs[5]] < thresh_l)
        if(p[offs[4]] < thresh_l)
        if(p[offs[6]] < thresh_l)
            if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[3]] < thresh_l)
                    if(p[offs[2]] < thresh_l)
                    {}
                    else
                    if(p[offs[14]] < thresh_l)
                    {}
                    else
                    return false;
                else
                    if(p[offs[14]] < thresh_l)
                    if(p[offs[15]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
    else if(p[offs[0]] < thresh_l)
        if(p[offs[1]] > thresh_h)
        if(p[offs[5]] > thresh_h)
        if(p[offs[4]] > thresh_h)
        if(p[offs[6]] > thresh_h)
            if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[3]] > thresh_h)
                if(p[offs[2]] > thresh_h)
                    {}
                else
                    if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    if(p[offs[15]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if(p[offs[5]] < thresh_l)
        if(p[offs[6]] < thresh_l)
        if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    if(p[offs[3]] < thresh_l)
                    if(p[offs[4]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if(p[offs[2]] < thresh_l)
                    if(p[offs[3]] < thresh_l)
                    if(p[offs[4]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else if(p[offs[1]] < thresh_l)
        if(p[offs[2]] > thresh_h)
        if(p[offs[6]] > thresh_h)
        if(p[offs[4]] > thresh_h)
            if(p[offs[5]] > thresh_h)
            if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
                if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                    if(p[offs[3]] > thresh_h)
                    {}
                    else
                    if(p[offs[14]] > thresh_h)
                    if(p[offs[15]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if(p[offs[6]] < thresh_l)
        if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    if(p[offs[3]] < thresh_l)
                    if(p[offs[4]] < thresh_l)
                    if(p[offs[5]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if(p[offs[2]] < thresh_l)
        if(p[offs[3]] > thresh_h)
        if(p[offs[15]] < thresh_l)
            if(p[offs[7]] > thresh_h)
            if(p[offs[4]] > thresh_h)
            if(p[offs[5]] > thresh_h)
            if(p[offs[6]] > thresh_h)
                if(p[offs[8]] > thresh_h)
                if(p[offs[9]] > thresh_h)
                if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                    if(p[offs[12]] > thresh_h)
                    if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            if(p[offs[4]] > thresh_h)
            if(p[offs[5]] > thresh_h)
            if(p[offs[6]] > thresh_h)
            if(p[offs[7]] > thresh_h)
                if(p[offs[8]] > thresh_h)
                if(p[offs[9]] > thresh_h)
                if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                    if(p[offs[12]] > thresh_h)
                    if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if(p[offs[3]] < thresh_l)
        if(p[offs[4]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[5]] > thresh_h)
            if(p[offs[6]] > thresh_h)
            if(p[offs[7]] > thresh_h)
                if(p[offs[9]] > thresh_h)
                if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                    if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    if(p[offs[15]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if(p[offs[4]] < thresh_l)
            if(p[offs[5]] < thresh_l)
            if(p[offs[6]] < thresh_l)
            if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
                if(p[offs[9]] < thresh_l)
                if(p[offs[10]] < thresh_l)
                if(p[offs[11]] < thresh_l)
                {}
                else
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
        else
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
        if(p[offs[7]] > thresh_h)
            if(p[offs[4]] > thresh_h)
            if(p[offs[5]] > thresh_h)
            if(p[offs[6]] > thresh_h)
            if(p[offs[8]] > thresh_h)
                if(p[offs[9]] > thresh_h)
                if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                    if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    if(p[offs[15]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        if(p[offs[6]] > thresh_h)
        if(p[offs[4]] > thresh_h)
            if(p[offs[5]] > thresh_h)
            if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
                if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    if(p[offs[3]] > thresh_h)
                    {}
                    else
                    if(p[offs[15]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else if(p[offs[6]] < thresh_l)
        if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    if(p[offs[3]] < thresh_l)
                    if(p[offs[4]] < thresh_l)
                    if(p[offs[5]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        if(p[offs[5]] > thresh_h)
        if(p[offs[4]] > thresh_h)
        if(p[offs[6]] > thresh_h)
            if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
                if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[13]] > thresh_h)
                if(p[offs[3]] > thresh_h)
                    if(p[offs[2]] > thresh_h)
                    {}
                    else
                    if(p[offs[14]] > thresh_h)
                    {}
                    else
                    return false;
                else
                    if(p[offs[14]] > thresh_h)
                    if(p[offs[15]] > thresh_h)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else if(p[offs[5]] < thresh_l)
        if(p[offs[6]] < thresh_l)
        if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                if(p[offs[15]] < thresh_l)
                    {}
                else
                    if(p[offs[3]] < thresh_l)
                    if(p[offs[4]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                if(p[offs[2]] < thresh_l)
                    if(p[offs[3]] < thresh_l)
                    if(p[offs[4]] < thresh_l)
                    {}
                    else
                    return false;
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
    else
        if(p[offs[4]] > thresh_h)
        if(p[offs[5]] > thresh_h)
        if(p[offs[6]] > thresh_h)
        if(p[offs[7]] > thresh_h)
            if(p[offs[8]] > thresh_h)
            if(p[offs[9]] > thresh_h)
            if(p[offs[10]] > thresh_h)
            if(p[offs[11]] > thresh_h)
                if(p[offs[12]] > thresh_h)
                if(p[offs[3]] > thresh_h)
                if(p[offs[2]] > thresh_h)
                if(p[offs[1]] > thresh_h)
                    {}
                else
                    if(p[offs[13]] > thresh_h)
                    {}
                    else
                    return false;
                else
                if(p[offs[13]] > thresh_h)
                    if(p[offs[14]] > thresh_h)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                if(p[offs[13]] > thresh_h)
                if(p[offs[14]] > thresh_h)
                    if(p[offs[15]] > thresh_h)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else if(p[offs[4]] < thresh_l)
        if(p[offs[5]] < thresh_l)
        if(p[offs[6]] < thresh_l)
        if(p[offs[7]] < thresh_l)
            if(p[offs[8]] < thresh_l)
            if(p[offs[9]] < thresh_l)
            if(p[offs[10]] < thresh_l)
            if(p[offs[11]] < thresh_l)
                if(p[offs[12]] < thresh_l)
                if(p[offs[3]] < thresh_l)
                if(p[offs[2]] < thresh_l)
                if(p[offs[1]] < thresh_l)
                    {}
                else
                    if(p[offs[13]] < thresh_l)
                    {}
                    else
                    return false;
                else
                if(p[offs[13]] < thresh_l)
                    if(p[offs[14]] < thresh_l)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                if(p[offs[13]] < thresh_l)
                if(p[offs[14]] < thresh_l)
                    if(p[offs[15]] < thresh_l)
                    {}
                    else
                    return false;
                else
                    return false;
                else
                return false;
                else
                return false;
            else
                return false;
            else
            return false;
            else
            return false;
            else
            return false;
        else
            return false;
        else
        return false;
        else
        return false;
        else
        return false;
    
    return true;
}

