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
