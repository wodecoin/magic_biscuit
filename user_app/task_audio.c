#include "app.h"
#include "onewire_bus.h"

#undef LOG_TAG
#define LOG_TAG "AUDIO_TASK"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

//=======================
// 主线程函数
//=======================
void thread_audio_task_entry(void *parameter)
{
    rt_thread_mdelay(3000);
    rt_uint32_t e;

    LOG_D("thread_audio_task_entry");
    while (1)
    {
        if (rt_event_recv(&env.ui_event,
                          EVENT_PLAY_AUDIO | EVENT_MOTOR_WORK,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_NO, &e) == RT_EOK)
        {
            if (e & EVENT_MOTOR_WORK)
            {
                LOG_I("[UI_EVENT] Recv: EVENT_MOTOR_WORK (0x%08x)\n", e);
                // 使能震动电机
                GPIO_SetBits(SHAKE_MOTOR_PORT, SHAKE_MOTOR_PIN);
                rt_thread_mdelay(10);
                GPIO_ResetBits(SHAKE_MOTOR_PORT, SHAKE_MOTOR_PIN);
            }
            if (e & EVENT_PLAY_AUDIO)
            {
                static uint8_t cnt = 1;
                wt588f_init(&g_wt588f); // 必须初始化引脚 才能100% 播放，有点奇怪
                LOG_I("[UI_EVENT] Recv: EVENT_PLAY_AUDIO (0x%08x)\n", e);
                wt588f_play(&g_wt588f, cnt++);
                if (cnt > 4)
                    cnt = 1;
                wt588f_wait_finish(&g_wt588f, 2000);
                wt588f_stop(&g_wt588f);
            }
        }
        rt_thread_mdelay(50);
    }
}

/* ----------------- module init ----------------- */
// =================== 示例使用 ===================

void wt588f_demo(void)
{
    wt588f_init(&g_wt588f);
    wt588f_set_volume(&g_wt588f, 0xEF);
    wt588f_play(&g_wt588f, 0x01);
    wt588f_wait_finish(&g_wt588f, 5000);
    wt588f_stop(&g_wt588f);
}

MSH_CMD_EXPORT(wt588f_demo, WT588F voice test);
