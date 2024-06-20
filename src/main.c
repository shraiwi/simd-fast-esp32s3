#include "esp_timer.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "chip_qvga.h"
#include "fast.h"
#include "simd_fast.h"

static const char * TAG = "main";

#define LOG_ARR(arr) do { for (size_t _ = 0; _ < sizeof(arr) / sizeof(arr[0]); _++) { printf("%d, ", (int)arr[_]); } printf("\n"); } while (0)

void app_main() {
    vTaskDelay(pdMS_TO_TICKS(5000));

    const uint32_t b = 40;
    
    /*
    const size_t sx = 160;
    const size_t sy = 150;
    const size_t width = 3 * 16;
    const size_t height = 20;
    */

    
    const size_t sx = 0;
    const size_t sy = 0;
    const size_t width = chip_qvga_width & ~0xf;
    const size_t height = chip_qvga_height;

    //simd_u8x16 chunks[width * height / 16];
    uint8_t * image = (uint8_t *)heap_caps_aligned_alloc(16, width*height, MALLOC_CAP_DEFAULT);
    for (size_t x = 0; x < width; x++) {
        for (size_t y = 0; y < height; y++) {
            image[y * width + x] = chip_qvga_data[(y+sy) * chip_qvga_width + (x+sx)];
        }
    }

    /*uint32_t test_center = 145;
    simd_u8x16 test_ring = { 159, 49, 14, 23, 21, 21, 20, 23, 71, 189, 205, 218, 213, 211, 195, 188, };
    simd_s8x16 debug[8];

    int32_t score = simd_fast_compute3(test_center, test_ring, b, debug);

    ESP_LOGI(TAG, "score=%ld", score);

    for (size_t i = 0; i < 8; i++) {
        ESP_LOGI(TAG, "[debug buffer %u]", i);
        LOG_ARR(debug[i]);
    }
    
    return;*/

    ESP_LOGI(TAG, "sx=%u, sy=%u, w=%u, h=%u", sx, sy, width, height);
    int64_t start, end;

    ESP_LOGI(TAG, "[simd implementation]");
    size_t num_simd_corners;
    start = esp_timer_get_time();
    simd_fast_point_t * simd_corners = simd_fast12_detect(image, width, height, b, &num_simd_corners);
    end = esp_timer_get_time();

    ESP_LOGI(TAG, "num_corners=%u, elapsed=%lld", num_simd_corners, end-start);
    /*for (size_t i = 0; i < num_simd_corners; i++) {
        ESP_LOGI(TAG, "x=%u, y=%u, score=%u", simd_corners[i].x, simd_corners[i].y, simd_corners[i].score);
    }*/


    ESP_LOGI(TAG, "[reference implementation]");
    free(simd_corners);

    int num_ref_corners;
    start = esp_timer_get_time();
    xy * ref_corners = fast12_detect_nonmax(image, width, height, width, b, &num_ref_corners);
    end = esp_timer_get_time();

    ESP_LOGI(TAG, "num_corners=%d, elapsed=%lld", num_ref_corners, end - start);
    for (size_t i = 0; i < num_ref_corners; i++) {
        ESP_LOGI(TAG, "x=%d, y=%d", ref_corners[i].x, ref_corners[i].y);
    }

    free(ref_corners);

    return;
}