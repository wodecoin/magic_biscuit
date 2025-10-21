#include "app.h"
#include "ui.h"
#undef LOG_TAG
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
typedef struct
{
    uint16_t widget_id;         // 控件 ID
    uint16_t tlv_type;          // 对应 TLV 类型
    uint8_t (*get_value)(void); // 获取该控件的值（函数指针）
    uint16_t delayed_event;     // 延迟更新事件
    uint16_t delayed_time;      // 延迟时间（ms）
} widget_tlv_map_t;

// --- 每个控件与TLV、函数的映射表 ---
static uint8_t get_slider1_value(void) { return lv_slider_get_value(ui_Slider1); }
static uint8_t get_slider2_value(void) { return lv_slider_get_value(ui_Slider2); }

static const widget_tlv_map_t widget_map[] = {
    {WIDGET_AIR_FRY_TEMP, TLV_MODE_TEMP, get_slider1_value, EVENT_UPDATA_SLIDER1_SCT, 1000},
    {WIDGET_AIR_FRY_TIMER, TLV_HEAT_TIME, get_slider2_value, EVENT_UPDATA_SLIDER2_SCT, 1000},
    {WIDGET_AIR_FRY_MODE, TLV_COOK_MODE, NULL, EVENT_UPDATA_MODE_SCT, 1500},
    {WIDGET_AIR_FRY_START, TLV_MODE_TEMP, get_slider1_value, EVENT_UPDATA_SW_SCT, 1500},
    {WIDGET_INDUCTION_COOKING_MODE, TYPE_COOKER_MODE_SELECT, NULL, EVENT_UPDATA_MODE_SCT, 1500},
};

#define RT_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

void thread_deal_task_entry(void *parameter)
{
    ui_msg_t msg;
    rt_err_t result;

    rt_mq_init(&env.msg_queue,
               "ui_msgq",
               msg_pool,
               sizeof(ui_msg_t),
               MSG_POOL_SIZE,
               RT_IPC_FLAG_FIFO);

    if (rt_event_init(&env.ui_event, "ui_event", RT_IPC_FLAG_FIFO) != RT_EOK)
        rt_kprintf("init event failed.\r\n");

    ble_sent_sem = rt_sem_create("blesem", 1, RT_IPC_FLAG_PRIO);
    if (ble_sent_sem == RT_NULL)
        rt_kprintf("create sem failed\n");

    while (1)
    {
        result = rt_mq_recv(&env.msg_queue, &msg, sizeof(msg), RT_WAITING_NO);

        if (result == RT_EOK)
        {
            LOG_I("widget_id=%d, event=%d, value=%d", msg.widget_id, msg.event, msg.value);

            tlv_t tlvs[15];
            int tlv_count = 0;

            // 在表中查找对应控件
            for (int i = 0; i < RT_ARRAY_SIZE(widget_map); i++)
            {
                const widget_tlv_map_t *map = &widget_map[i];
                if (msg.widget_id == map->widget_id)
                {
                    // 特殊处理 START：同时发送温度和时间
                    if (msg.widget_id == WIDGET_AIR_FRY_START)
                    {
                        tlvs[tlv_count++] = (tlv_t){TLV_MODE_TEMP, 1, {get_slider1_value()}};
                        tlvs[tlv_count++] = (tlv_t){TLV_HEAT_TIME, 1, {get_slider2_value()}};
                    }
                    else if (msg.widget_id == WIDGET_AIR_FRY_MODE)
                    {
                        // ==========================
                        // 设置分区温度
                        // ==========================
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE1_TEMP, 1, {cooking_table[msg.value].area_temp[0]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE2_TEMP, 1, {cooking_table[msg.value].area_temp[1]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE3_TEMP, 1, {cooking_table[msg.value].area_temp[2]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE4_TEMP, 1, {cooking_table[msg.value].area_temp[3]}};

                        // ==========================
                        // 设置分区时间
                        // ==========================
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE1_TIME, 1, {cooking_table[msg.value].area_time[0]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE2_TIME, 1, {cooking_table[msg.value].area_time[1]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE3_TIME, 1, {cooking_table[msg.value].area_time[2]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_SET_ZONE4_TIME, 1, {cooking_table[msg.value].area_time[3]}};

                        // ==========================
                        // 设置分区预约时间
                        // ==========================
                        tlvs[tlv_count++] = (tlv_t){TLV_RESERVE_ZONE1_TIME, 1, {cooking_table[msg.value].area_countdown[0]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_RESERVE_ZONE2_TIME, 1, {cooking_table[msg.value].area_countdown[1]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_RESERVE_ZONE3_TIME, 1, {cooking_table[msg.value].area_countdown[2]}};
                        tlvs[tlv_count++] = (tlv_t){TLV_RESERVE_ZONE4_TIME, 1, {cooking_table[msg.value].area_countdown[3]}};
                    }
                    else if (msg.widget_id == WIDGET_INDUCTION_COOKING_MODE)
                    {
                        tlvs[tlv_count++] = (tlv_t){TYPE_COOKER_MODE_SELECT, 1, {msg.value}};
                    }
                    // else
                    // {
                    //     tlvs[tlv_count++] = (tlv_t){map->tlv_type, 1, {map->get_value ? map->get_value() : msg.value}};
                    // }

                    // 延迟事件
                    send_ui_event_delayed(map->delayed_event, map->delayed_time);
                    break;
                }
            }
            // 开始/停止特殊处理
            if (msg.event == EVT_ON || msg.event == EVT_OFF)
            {
                tlvs[tlv_count++] = (tlv_t){TLV_MODE_TEMP, 1, {(msg.event == EVT_ON) ? 1 : 0}};
            }
            else if (msg.event == EVT_INDUCTION_COOKING_ON || msg.event == EVT_INDUCTION_COOKING_OFF)
            {
                tlvs[tlv_count++] = (tlv_t){TYPE_COOKER_OFF_TIMER, 1, {(msg.event == EVT_INDUCTION_COOKING_ON) ? 1 : 0}};
            }

            // 统一发送 TLV
            if (tlv_count > 0)
                send_tlv_settings(AIR_FRY_DEV_TYPE, tlvs, tlv_count);
        }

        // 定时查询
        rt_thread_mdelay(TICK_MS);
        static uint32_t tick_count = 0;
        tick_count += TICK_MS;
        if (tick_count >= 1500)
        {
            tick_count = 0;
            LOG_D("1s air fry query");
            do_1s_air_fry_query_cmd_generic();
            // 检查闹钟
            if (ds3231_check_alarm1())
                rt_kprintf("Alarm1 triggered!\n");
            else
                rt_kprintf("Alarm1 not triggered.\n");
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
uint8_t send_buf[64];

static void send_query_cmd(uint8_t dev_idx, tlv_t *tlvs, int tlv_count)
{
    int len = build_frame(&send_frame, FRAME_QUERY_REQ, dev_idx,
                          tlvs, tlv_count, send_buf);
    print_hex_buffer(TAG, send_buf, len);
    UART_SendBytes(UART4, send_buf, len);
    LOG_I(TAG, "writprint_hex_buffere to BLE");
}

tlv_t ctl_tlvs[8]; // 用于查询的 TLV，最多 4 个 为3 会产生bug -- UNALIGNED

static int do_1s_air_fry_query_cmd_generic(void)
{

    // 先清空数组，假设 ctl_tlvs 足够大
    memset(ctl_tlvs, 0, sizeof(ctl_tlvs));

    int tlv_count = 0;
    lv_obj_t *act_scr = lv_scr_act();
    if (act_scr == ui_Screen5)
    {
        ctl_tlvs[tlv_count++].type = TYPE_COOKER_POT_DETECT;
        ctl_tlvs[tlv_count++].type = TLV_MODE_TEMP;
        ctl_tlvs[tlv_count++].type = TYPE_COOKER_SET_POWER;
    }
    else if (act_scr == ui_Screen1)
    {
        // 查询分区温度
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE1_TEMP;
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE2_TEMP;
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE3_TEMP;
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE4_TEMP;

        // 查询分区剩余时间
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE1_REMAIN;
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE2_REMAIN;
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE3_REMAIN;
        ctl_tlvs[tlv_count++].type = TLV_QUERY_ZONE4_REMAIN;
    }
    // 发送查询命令
    send_query_cmd(AIR_FRY_DEV_TYPE, ctl_tlvs, tlv_count);

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

        int len = build_frame(&send_frame, FRAME_CTRL_REQ, dev_id, tlvs, tlv_count, send_buf);

        LOG_I(TAG, "send data and  tlv_count = %d", tlv_count);
        print_hex_buffer(TAG, send_buf, len);
        UART_SendBytes(UART4, send_buf, len);
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
static int x_start = 0; // x 轴起始值
#define POINT_COUNT 10
static lv_coord_t x_array[POINT_COUNT];
void lv_ui_deal(uint16_t ms)
{

    rt_uint32_t e;

    if (rt_event_recv(&env.ui_event,
                      EVENT_UPDATA_SLIDER1_SCT | EVENT_UPDATA_SLIDER2_SCT |
                          EVENT_UPDATA_SW_SCT | EVENT_UPDATA_MODE_SCT |
                          EVENT_UPDATA_TIME,
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
        if (e & EVENT_UPDATA_TIME)
        {
            char buf[16];
            lv_obj_t *act_scr = lv_scr_act();
            ds3231_get_time(&env.time);
            rt_kprintf("[UI_EVENT] Recv: time (0x%08x) %02d:%02d:%02d\n", e, env.time.tm_hour, env.time.tm_min, env.time.tm_sec);
            // screen2
            if (act_scr == ui_Screen2)
            {
                snprintf(buf, sizeof(buf), "%d", env.alarm.tm_year);
                lv_label_set_text(ui_yyLabel1, buf);
                snprintf(buf, sizeof(buf), "%d", env.alarm.tm_mon);
                lv_label_set_text(ui_mmLabel1, buf);
                snprintf(buf, sizeof(buf), "%d", env.alarm.tm_mday);
                lv_label_set_text(ui_ddLabel1, buf);
                snprintf(buf, sizeof(buf), "%d:%d", env.alarm.tm_hour, env.alarm.tm_min);
                lv_label_set_text(ui_minLabel1, buf);
                snprintf(buf, sizeof(buf), "%d", env.alarm.tm_sec);
                lv_label_set_text(ui_ssLabel1, buf);
            }
        }
    }
    if (rt_mq_recv(&env.bt_rev_msg_queue, &dmsg, sizeof(dmsg), RT_WAITING_NO) == RT_EOK)
    {
        LOG_D("recv bt_rev_msg_queue");
        ui_update_cb(&dmsg);
    }
    static uint32_t tick_last = 0;
    tick_last += ms;
    if (tick_last >= 500)
    {
        tick_last = 0;
        extern lv_chart_series_t *ui_Chart3_series_1;
        static uint8_t value1 = 0;
        lv_chart_set_next_value(ui_Chart3, ui_Chart3_series_1, value1 += 5);

        lv_chart_refresh(ui_Chart3); // 刷新图表以显示新数据
        // lv_refresh_area_percent(0, 0, 100, 10);
        // lv_refresh_area_percent(0, 75, 100, 100);
    }
    lv_timer_handler(); // LVGL渲染
}
void ui_update_cb(void *param)
{
    display_msg_t *msg = (display_msg_t *)param;

    if (msg->widget_id == WIDGET_AIR_FRY_DISPLAY)
    {
        rt_kprintf("%0.1f %0.1f %0.1f", env.ctl_dev.kpa, env.ctl_dev.zone_real_temp[0], env.ctl_dev.rh_value);
        char buf[16];
        // screen1
        lv_obj_t *act_scr = lv_scr_act(); // 获取当前活动屏幕

        if (act_scr == ui_Screen1)
        {
            snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.kpa);
            lv_label_set_text(ui_paLabel, buf);
            snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.zone_real_temp[0]);
            lv_label_set_text(ui_humidityLabel, buf);
            snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.rh_value);
            lv_label_set_text(ui_tempLabel, buf);
        }

        if (act_scr == ui_Screen5)
        {
            // snprintf(buf, sizeof(buf), "%.2f", (float)(env.induction_cooking_ctl_dev.temperature / 100));
            // lv_label_set_text(ui_paLabel, buf);
            // snprintf(buf, sizeof(buf), "%d", env.induction_cooking_ctl_dev.pot_detect);
            // lv_label_set_text(ui_humidityLabel, buf);
            // snprintf(buf, sizeof(buf), "%.1f", env.induction_cooking_ctl_dev.power);
            // lv_label_set_text(ui_tempLabel, buf);
        }
        // screen3
        if (act_scr == ui_Screen3)
        {
            snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.zone_real_temp[0]);
            lv_label_set_text(ui_area1tempLabel, buf);
            snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.zone_real_temp[1]);
            lv_label_set_text(ui_area1tempLabel2, buf);
            snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.zone_real_temp[2]);
            lv_label_set_text(ui_area1tempLabel3, buf);
            snprintf(buf, sizeof(buf), "%.1f", env.ctl_dev.zone_real_temp[3]);
            lv_label_set_text(ui_area1tempLabel4, buf);

            snprintf(buf, sizeof(buf), "%d", env.ctl_dev.zone_remain_time[0]);
            lv_label_set_text(ui_area1timeLabel1, buf);
            snprintf(buf, sizeof(buf), "%d", env.ctl_dev.zone_remain_time[1]);
            lv_label_set_text(ui_area1timeLabel2, buf);
            snprintf(buf, sizeof(buf), "%d", env.ctl_dev.zone_remain_time[2]);
            lv_label_set_text(ui_area1timeLabel3, buf);
            snprintf(buf, sizeof(buf), "%d", env.ctl_dev.zone_remain_time[3]);
            lv_label_set_text(ui_area1timeLabel4, buf);
        }

        lv_refresh_area_percent(0, 75, 50, 100); // 刷新1/4左下角
    }
}