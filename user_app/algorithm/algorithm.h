#include <stdint.h>

#define FILTER_WINDOW_SIZE 8 // 滑动窗口大小，可修改

typedef struct
{
    float buffer[FILTER_WINDOW_SIZE]; // 数据缓存
    uint8_t index;                    // 当前写入位置
    uint8_t count;                    // 已填充的数据数量
} sliding_filter_t;

void sliding_filter_init(sliding_filter_t *filter);
float sliding_filter_process(sliding_filter_t *filter, float new_value);
