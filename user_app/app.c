#include "app.h"
#define Q_CREATE_THREAD(name, thread_stack_size, thread_priority, time_tick)    \
    do                                                                          \
    {                                                                           \
        static rt_thread_t tid_##name##_task = RT_NULL;                         \
        extern void thread_##name##_task_entry(void *parameter);                \
        tid_##name##_task = rt_thread_create(#name, thread_##name##_task_entry, \
                                             RT_NULL,                           \
                                             thread_stack_size,                 \
                                             thread_priority, time_tick);       \
        if (tid_##name##_task != RT_NULL)                                       \
            rt_thread_startup(tid_##name##_task);                               \
    } while (0)
env_t env;
void app(void)
{

    // Q_CREATE_THREAD(led,               512*1,              25,          5);
    // Q_CREATE_THREAD(adc,               1024*1,              26,          5);
    // Q_CREATE_THREAD(oled,              1024*2,              26,          5);
    // Q_CREATE_THREAD(gnss,              1024*2,              26,          5);
    // Q_CREATE_THREAD(ws2812,            1024*1,              26,          5);
    // Q_CREATE_THREAD(key,               1024*1,              26,          5);
    // Q_CREATE_THREAD(flash, 1024 * 1 + 512, 26, 5);
    Q_CREATE_THREAD(lvgl, 1024 * 10, 26, 5);
    rt_thread_mdelay(2000);
    Q_CREATE_THREAD(deal, 1024 * 2, 26, 5);
    Q_CREATE_THREAD(uart, 1024, 26, 5);

    for (;;)
    {
        rt_thread_mdelay(100);
    }
}

/* 菜谱初始化数据 */
const cooking_data_t cooking_table[] =
    {
        // 薯条
        {
            .recipe = MODE_CHIPS,
            .area_temp = {180, 180, 180, 180},
            .area_time = {15, 15, 15, 15},
            .area_countdown = {0, 0, 0, 0},
        },
        // 鸡翅
        {
            .recipe = MODE_CHICKEN,
            .area_temp = {200, 200, 200, 200},
            .area_time = {20, 20, 20, 20},
            .area_countdown = {0, 0, 0, 0},
        },
        // 鱼
        {
            .recipe = MODE_FISH,
            .area_temp = {190, 190, 190, 190},
            .area_time = {15, 15, 15, 15},
            .area_countdown = {0, 0, 0, 0},
        },
        // 牛排
        {
            .recipe = MODE_STEAK,
            .area_temp = {210, 210, 210, 210},
            .area_time = {12, 12, 12, 12},
            .area_countdown = {0, 0, 0, 0},
        },
        // 虾
        {
            .recipe = MODE_SHRIMP,
            .area_temp = {190, 190, 190, 190},
            .area_time = {10, 10, 10, 10},
            .area_countdown = {0, 0, 0, 0},
        },
        // 宝宝辅食
        {
            .recipe = MODE_BABYFOOD,
            .area_temp = {130, 130, 130, 130},
            .area_time = {20, 20, 20, 20},
            .area_countdown = {0, 0, 0, 0},
        },
        // 保温
        {
            .recipe = MODE_KEEPWARM,
            .area_temp = {70, 70, 70, 70},
            .area_time = {40, 40, 40, 40},
            .area_countdown = {0, 0, 0, 0},
        },
};

const char *const recipe_name[] = RECIPE_NAME_TABLE;

induction_cooking_data_t induction_cooking_table[] =
    {
        // 待机
        {
            .recipe = MODE_STANDBY,
            .power = 0,
            .time = 0,
            .countdown = 0,
        },
        // 火锅
        {
            .recipe = MODE_HOT_POT,
            .power = 180,
            .time = 60,
            .countdown = 0,
        },
};

const char *const induction_cooking_recipe_name[] = INDUCTION_COOKING_NAME_TABLE;