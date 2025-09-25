#include "platform.h"
#include "lvgl.h"
#include "amoled_touch.h"
#define USE_LINE_MODE 1
#define AMOLED_MAX_WIDTH   460
#define AMOLED_MAX_HEIGHT  460
#define LOG_TAG "lvgl"
#define LOG_LVL LOG_LVL_DBG
extern void amoled_write_block(uint16_t x, uint16_t y,
                        uint16_t width, uint16_t height,
                        const uint16_t *color, uint32_t buf_size);

void amoled_flush_area(const lv_area_t *area, lv_color_t *color_p)
{
    if (!area || !color_p) return;
//    LOG_I("area: x1=%d, y1=%d, x2=%d, y2=%d", area->x1, area->y1, area->x2, area->y2);

    // 裁剪区域，防止越界
    int32_t x1 = (area->x1 < 0) ? 0 : area->x1;
    int32_t y1 = (area->y1 < 0) ? 0 : area->y1;
    int32_t x2 = (area->x2 >= AMOLED_MAX_WIDTH) ? (AMOLED_MAX_WIDTH - 1) : area->x2;
    int32_t y2 = (area->y2 >= AMOLED_MAX_HEIGHT) ? (AMOLED_MAX_HEIGHT - 1) : area->y2;

    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;

    // 检查宽度和高度
    if (width <= 0 || height <= 0) {
        LOG_E("Invalid area dimensions: width=%d, height=%d", width, height);
        return;
    }

//    LOG_I("color_p address: %p", color_p);
//    for (int i = 0; i < 5 && i < width * height; i++) { // 打印前 5 个像素或全部（取最小值）
//        rt_kprintf("color_p[%d]: 0x%04x ", i, ((uint16_t*)color_p)[i]);
//    }
//		LOG_I(" ");

		// 静态互斥锁，初始化一次
    static rt_mutex_t disp_mutex = NULL;
    if (disp_mutex == NULL) {
        disp_mutex = rt_mutex_create("disp_mutex", RT_IPC_FLAG_PRIO);
        if (disp_mutex == RT_NULL) {
            LOG_E("Failed to create disp_mutex");
            return;
        }
    }
		// 获取互斥锁
    if (rt_mutex_take(disp_mutex, RT_WAITING_FOREVER) != RT_EOK) {
        LOG_E("Failed to take disp_mutex");
        return;
    }

    amoled_write_block(
        x1,
        y1,
        width,
        height,
        (uint16_t*)color_p,
        height * width * 2
    );

    // 释放互斥锁
    if (rt_mutex_release(disp_mutex) != RT_EOK) {
        LOG_E("Failed to release disp_mutex");
    }
}

#include "lvgl.h"
#include "rtthread.h"

#define TEST_WIDTH  40
#define TEST_HEIGHT 40

static uint16_t test_buf[TEST_WIDTH * TEST_HEIGHT];

static void msh_amoled_test(int argc, char** argv)
{
    // 初始化屏幕
    amoled_init();
//    amoled_clear(0x0000); // 黑色

    // 填充颜色（示例：红色）
    for (int i = 0; i < TEST_WIDTH * TEST_HEIGHT; i++)
    {
        test_buf[i] = 0xF800; // RGB565 红色
    }

    // 构造 LVGL 区域
    lv_area_t area;
    area.x1 = 50;
    area.y1 = 50;
    area.x2 = 50 + TEST_WIDTH - 1;
    area.y2 = 50 + TEST_HEIGHT - 1;

    // 调用 flush_area
    amoled_flush_area(&area, (lv_color_t*)test_buf);
		rt_thread_mdelay(1);

//    rt_kprintf("amoled_flush_area test done.\r\n");
}

MSH_CMD_EXPORT(msh_amoled_test, Test AMOLED flush_area with a small rectangle);

void amoled_touchpad_init(void)
{
    /*Your code comes here*/
      // 若i2c_config功能需保留，请在平台初始化阶段调用
    uint8_t irq_ctl = CST820_IRQ_CTL_EN_CHANGE;
    i2c_write_bytes(CST820_I2C_ADDR, CST820_REG_IRQ_CTL, &irq_ctl, 1); // 设置触摸状态变化发低脉冲
}

void amoled_touch_get_xy(lv_coord_t * x, lv_coord_t * y)
{
    /*Your code comes here*/
        uint8_t xy_buf[4] = {0};
        i2c_read_bytes(CST820_I2C_ADDR, CST820_REG_XPOS_H, xy_buf, 4);
        // 3. 解析12位坐标（文档1-30、1-39、1-48、1-54）
        uint8_t xh = xy_buf[0], xl = xy_buf[1];
        uint8_t yh = xy_buf[2], yl = xy_buf[3];
        lv_coord_t curr_x = CST820_GET_XPOS(xh, xl); // 组合X坐标（高4位<<8 + 低8位）
        lv_coord_t curr_y = CST820_GET_YPOS(yh, yl); // 组合Y坐标

        // 4. 更新缓存并输出坐标
        if(x != NULL) *x = curr_x;
        if(y != NULL) *y = curr_y;
                rt_kprintf("xpos 0x%02X,0x%02X\r\n",curr_x,curr_y);
}

bool amoled_touchpad_is_pressed(void)
{
    /*Your code comes here*/
        uint8_t reg_buf[2] = {0};
    i2c_read_bytes(CST820_I2C_ADDR, CST820_REG_FINGER_NUM, reg_buf, 2);


    // 3. 解析文档寄存器数据
    uint8_t finger_num = reg_buf[0];          // 解析手指个数（文档1-14）
    uint8_t event_flag = (reg_buf[1] >> 6) & 0x03; // 解析触摸状态（文档1-23）

    // 4. 按压判定：1个手指 + 按下（00）或保持/移动（10）→ 映射为LV_INDEV_STATE_PRESSED
    if (finger_num == CST820_FINGER_NUM_ONE ) //&& (event_flag == CST820_EVENT_PRESS || event_flag == CST820_EVENT_HOLD_MOVE)
    {
        return true;
    }
    // 释放判定：无手指/抬起/保留 → 映射为LV_INDEV_STATE_RELEASED
    else
    {
        return false;
    }
 
}
