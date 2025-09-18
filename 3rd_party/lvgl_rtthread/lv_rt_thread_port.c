/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: MIT
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     the first version
 * 2022-05-10     Meco Man     improve rt-thread initialization process
 */

#ifdef LVGL_RTTHREAD
#define LV_TICK_PERIOD_MS 5  // 每隔 5ms 更新一次 LVGL tick

#include <lvgl.h>
#include <rtthread.h>
#ifdef PKG_USING_CPU_USAGE
#include "cpu_usage.h"
#endif /* PKG_USING_CPU_USAGE */

#define DBG_TAG "LVGL"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#ifndef PKG_LVGL_THREAD_STACK_SIZE
#define PKG_LVGL_THREAD_STACK_SIZE 4096
#endif /* PKG_LVGL_THREAD_STACK_SIZE */

#ifndef PKG_LVGL_THREAD_PRIO
#define PKG_LVGL_THREAD_PRIO (RT_THREAD_PRIORITY_MAX * 2 / 3)
#endif /* PKG_LVGL_THREAD_PRIO */

#ifndef PKG_LVGL_DISP_REFR_PERIOD
#define PKG_LVGL_DISP_REFR_PERIOD 33
#endif /* PKG_LVGL_DISP_REFR_PERIOD */

extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);
extern void lv_user_gui_init(void);

static struct rt_thread lvgl_thread;

#ifdef rt_align
rt_align(RT_ALIGN_SIZE)
#else
ALIGN(RT_ALIGN_SIZE)
#endif
    static rt_uint8_t lvgl_thread_stack[PKG_LVGL_THREAD_STACK_SIZE];

#if LV_USE_LOG
static void lv_rt_log(lv_log_level_t level, const char *buf)
{
    (void)level;
    LOG_I(buf);
}
#endif /* LV_USE_LOG */

#ifdef PKG_USING_CPU_USAGE
uint32_t lv_timer_os_get_idle(void)
{
    return (100 - (uint32_t)cpu_load_average());
}
#endif /* PKG_USING_CPU_USAGE */
static rt_timer_t lv_tick_timer;

static void lv_tick_timer_cb(void *parameter)
{
    /* 增加 LVGL tick */
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
void lv_port_tick_init(void)
{
    /* 创建周期性定时器 */
    lv_tick_timer = rt_timer_create("lv_tick",
                                    lv_tick_timer_cb,
                                    RT_NULL,
                                    LV_TICK_PERIOD_MS,
                                    RT_TIMER_FLAG_PERIODIC);

    if(lv_tick_timer != RT_NULL)
    {
        rt_timer_start(lv_tick_timer);
    }
}

static void lvgl_thread_entry(void *parameter)
{
#if LV_USE_LOG
    lv_log_register_print_cb(lv_rt_log);
#endif /* LV_USE_LOG */
    lv_init();
		lv_port_tick_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_user_gui_init();

#ifdef PKG_USING_CPU_USAGE
    cpu_usage_init();
#endif /* PKG_USING_CPU_USAGE */

    /* handle the tasks of LVGL */
    while (1)
    {
        lv_timer_handler();
        rt_thread_mdelay(PKG_LVGL_DISP_REFR_PERIOD);
    }
}

static int lvgl_thread_init(void)
{
    rt_err_t err;

    err = rt_thread_init(&lvgl_thread, "LVGL", lvgl_thread_entry, RT_NULL,
                         &lvgl_thread_stack[0], sizeof(lvgl_thread_stack), PKG_LVGL_THREAD_PRIO, 10);
    if (err != RT_EOK)
    {
        LOG_E("Failed to create LVGL thread");
        return -1;
    }
    rt_thread_startup(&lvgl_thread);

    return 0;
}



INIT_ENV_EXPORT(lvgl_thread_init);

#endif /*__RTTHREAD__*/
