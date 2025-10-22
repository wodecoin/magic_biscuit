#ifndef TLV_PROTOCOL_H
#define TLV_PROTOCOL_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#define MAX_TLV 10
    // 公共 TLV 类型
    typedef enum
    {
        TLV_CURR = 0x01,
        TLV_VOLT = 0x02,
        TLV_POWER = 0x03,
        TLV_TEMP = 0x04,
        TLV_STATUS = 0x05,
        TLV_ERR = 0x06,
        TLV_RUNTIME = 0x07,
        TLV_DEV_ID = 0x08,
        TLV_FW_VER = 0x09,
        TLV_HW_VER = 0x0A,
        TLV_SWITCH = 0x20,
        TLV_MODE = 0x70,
        TLV_SET_TEMP = 0x71,
        TLV_SET_PWR = 0x72,
        TLV_MODE_SEL = 0x73,
        TLV_PAN_DET = 0x74,
        TLV_OFF_TIME = 0x75,
        TLV_TIMER = 0x76
    } tlv_type_t;

    typedef enum
    {
        FRAME_QUERY_REQ = 0xF0,
        FRAME_QUERY_RESP = 0xA0,
        FRAME_CTRL_REQ = 0xF1,
        FRAME_CTRL_RESP = 0xA1
    } frame_type_t;
    typedef enum
    {
        TYPE_BAT_OUTPUT_ON = 0xB0,
        TYPE_TYPEC_OUTPUT = 0xB1,
        TYPE_BAT_OUTPUT_OFF = 0xB2
    } magic_bat_type_t;
    // 电磁炉控制参数
    typedef enum
    {
        TYPE_COOKER_CURRENT_MODE = 0x70,
        TYPE_COOKER_SET_TEMP = 0x71,
        TYPE_COOKER_SET_POWER = 0x72,
        TYPE_COOKER_MODE_SELECT = 0x73,
        TYPE_COOKER_POT_DETECT = 0x74,
        TYPE_COOKER_OFF_TIMER = 0x75,
        TYPE_COOKER_APPOINT_TIME = 0x76,
        TYPE_COOKER_START = 0x77,
    } cooker_type_t;

    typedef enum
    {
        TLV_COOK_MODE = 0x80,    // 烹饪模式
        TLV_HEAT_STATE = 0x81,   // 加热状态
        TLV_HEAT_TIME = 0x82,    // 加热时间
        TLV_RESERVE_TIME = 0x83, // 预约时间
        TLV_MODE_TEMP = 0x84,    // 模式温度
        TLV_MODE_POT = 0x85,     // 模式锅识别
        TLV_AREA1_TEMP = 0x86,   // 区域1温度
        TLV_SW = 0x88,           // 开关
        TLV_KPA = 0x8D,          // 大气压
        TLV_RH = 0x8E,           // 湿度

        // =====================
        // 新增：多分区温度 / 时间 / 预约 / 执行
        // =====================
        // --- 查询分区温度 (单位: 摄氏度) ---
        TLV_QUERY_ZONE1_TEMP = 0x50, // 查询分区1温度 (uint16_t)
        TLV_QUERY_ZONE2_TEMP = 0x51, // 查询分区2温度 (uint16_t)
        TLV_QUERY_ZONE3_TEMP = 0x52, // 查询分区3温度 (uint16_t)
        TLV_QUERY_ZONE4_TEMP = 0x53, // 查询分区4温度 (uint16_t)

        // --- 查询分区剩余时间 (单位: 分钟) ---
        TLV_QUERY_ZONE1_REMAIN = 0x54, // 查询分区1剩余时间 (uint16_t)
        TLV_QUERY_ZONE2_REMAIN = 0x55, // 查询分区2剩余时间 (uint16_t)
        TLV_QUERY_ZONE3_REMAIN = 0x56, // 查询分区3剩余时间 (uint16_t)
        TLV_QUERY_ZONE4_REMAIN = 0x57, // 查询分区4剩余时间 (uint16_t)

        // --- 设置分区温度 (单位: 摄氏度) ---
        TLV_SET_ZONE1_TEMP = 0x58, // 设置分区1温度 (uint16_t)
        TLV_SET_ZONE2_TEMP = 0x59, // 设置分区2温度 (uint16_t)
        TLV_SET_ZONE3_TEMP = 0x5A, // 设置分区3温度 (uint16_t)
        TLV_SET_ZONE4_TEMP = 0x5B, // 设置分区4温度 (uint16_t)

        // --- 设置分区时间 (单位: 分钟) ---
        TLV_SET_ZONE1_TIME = 0x5C, // 设置分区1时间 (uint16_t)
        TLV_SET_ZONE2_TIME = 0x5D, // 设置分区2时间 (uint16_t)
        TLV_SET_ZONE3_TIME = 0x5E, // 设置分区3时间 (uint16_t)
        TLV_SET_ZONE4_TIME = 0x5F, // 设置分区4时间 (uint16_t)

        // --- 分区预约时间 (单位: 分钟) ---
        TLV_RESERVE_ZONE1_TIME = 0x60, // 分区1预约时间 (uint16_t)
        TLV_RESERVE_ZONE2_TIME = 0x61, // 分区2预约时间 (uint16_t)
        TLV_RESERVE_ZONE3_TIME = 0x62, // 分区3预约时间 (uint16_t)
        TLV_RESERVE_ZONE4_TIME = 0x63, // 分区4预约时间 (uint16_t)

        // --- 执行设置参数 ---
        TLV_EXEC_PARAM = 0x64, // 执行设置参数 (uint8_t, 1=开始执行, 0=停止所有任务)

    } air_fryer_type_t;

    typedef enum
    {
        TLV_CURR_MODE = 0x90,        // 当前模式
        TLV_SWITCH_CTRL = 0x91,      // 开关控制
        TLV_MODE_SET = 0x98,         // 模式设置
        TLV_QUERY_STATUS = 0x93,     // 查询当前设备状态
        TLV_QUERY_PRESSURE = 0x94,   // 查询当前环境大气压
        TLV_QUERY_BOIL_POINT = 0x95, // 查询当前沸点
        TLV_QUERY_TILT_ANGLE = 0x96, // 查询当前倾斜角度
        TLV_QUERY_TARGET_TEMP = 0x97 // 查询目标温度
    } mug_type_t;
    typedef struct
    {
        uint8_t type;
        uint8_t length;
        uint8_t value[4]; // 可扩展
    } tlv_t;

    typedef struct
    {
        uint8_t header;
        uint8_t length;
        frame_type_t frame_type;
        uint8_t dev_type;
        uint8_t tlv_count;
        tlv_t tlvs[MAX_TLV];
    } frame_t;

    extern int build_frame(frame_t *frame, frame_type_t ftype, uint8_t dev_type, tlv_t *tlvs, uint8_t tlv_count, uint8_t *buf);
    extern int parse_frame(uint8_t *buf, frame_t *frame);
    extern int handle_frame(const frame_t *frame);
    extern void example_query(void);
    extern void example_ctrl(void);
    extern void print_hex_buffer(const char *tag, const uint8_t *buf, size_t len);
#ifdef __cplusplus
}
#endif
#endif // TLV_PROTOCOL_H