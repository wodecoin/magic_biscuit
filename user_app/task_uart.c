#include "app.h"

#define MSG_QUEUE_CNT 4

static struct rt_messagequeue uartx_mq;
struct bt_data
{
	uint8_t *pbuf;
	uint16_t len;
};
static struct bt_data evt;
static frame_t recv_frame;
static struct bt_data msg_pool[MSG_QUEUE_CNT];

#define QUEUE_CNT 8 // 最大消息数
static display_msg_t dis_pool[QUEUE_CNT];

#define LOG_TAG "BT_REV"
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>

#define RT_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static uint32_t tlv_get_u32(const tlv_t *tlv)
{
	uint32_t val = 0;
	for (int j = 0; j < tlv->length && j < 4; j++)
	{
		val = (val << 8) | tlv->value[j]; // 高字节在前
	}
	return val;
}
static uint32_t tlv_get_u32_le(const tlv_t *tlv)
{
	uint32_t val = 0;
	for (int j = tlv->length - 1; j >= 0; j--)
	{
		val = (val << 8) | tlv->value[j];
	}
	return val;
}


//=======================
// TLV → 内存映射定义
//=======================
typedef struct
{
	uint8_t type; // TLV 类型
	void *target; // 对应目标变量地址
} tlv_map_t;

//=== 控制响应 TLV 映射 ===
static tlv_map_t ctrl_tlv_map[] =
	{
		{TLV_HEAT_TIME, &env.ctl_dev.set_temp},
		{TLV_MODE_TEMP, &env.ctl_dev.set_temp},
};

//=== 查询响应 TLV 映射 ===
// 代码更模块化、可维护性更强(方便增加/修改 TLV)
static tlv_map_t query_tlv_map[] =
	{
		{TLV_AREA1_TEMP, &env.ctl_dev.real_temp},
		// {TLV_AREA2_TEMP, &env.ctl_dev.area_temp[1]},
		// {TLV_AREA3_TEMP, &env.ctl_dev.area_temp[2]},
		// {TLV_AREA4_TEMP, &env.ctl_dev.area_temp[3]},
		// {TLV_AREA1_TIME, &env.ctl_dev.area_time[0]},
		// {TLV_AREA2_TIME, &env.ctl_dev.area_time[1]},
		// {TLV_AREA3_TIME, &env.ctl_dev.area_time[2]},
		// {TLV_AREA4_TIME, &env.ctl_dev.area_time[3]},
		{TLV_KPA, &env.ctl_dev.kpa},
		{TLV_RH, &env.ctl_dev.rh_value},
};

//=======================
// 通用 TLV 解析函数 by 表映射
//=======================
static void parse_tlv_by_map(const frame_t *frame, const tlv_map_t *map, size_t map_size)
{
	for (int i = 0; i < frame->tlv_count; i++)
	{
		const tlv_t *tlv = &frame->tlvs[i];
		uint32_t val = tlv_get_u32(tlv);

		for (size_t j = 0; j < map_size; j++)
		{
			if (tlv->type == map[j].type)
			{
				*(uint32_t *)map[j].target = val;
				LOG_D("[TLV] type=0x%02X val=%d", tlv->type, val);
				break;
			}
		}
	}
}

//=======================
// 主线程函数
//=======================
void thread_uart_task_entry(void *parameter)
{
	ui_msg_t msg;
	display_msg_t dmsg;
	struct bt_data data;

	// 初始化显示消息队列
	rt_mq_init(&env.bt_rev_msg_queue,
			   "bt_msgq",
			   dis_pool,
			   sizeof(display_msg_t),
			   sizeof(dis_pool),
			   RT_IPC_FLAG_FIFO);

	rt_thread_mdelay(1000); // 等待系统初始化完成

	while (1)
	{
		// 非阻塞接收 UART 数据
		if (rt_mq_recv(&uartx_mq, &data, sizeof(struct bt_data), RT_WAITING_NO) == RT_EOK)
		{
			print_hex_buffer("uart4_rev", data.pbuf, data.len);
			rt_memset(&recv_frame, 0, sizeof(frame_t));

			if (parse_frame(data.pbuf, &recv_frame) == 0)
			{
				if (handle_frame(&recv_frame) == 0)
				{
					switch (recv_frame.frame_type)
					{
					case FRAME_CTRL_RESP:
						parse_tlv_by_map(&recv_frame, ctrl_tlv_map, RT_ARRAY_SIZE(ctrl_tlv_map));
						LOG_I("receive ctrl resp, set_temp=%d", env.ctl_dev.set_temp);
						break;

					case FRAME_QUERY_RESP:
						parse_tlv_by_map(&recv_frame, query_tlv_map, RT_ARRAY_SIZE(query_tlv_map));
						dmsg.widget_id = WIDGET_AIR_FRY_DISPLAY;
						rt_mq_send(&env.bt_rev_msg_queue, &dmsg, sizeof(dmsg));
						break;

					default:
						break;
					}
				}
			}
		}

		rt_thread_mdelay(50);
	}
}

/* ----------------- module init ----------------- */

#define MSG_QUEUE_SIZE 32 // 最多缓存 32 字节

void uartx_idle_callback(uint8_t *pBuf, uint16_t len)
{
	struct bt_data bt_msg;
	bt_msg.pbuf = pBuf;
	bt_msg.len = len;
	rt_mq_send(&uartx_mq, &bt_msg, sizeof(struct bt_data));
}

int uart_module_init(void)
{
	uart_config(UART4,
				115200,
				GPIOC,
				GPIO_Pin_0, GPIO_Pin_1,
				GPIO_PinSource0, GPIO_PinSource1,
				GPIO_AF_8,
				UART_IDLE,
				UART4_IRQn);

	rt_mq_init(&uartx_mq, "mq_uart",
			   &msg_pool[0],
			   sizeof(struct bt_data),
			   sizeof(msg_pool),
			   RT_IPC_FLAG_FIFO);

	uart4.idle_callback = uartx_idle_callback;

	return 0;
}

INIT_APP_EXPORT(uart_module_init);

#include "stdlib.h"
static void ui_updata(int argc, char **argv)
{
	display_msg_t dmsg;
	dmsg.widget_id = WIDGET_AIR_FRY_DISPLAY;
	env.ctl_dev.real_temp = 50.1f;
	env.ctl_dev.kpa = 50.2f;
	env.ctl_dev.rh_value = 50.3f;

	rt_err_t ret = rt_mq_send(&env.bt_rev_msg_queue, &dmsg, sizeof(dmsg));
	if (ret == RT_EOK)
		rt_kprintf("mq_test: send widget_id=%d OK\n", dmsg.widget_id);
	else
		rt_kprintf("mq_test: send failed! ret=%d\n", ret);
}

/* 导出 msh 命令 */
MSH_CMD_EXPORT(ui_updata, send test message to UI queue);