#include "algorithm.h"
/**
 * @brief 初始化滑动滤波器
 * @param filter 滤波器结构体指针
 */
void sliding_filter_init(sliding_filter_t *filter)
{
    for (uint8_t i = 0; i < FILTER_WINDOW_SIZE; i++)
        filter->buffer[i] = 0.0f;
    filter->index = 0;
    filter->count = 0;
}
float sliding_filter_process(sliding_filter_t *filter, float new_value)
{
    filter->buffer[filter->index] = new_value;
    filter->index = (filter->index + 1) % FILTER_WINDOW_SIZE;
    if (filter->count < FILTER_WINDOW_SIZE)
        filter->count++;

    // 当缓存未满时，直接返回最新值
    if (filter->count < FILTER_WINDOW_SIZE)
        return new_value;

    // 缓存满后，计算窗口平均
    float sum = 0.0f;
    for (uint8_t i = 0; i < FILTER_WINDOW_SIZE; i++)
        sum += filter->buffer[i];

    return sum / FILTER_WINDOW_SIZE;
}
