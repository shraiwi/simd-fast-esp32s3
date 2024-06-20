#include "simd_fast.h"

#include "string.h"
#include "stdio.h"
#include "stdbool.h"

#include "esp_log.h"

#include "fast.h"

static const char * TAG = "simd_fast";

#define min(x, y) (x)<(y)?(x):(y)
#define max(x, y) (x)>(y)?(x):(y)

#define LOG_ARR(arr) do { for (size_t _ = 0; _ < sizeof(arr) / sizeof(arr[0]); _++) { printf("%d, ", (int)arr[_]); } printf("\n"); } while (0)

static inline bool simd_fast_12_is_corner(const uint8_t * p, const int32_t * offs, int32_t b);
static inline bool simd_fast_12_is_corner_ring(const uint8_t * p, const int32_t * offs, int32_t b, simd_u8x16 ring);

// usually take 11112
simd_fast_point_t * simd_fast12_detect(const uint8_t * pixels, int32_t w, int32_t h, uint32_t b, size_t * ret_num_corners) {
    int32_t offs[] = {
        0 + w * 3,
        1 + w * 3,
        2 + w * 2,
        3 + w * 1,
        3 + w * 0,
        3 + w * -1,
        2 + w * -2,
        1 + w * -3,
        0 + w * -3,
        -1 + w * -3,
        -2 + w * -2,
        -3 + w * -1,
        -3 + w * 0,
        -3 + w * 1,
        -2 + w * 2,
        -1 + w * 3,
    };

    size_t points_len = 256;
    simd_fast_point_t * points = malloc(sizeof(simd_fast_point_t) * points_len);
    *ret_num_corners = 0;

    simd_u8x16 candidates, ring;

    int32_t row_starts[h];

    for (int32_t y = 3; y < h - 3; y++) {
        row_starts[y] = INT32_MAX;
        for (int32_t x = 0; x < w; x += 16) {
            const uint8_t * p = pixels + x + y * w;
            simd_fast_candidates2(p, w, b, candidates);

            int32_t start = max(3 - x, 0);
            int32_t end = min(w - 3 - x, 16);

            for (int32_t i = start; i < end; i++) {
                if (candidates[i] > 1 
                    && simd_fast_12_is_corner_ring(p + i, offs, b, ring)) {

                    simd_fast_point_t * pt = &points[*ret_num_corners];
                    
                    pt->x = x+i;
                    pt->y = y;
                    pt->score = simd_fast_score(ring, p[i]);
                    row_starts[y] = min(*ret_num_corners, row_starts[y]);
                    
                    ESP_LOGI(TAG, "x=%u, y=%u, score=%ld, center=%hu", pt->x, pt->y, pt->score, p[i]);
                    LOG_ARR(ring);

                    if (++(*ret_num_corners) == points_len) {
                        points_len *= 2;
                        points = realloc(points, sizeof(simd_fast_point_t) * points_len);
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < *ret_num_corners; i++) {
        
    }

    return points;
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

