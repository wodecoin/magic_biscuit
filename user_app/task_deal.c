#include "app.h"
#include "ui.h"

#define LOG_TAG "DEAL_TASK"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

#define MSG_QUEUE_SIZE 4 // 队列最多存放 16 条消息
#define MSG_POOL_SIZE (MSG_QUEUE_SIZE * sizeof(ui_msg_t))
static char msg_pool[MSG_POOL_SIZE]; // 注意这里是字节数组

#define AIR_FRY_DEV_TYPE 0x03
#define TICK_MS 50

static rt_sem_t ble_sent_sem = RT_NULL;
static void send_query_cmd(uint8_t dev_idx, tlv_t *tlvs, int tlv_count);
static void send_tlv_settings(uint8_t dev_id, tlv_t *tlvs, int tlv_count);
static int do_1s_air_fry_query_cmd_generic(void);
void ui_update_cb(void *param);

void send_ui_event_delayed(rt_uint32_t event, rt_tick_t delay_ms);

display_msg_t dmsg;
void thread_deal_task_entry(void *parameter)
{
    ui_msg_t msg;
    rt_err_t result;

    rt_mq_init(&env.msg_queue,
               "ui_msgq",
               msg_pool,
               sizeof(ui_msg_t), // 单条消息大小
               MSG_POOL_SIZE,    // 整个缓冲池大小（字节数）
               RT_IPC_FLAG_FIFO);
    if (rt_event_init(&env.ui_event, "ui_event", RT_IPC_FLAG_FIFO) != RT_EOK)
    {
        rt_kprintf("init event failed.\r\n");
    }
    ble_sent_sem = rt_sem_create("blesem", 1, RT_IPC_FLAG_PRIO);
    if (ble_sent_sem == RT_NULL)
    {
        rt_kprintf("create sem failed\n");
    }

    while (1)
    {
        /* 阻塞等待消息到来 */
        result = rt_mq_recv(&env.msg_queue, &msg, sizeof(msg), RT_WAITING_NO); // 阻塞等待 避免打断qspi
        if (result == RT_EOK)
        {
            LOG_I("widget_id=%d, event=%d, value=%d\n",
                  msg.widget_id, msg.event, msg.value);
            tlv_t air_fry_tlvs[3];
            /* 根据 widget_id 和 event 做处理 */
            tlv_t tlvs[3]; // 最多 3 个 TLV
            int tlv_count = 0;

            switch (msg.widget_id)
            {
            case WIDGET_AIR_FRY_TEMP:
                tlvs[tlv_count].type = TLV_MODE_TEMP;
                tlvs[tlv_count].length = 1;
                tlvs[tlv_count].value[0] = lv_slider_get_value(ui_Slider1);
                tlv_count++;
                break;

            case WIDGET_AIR_FRY_TIMER:
                tlvs[tlv_count].type = TLV_HEAT_TIME;
                tlvs[tlv_count].length = 1;
                tlvs[tlv_count].value[0] = lv_slider_get_value(ui_Slider2);
                tlv_count++;
                break;

            case WIDGET_AIR_FRY_MODE:
                tlvs[tlv_count].type = TLV_COOK_MODE;
                tlvs[tlv_count].length = 1;
                tlvs[tlv_count].value[0] = msg.value;
                tlv_count++;
                break;

            case WIDGET_AIR_FRY_START:
                // 可以根据需要选择同时发送温度和时间，也可以只发送开关
                tlvs[tlv_count].type = TLV_MODE_TEMP;
                tlvs[tlv_count].length = 1;
                tlvs[tlv_count].value[0] = lv_slider_get_value(ui_Slider1);
                tlv_count++;

                tlvs[tlv_count].type = TLV_HEAT_TIME;
                tlvs[tlv_count].length = 1;
                tlvs[tlv_count].value[0] = lv_slider_get_value(ui_Slider2);
                tlv_count++;
                break;

            default:
                rt_kprintf("Other widget event\n");
                return;
            }

            // 通用开关 TLV
            if (msg.event == EVT_ON || msg.event == EVT_OFF)
            {
                tlvs[tlv_count].type = TLV_SW;
                tlvs[tlv_count].length = 1;
                tlvs[tlv_count].value[0] = (msg.event == EVT_ON) ? 1 : 0;
                tlv_count++;
            }

            if (tlv_count > 0)
                send_tlv_settings(AIR_FRY_DEV_TYPE, tlvs, tlv_count);

            if (msg.widget_id == WIDGET_AIR_FRY_TEMP)
                send_ui_event_delayed(EVENT_UPDATA_SLIDER1_SCT, 1000);
            else if (msg.widget_id == WIDGET_AIR_FRY_TIMER)
                send_ui_event_delayed(EVENT_UPDATA_SLIDER2_SCT, 1000);
            else if (msg.widget_id == WIDGET_AIR_FRY_START)
                send_ui_event_delayed(EVENT_UPDATA_SW_SCT, 1500);
            else if (msg.widget_id == WIDGET_AIR_FRY_MODE)
                send_ui_event_delayed(EVENT_UPDATA_MODE_SCT, 1500);
        }

        rt_thread_mdelay(TICK_MS);
        static uint32_t tick_count = 0;
        tick_count += TICK_MS;
        if (tick_count >= 1000)
        {
            tick_count = 0;
            LOG_D("1s air fry query");
            do_1s_air_fry_query_cmd_generic();
        }
    }
}
/* 静态定时器和参数 */
static rt_timer_t ui_event_timer = RT_NULL;
typedef struct
{
    rt_uint32_t event;
} ui_event_param_t;
static ui_event_param_t ui_event_param;

/* 定时器回调 */
static void ui_event_timer_cb(void *parameter)
{
    rt_event_send(&env.ui_event, ui_event_param.event);
}

/* 延时发送事件 */
void send_ui_event_delayed(rt_uint32_t event, rt_tick_t delay_ms)
{
    rt_tick_t ticks = rt_tick_from_millisecond(delay_ms);

    /* 更新事件值 */
    ui_event_param.event = event;
    rt_kprintf("send_ui_event_delayed event=0x%08x, delay_ms=%d, ticks=%d\n", event, delay_ms, ticks);
    /* 创建定时器（只创建一次） */
    if (ui_event_timer == RT_NULL)
    {
        ui_event_timer = rt_timer_create("ui_evt",
                                         ui_event_timer_cb,
                                         NULL,
                                         ticks,
                                         RT_TIMER_FLAG_ONE_SHOT);
        if (ui_event_timer == RT_NULL)
            return;
    }
    else
    {
        /* 更新定时器周期 */
        rt_timer_control(ui_event_timer, RT_TIMER_CTRL_SET_TIME, (void *)ticks);
        rt_timer_stop(ui_event_timer);
    }

    /* 启动/重启定时器 */
    rt_timer_start(ui_event_timer);
}

frame_t send_frame;
#define TAG "TASK_DEAL"
static void send_query_cmd(uint8_t dev_idx, tlv_t *tlvs, int tlv_count)
{
    uint8_t buf[32];
    int len = build_frame(&send_frame, FRAME_QUERY_REQ, dev_idx,
                          tlvs, tlv_count, buf);
    print_hex_buffer(TAG, buf, len);
    LOG_I(TAG, "writprint_hex_buffere to BLE");
}

tlv_t ctl_tlvs[4]; // 用于查询的 TLV，最多 4 个 为3 会产生bug -- UNALIGNED

static int do_1s_air_fry_query_cmd_generic(void)
{
    ctl_tlvs[0].type = TLV_AREA1_TEMP;
    ctl_tlvs[1].type = TLV_KPA;
    ctl_tlvs[2].type = TLV_RH;
    send_query_cmd(AIR_FRY_DEV_TYPE, ctl_tlvs, 3);
    return 0;
}

static void send_tlv_settings(uint8_t dev_id, tlv_t *tlvs, int tlv_count)
{
    if (ble_sent_sem == RT_NULL)
    {
        LOG_W(TAG, "BLE sem is NULL");
        return;
    }
    if (rt_sem_take(ble_sent_sem, RT_WAITING_FOREVER) == RT_EOK)
    {
        uint8_t buf[32];
        int len = build_frame(&send_frame, FRAME_CTRL_REQ, dev_id, tlvs, tlv_count, buf);

        LOG_I(TAG, "send data and  tlv_count = %d", tlv_count);
        print_hex_buffer(TAG, buf, len);
        UART_SendBytes(UART4, buf, len);
        rt_sem_release(ble_sent_sem);
        LOG_I(TAG, "write to BLE");
    }
    else
    {
        LOG_E(TAG, "BLE sem is failed");
    }
}
void lv_refresh_area_percent(uint8_t x_start_percent, uint8_t y_start_percent,
                             uint8_t x_end_percent, uint8_t y_end_percent)
{
    lv_area_t area;
    const lv_coord_t hor_res = lv_disp_get_hor_res(NULL);
    const lv_coord_t ver_res = lv_disp_get_ver_res(NULL);

    /* 计算像素区域 */
    area.x1 = (x_start_percent * hor_res) / 100;
    area.y1 = (y_start_percent * ver_res) / 100;
    area.x2 = (x_end_percent * hor_res) / 100 - 1;
    area.y2 = (y_end_percent * ver_res) / 100 - 1;

    /* 标记该区域需要刷新 */
    lv_obj_invalidate_area(lv_scr_act(), &area);
}

void lv_ui_deal(uint16_t ms)
{

    rt_uint32_t e;

    if (rt_event_recv(&env.ui_event,
                      EVENT_UPDATA_SLIDER1_SCT | EVENT_UPDATA_SLIDER2_SCT | EVENT_UPDATA_SW_SCT | EVENT_UPDATA_MODE_SCT,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_NO, &e) == RT_EOK)
    {
        if (e & EVENT_UPDATA_SLIDER1_SCT)
        {
            rt_kprintf("[UI_EVENT] Recv: SLIDER1_SCT (0x%08x)\n", e);
            lv_refresh_area_percent(0, 25, 100, 100);
        }
        if (e & EVENT_UPDATA_SLIDER2_SCT)
        {
            rt_kprintf("[UI_EVENT] Recv: SLIDER2_SCT (0x%08x)\n", e);
            lv_refresh_area_percent(0, 50, 100, 100);
        }
        if (e & EVENT_UPDATA_MODE_SCT)
        {
            rt_kprintf("[UI_EVENT] Recv: mode (0x%08x)\n", e);
            lv_refresh_area_percent(0, 0, 100, 25);
        }
    }
    if (rt_mq_recv(&env.bt_rev_msg_queue, &dmsg, sizeof(dmsg), RT_WAITING_NO) == RT_EOK)
    {
        LOG_D("recv bt_rev_msg_queue");
        ui_update_cb(&dmsg);
    }
    static uint32_t tick_last = 0;
    tick_last += ms;
    if (tick_last >= 1000)
    {
        tick_last = 0;
        lv_refresh_area_percent(0, 0, 100, 25);
        lv_refresh_area_percent(0, 75, 100, 100);
    }
    lv_timer_handler(); // LVGL渲染
}
void ui_update_cb(void *param)
{
    display_msg_t *msg = (display_msg_t *)param;

    if (msg->widget_id == WIDGET_AIR_FRY_DISPLAY)
    {
        rt_kprintf("%0.1f %0.1f %0.1f", env.ctl_dev.kpa, env.ctl_dev.real_temp, env.ctl_dev.rh_value);
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.kpa);
        lv_label_set_text(ui_paLabel, buf);
        snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.real_temp);
        lv_label_set_text(ui_tempLabel, buf);
        snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.rh_value);
        lv_label_set_text(ui_humidityLabel, buf);
        lv_refresh_area_percent(0, 75, 50, 100); // 刷新1/4左下角
    }
}