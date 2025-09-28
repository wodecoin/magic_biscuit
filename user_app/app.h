#ifndef _APP_H_
#define _APP_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "platform.h"
#include "rtthread.h"
#include "tlv_protocol.h"

  typedef enum
  {
    // 电磁炉
    WIDGET_MODE,       // 模式滚轮
    WIDGET_TEMP,       // 温度滚轮
    WIDGET_PAN_DETECT, // 锅具检测开关
    WIDGET_TIMER,      // 预约时间
    WIDGET_WORK_TIME,  // 工作时间
    WIDGET_SHUTDOWN,   // 关机时间

    WIDGET_AIR_FRY_TIMER,
    WIDGET_AIR_FRY_TEMP,
    WIDGET_AIR_FRY_START,
    WIDGET_AIR_FRY_MODE,
    WIDGET_AIR_FRY_DISPLAY,
    WIDGET_MUG_MODE,
    WIDGET_MAX
  } widget_id_t;

  typedef enum
  {
    // 电磁炉
    EVT_NONE = 0,
    EVT_MODE_CHANGE,
    EVT_TEMP_CHANGE,
    EVT_ON,
    EVT_OFF,
    EVT_PAN_DETECT_CHANGE,
    EVT_TIMER_CHANGE,
    EVT_WORK_TIME_CHANGE,
    EVT_SHUTDOWN_CHANGE,

    EVT_AIR_FRY_TEMP_CHANGE,
    EVT_AIR_FRY_TIMER_CHANGE,
    EVT_AIR_FRY_DISPLAY_CHANGE,
    EVT_AIR_FRY_MOED_CHANGE,
    EVT_MUG_MODE_CHANGE

  } widget_event_t;

  typedef struct
  {
    widget_id_t widget_id;
    widget_event_t event;
    int value; // 对滚轮是选中索引，对开关是0/1
  } ui_msg_t;
  typedef struct
  {
    widget_id_t widget_id;
    widget_event_t event;
    int value; // 对滚轮是选中索引，对开关是0/1
  } display_msg_t;

  struct ctl_dev_t
  {
    uint8_t mode;
    uint16_t set_temp;
    uint16_t set_time;
    uint16_t start_flag;
    float kpa;
    float real_temp;
    float rh_value;
  };
  struct base_dev_t
  {
    uint16_t run_time;
    uint16_t software_version;
    uint16_t hardware_version;
    uint16_t uuid;
    uint8_t error_code;
  };

  struct device_config_t
  {
    // 模式设置
    uint8_t mode_id;         // 当前模式编号
    uint16_t time_min;       // 当前模式时长（分钟）
    uint16_t count_time_min; // 当前模式预约时长（分钟）
    uint16_t temp_c;         // 当前模式温度（℃）
    uint8_t time_step;       // 时长步长（分钟）
    uint8_t temp_step;       // 温度步长（℃）

    // 状态标志
    bool is_running;   // 是否正在运行
    bool is_countdown; // 是否正在运行
    bool def_mode;     // 是否一键启动默认模式
    bool long_cancel;  // 是否长按取消
    bool allow_edit;   // 是否允许中途修改参数
    bool is_close;

    // 显示与提示
    bool beep;      // 是否有提示音
    bool show_mode; // 是否显示当前模式
    bool show_left; // 是否显示剩余时长
    bool show_temp; // 是否显示实时温度

    // 电池信息
    bool bat_ok;   // 是否检测到电池连接
    float bat_v;   // 电池电压
    float bat_i;   // 电池电流
    float bat_soc; // 电池电量百分比
    float heater_i;

    // 电池事件
    bool bat_add; // 增加电池
    bool bat_rm;  // 移除电池

    // 新增字段
    uint16_t remain_sec;  // 剩余时间，单位秒
    uint8_t fault_flag;   // 故障码，0正常，非0故障
    bool temp_sensor_err; // 温度传感器异常
    bool batt_sensor_err; // 电池传感器异常

    float temperature;
  };
  typedef struct
  {
    struct base_dev_t base;
    struct ctl_dev_t ctl_dev;
    struct device_config_t dev_conf;
    struct rt_event btn_event; // 按键 事件控制块
    struct rt_messagequeue msg_queue;
    struct rt_messagequeue bt_rev_msg_queue;
    struct rt_event ui_event;
  } env_t;
  extern env_t env;

#define EVENT_UPDATA_SLIDER1_SCT (1 << 1)
#define EVENT_UPDATA_SLIDER2_SCT (1 << 2)
#define EVENT_UPDATA_SW_SCT (1 << 3)
#define EVENT_UPDATA_MODE_SCT (1 << 4)
#ifdef __cplusplus
}
#endif

#endif
