
#include <stdio.h>
#include <string.h>

#include "tlv_protocol.h"

#define MAX_FRAME_LEN 64
#define LOG_TAG "BT_REV"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#define FRAME_HEADER 0xAA
typedef void (*frame_type_callback_t)(const frame_t *frame);
#define MAX_FRAME_CALLBACKS 8
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
static struct
{
    uint8_t frame_type;
    frame_type_callback_t cb;
} g_frame_callbacks[MAX_FRAME_CALLBACKS];
static int g_frame_cb_count = 0;

void print_hex_buffer(const char *tag, const uint8_t *buf, size_t len)
{
    rt_kprintf("[%s] Hex data (%d bytes): ", tag, len);
    for (size_t i = 0; i < len; i++)
    {
        rt_kprintf("%02X ", buf[i]);
    }
    rt_kprintf("\r\n");
}
int register_frame_callback(uint8_t frame_type, frame_type_callback_t cb)
{
    if (g_frame_cb_count >= MAX_FRAME_CALLBACKS)
        return -1;
    g_frame_callbacks[g_frame_cb_count].frame_type = frame_type;
    g_frame_callbacks[g_frame_cb_count].cb = cb;
    g_frame_cb_count++;
    return 0;
}

// ------------------------------------
// 校验和计算（包含帧头）
// ------------------------------------
uint8_t calc_checksum(uint8_t *buf, uint8_t len)
{
    uint8_t sum = 0;
    for (int i = 0; i < len; i++)
        sum += buf[i];
    return sum;
}

// ------------------------------------
// TLV 打印
// ------------------------------------
void tlv_print(const tlv_t *tlv)
{
    LOG_I("TLV Type: %02X, Len: %d, Value:", tlv->type, tlv->length);
    print_hex_buffer(LOG_TAG, tlv->value, tlv->length);
}

// ------------------------------------
// 构建帧（带校验和）
// ------------------------------------
int build_frame(frame_t *frame, frame_type_t ftype, uint8_t dev_type,
                tlv_t *tlvs, uint8_t tlv_count, uint8_t *buf)
{
    uint8_t idx = 0;
    frame->header = FRAME_HEADER;
    frame->frame_type = ftype;
    frame->dev_type = dev_type;
    frame->tlv_count = 0;

    // length = frame_type + dev_type + length本身
    frame->length = 1 + 1 + 1;

    // 遍历 TLV
    for (int i = 0; i < tlv_count; i++)
    {
        // 控制帧长度为0的 TLV 不发送
        if (ftype == FRAME_CTRL_REQ && tlvs[i].length == 0)
            continue;

        frame->tlvs[frame->tlv_count++] = tlvs[i];
        frame->length += 2 + tlvs[i].length; // type+len+value
        // 查询帧长度为0的 TLV 只加 type
        if (ftype == FRAME_QUERY_REQ && tlvs[i].length == 0)
            frame->length -= 1;
    }

    // 校验和也要算进 length
    frame->length += 1;

    // 打包帧
    buf[idx++] = frame->header;
    buf[idx++] = frame->length; // 长度包含自己，不含header
    buf[idx++] = frame->frame_type;
    buf[idx++] = frame->dev_type;

    for (int i = 0; i < frame->tlv_count; i++)
    {
        buf[idx++] = frame->tlvs[i].type;
        if (!(ftype == FRAME_QUERY_REQ && frame->tlvs[i].length == 0))
        {
            buf[idx++] = frame->tlvs[i].length;
            memcpy(&buf[idx], frame->tlvs[i].value, frame->tlvs[i].length);
            idx += frame->tlvs[i].length;
        }
    }

    // 计算校验和（不包含校验和本身）
    uint8_t checksum = calc_checksum(buf, idx);
    buf[idx++] = checksum;

    return idx; // 返回帧总长度（含header和校验）
}

// ------------------------------------
// 解析帧并验证校验和
// ------------------------------------
int parse_frame(uint8_t *buf, frame_t *frame)
{
    if (buf[0] != FRAME_HEADER)
    {
        LOG_I("Header error! recv:%02X\n", buf[0]);
        return -1;
    }
    uint8_t frame_len = buf[1]; // 长度字段（含自身、不含header）
    if (frame_len < 4)          // 至少要有 length + ftype + dev_type + checksum
    {
        LOG_E("Length error! len:%d\n", frame_len);
        return -2;
    }
    print_hex_buffer(LOG_TAG, buf, frame_len + 1); // +1 包含 header
    // 校验和校验：范围是 header(AA) 到校验和前一个字节
    uint8_t recv_checksum = buf[frame_len];
    uint8_t calc = calc_checksum(buf, frame_len);
    if (calc != recv_checksum)
    {
        LOG_E("Checksum error! len = %d,recv:%02X calc:%02X\n", frame_len, recv_checksum, calc);
        return -3;
    }

    // 解析固定字段
    frame->header = buf[0];
    frame->length = frame_len;
    frame->frame_type = buf[2];
    frame->dev_type = buf[3];

    // 解析TLV
    uint8_t idx = 4;
    frame->tlv_count = 0;
    while (idx < frame_len + 1 - 1) // 减去header1字节、减去checksum1字节
    {
        tlv_t tlv;
        tlv.type = buf[idx++];
        // 查询帧 TLV 长度可能为0 → 不包含length字段
        if (frame->frame_type == FRAME_QUERY_REQ && idx <= frame_len + 1 - 1)
        {
            tlv.length = 0;
        }
        else if (frame->frame_type == FRAME_QUERY_REQ && buf[idx] == 0)
        {
            tlv.length = 0;
            idx++; // 只跳过len=0字节，不取value
        }
        else
        {
            tlv.length = buf[idx++];
            memcpy(tlv.value, &buf[idx], tlv.length);
            idx += tlv.length;
        }
        frame->tlvs[frame->tlv_count++] = tlv;
    }

    return 0; // 成功
}

// ------------------------------------
// 回调处理
// ------------------------------------
int handle_frame(const frame_t *frame)
{
    LOG_I("Frame Type: %02X, Dev Type: %02X, TLV Count: %d", frame->frame_type, frame->dev_type, frame->tlv_count);
    for (int i = 0; i < frame->tlv_count; i++)
    {
        tlv_print(&frame->tlvs[i]);
    }
    LOG_I("============ handele over ============\n");

    // -------------------------
    // 按注册回调轮询
    // -------------------------
    for (int i = 0; i < g_frame_cb_count; i++)
    {
        if (g_frame_callbacks[i].frame_type == frame->frame_type)
        {
            g_frame_callbacks[i].cb(frame); // 传整个 frame
        }
    }
    return 0;
}

void type_01_callback(const frame_t *frame)
{
    // 用户自定义处理逻辑
    LOG_I("type_01_callback");
}
// ------------------------------------
// 示例：查询帧
// ------------------------------------
void example_query(void)
{
    frame_t frame;
    tlv_t tlvs[3];
    tlvs[0].type = TLV_CURR;
    tlvs[0].length = 0;
    tlvs[1].type = TLV_VOLT;
    tlvs[1].length = 0;
    tlvs[2].type = TLV_POWER;
    tlvs[2].length = 0;

    uint8_t buf[64];
    int len = build_frame(&frame, FRAME_QUERY_REQ, 0x01, tlvs, 3, buf);

    LOG_I("Query Frame: ");
    print_hex_buffer(LOG_TAG, buf, len);

    // register_frame_callback(0x01, type_01_callback);
    // 模拟接收应答
    uint8_t resp[] = {0xAA, 14, 0xA0, 0x01, 0x01, 0x02, 0x0C, 0x34, 0x01, 0x01, 0x0C, 0x01, 0x01, 0x0C, 0xB8};
    frame_t recv_frame;
    if (parse_frame(resp, &recv_frame) == 0)
        handle_frame(&recv_frame);
}

// ------------------------------------
// 示例：控制帧
// ------------------------------------
void example_ctrl(void)
{
    frame_t frame;
    tlv_t tlvs[2];
    tlvs[0].type = TLV_SWITCH;
    tlvs[0].length = 1;
    tlvs[0].value[0] = 1;
    tlvs[1].type = TLV_MODE;
    tlvs[1].length = 0; // 不发送

    uint8_t buf[64];
    int len = build_frame(&frame, FRAME_CTRL_REQ, 0x01, tlvs, 2, buf);

    LOG_I("Ctrl Frame: ");
    print_hex_buffer(LOG_TAG, buf, len);
    // 模拟接收应答
    uint8_t resp[] = {0xAA, 0x07, 0xA1, 0x01, 0x20, 0x01, 0x01, 0x75};
    frame_t recv_frame;
    if (parse_frame(resp, &recv_frame) == 0)
        handle_frame(&recv_frame);
}
