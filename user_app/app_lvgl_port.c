#include "platform.h"
#include "lvgl.h"

#include "amoled_touch.h"
#define USE_LINE_MODE 1
#define AMOLED_MAX_WIDTH 460
#define AMOLED_MAX_HEIGHT 460
#define LOG_TAG "lvgl"
#define LOG_LVL LOG_LVL_DBG
#define AMOLED_FLUSH_ROWS 2
extern void amoled_write_block(uint16_t x, uint16_t y,
                               uint16_t width, uint16_t height,
                               const uint16_t *color, uint32_t buf_size);
static rt_mutex_t g_disp_mutex = RT_NULL;
void amoled_flush_area(const lv_area_t *area, lv_color_t *color_p)
{
    if (!area || !color_p)
        return;

    int32_t x1 = (area->x1 < 0) ? 0 : area->x1;
    int32_t y1 = (area->y1 < 0) ? 0 : area->y1;
    int32_t x2 = (area->x2 >= AMOLED_MAX_WIDTH) ? (AMOLED_MAX_WIDTH - 1) : area->x2;
    int32_t y2 = (area->y2 >= AMOLED_MAX_HEIGHT) ? (AMOLED_MAX_HEIGHT - 1) : area->y2;

    uint16_t width = x2 - x1 + 1;
    uint16_t height = y2 - y1 + 1;
		
//		LOG_I("Flush area: x1=%d y1=%d x2=%d y2=%d (w=%d h=%d)",
//					 area->x1, area->y1, area->x2, area->y2,
//					 width, height);
		
    if (width <= 0 || height <= 0)
    {
        LOG_E("Invalid area dimensions: width=%d, height=%d", width, height);
        return;
    }

    rt_mutex_take(g_disp_mutex, RT_WAITING_FOREVER);
		if(height <= 4)// 全局刷新
		{
				amoled_write_block(
						x1, y1,
						width, height,
						(uint16_t *)color_p,
						height * width * 2);
		}
		else // 局部
		{
				for (int32_t y = y1; y <= y2; y += AMOLED_FLUSH_ROWS) {
						uint16_t h = (y + AMOLED_FLUSH_ROWS - 1 <= y2) ? AMOLED_FLUSH_ROWS : (y2 - y + 1);
						uint32_t offset = (y - area->y1) * (area->x2 - area->x1 + 1);
						lv_color_t *row_buffer = color_p + offset;

//						LOG_I("Flush rows: x1=%d y=%d x2=%d (w=%d h=%d)", x1, y, x2, width, h);

						amoled_write_block(x1, y, width, h, (uint16_t *)row_buffer, width * h * 2);
				}
		}
    rt_mutex_release(g_disp_mutex);

//    lv_disp_flush_ready(lv_disp_get_default()->driver);
}
#define amoled_te_port GPIOB
#define amoled_te_pin GPIO_Pin_2 // TE (可选)

void te_wait_cb(lv_disp_drv_t *disp_drv)
{
    uint32_t timeout = 20000;   // 20ms超时 (适用于60Hz，约16ms每帧)
    uint32_t delay_us = 2;      // 减少到2us以实现更精细的轮询
    static uint8_t last_te = 0; // 跟踪上一次TE状态以检测边沿
    uint8_t curr_te;

    // 使用更精细的粒度轮询TE上升沿
    while (timeout > 0)
    {
        curr_te = GPIO_ReadInputDataBit(amoled_te_port, amoled_te_pin);
        if (curr_te == 1 && last_te == 0)
        { // 检测上升沿
            // 等待V-Blank稳定 (根据AMOLED数据手册调整，例如20-50us)
            rt_hw_us_delay(10); // 增加延迟以确保稳定性
            break;              // 在检测到上升沿时退出
        }
        last_te = curr_te;
        rt_hw_us_delay(delay_us);
        timeout -= delay_us;
    }

    if (timeout <= 0)
    {
        LOG_E("TE poll timeout!");                                 // TE轮询超时
        rt_kprintf("TE state: %d (last: %d)\n", curr_te, last_te); // 调试TE状态
    }
}

#include "lvgl.h"
#include "rtthread.h"

#define TEST_WIDTH 40
#define TEST_HEIGHT 40

static uint16_t test_buf[TEST_WIDTH * TEST_HEIGHT];

static void msh_amoled_test(int argc, char **argv)
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
    amoled_flush_area(&area, (lv_color_t *)test_buf);
    rt_thread_mdelay(1);

    //    rt_kprintf("amoled_flush_area test done.\r\n");
}

MSH_CMD_EXPORT(msh_amoled_test, Test AMOLED flush_area with a small rectangle);

void amoled_touchpad_init(void)
{
    /*Your code comes here*/
    // 若i2c_config功能需保留，请在平台初始化阶段调用
    uint8_t irq_ctl = CST820_IRQ_CTL_EN_CHANGE;
    i2c_write_bytes(&i2c1, CST820_I2C_ADDR, CST820_REG_IRQ_CTL, &irq_ctl, 1); // 设置触摸状态变化发低脉冲
}

void amoled_touch_get_xy(lv_coord_t *x, lv_coord_t *y)
{
    uint8_t xy_buf[4] = {0};

    rt_mutex_take(g_disp_mutex, RT_WAITING_FOREVER);
    i2c_read_bytes(&i2c1,  CST820_I2C_ADDR, CST820_REG_XPOS_H, xy_buf, 4);
    rt_mutex_release(g_disp_mutex);

    uint8_t xh = xy_buf[0], xl = xy_buf[1];
    uint8_t yh = xy_buf[2], yl = xy_buf[3];
    lv_coord_t curr_x = CST820_GET_XPOS(xh, xl);
    lv_coord_t curr_y = CST820_GET_YPOS(yh, yl);

    if (x)
        *x = curr_x;
    if (y)
        *y = curr_y;

    rt_kprintf("xpos %d, ypos %d\r\n", curr_x, curr_y);
}

bool amoled_touchpad_is_pressed(void)
{
    uint8_t reg_buf[2] = {0};

    rt_mutex_take(g_disp_mutex, RT_WAITING_FOREVER);
    i2c_read_bytes(&i2c1, CST820_I2C_ADDR, CST820_REG_FINGER_NUM, reg_buf, 2);
    rt_mutex_release(g_disp_mutex);

    uint8_t finger_num = reg_buf[0];
    uint8_t event_flag = (reg_buf[1] >> 6) & 0x03;

    if (finger_num == CST820_FINGER_NUM_ONE)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int disp_touch_mutex_init(void)
{
    if (g_disp_mutex == RT_NULL)
    {
        g_disp_mutex = rt_mutex_create("disp_mutex", RT_IPC_FLAG_PRIO);
        if (g_disp_mutex == RT_NULL)
        {
            rt_kprintf("Failed to create g_disp_mutex\n");
        }
    }
		return 0;
}

INIT_APP_EXPORT(disp_touch_mutex_init);
