#ifndef _APP_H_
#define _APP_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "platform.h"
#include "rtthread.h"


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
    struct device_config_t dev_conf;
    /* 按键 事件控制块 */
    struct rt_event btn_event;
  } env_t;
  extern env_t env;

#ifdef __cplusplus
}
#endif

#endif
