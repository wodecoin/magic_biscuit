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

void thread_uart_task_entry(void *parameter)
{
	ui_msg_t msg;
	display_msg_t dmsg;
	struct bt_data data;

	rt_mq_init(&env.bt_rev_msg_queue,
			   "bt_msgq",
			   dis_pool,
			   sizeof(display_msg_t),
			   sizeof(dis_pool),
			   RT_IPC_FLAG_FIFO);
	rt_thread_mdelay(1000); // 等待系统初始化完成
	while (1)
	{
		// 阻塞等待消息（一个字节）
		if (rt_mq_recv(&uartx_mq, &data, sizeof(struct bt_data), RT_WAITING_NO) == RT_EOK)
		{
			print_hex_buffer("uart4_rev", data.pbuf, data.len);
			rt_memset(&recv_frame, 0, sizeof(frame_t)); // 清空接收帧
			if (parse_frame(evt.pbuf, &recv_frame) == 0)
			{
				if (handle_frame(&recv_frame) == 0)
				{
					if (recv_frame.frame_type == FRAME_CTRL_RESP)
					{
						for (int i = 0; i < recv_frame.tlv_count; i++)
						{
							tlv_t *tlv = &recv_frame.tlvs[i];
							switch (tlv->type)
							{
							case TLV_HEAT_TIME:
								env.ctl_dev.set_temp = tlv->value[i];
								break;
							case TLV_MODE_TEMP:
								env.ctl_dev.set_temp = tlv->value[i];
								break;
							default:
								break;
							}
						}
						LOG_I("receive ctrl resp %d  %d", env.ctl_dev.set_temp, recv_frame.tlv_count);
					}
					else if (recv_frame.frame_type == FRAME_QUERY_RESP)
					{
						for (int i = 0; i < recv_frame.tlv_count; i++)
						{
							tlv_t *tlv = &recv_frame.tlvs[i];
							switch (tlv->type)
							{
							case TLV_AREA1_TEMP:
								env.ctl_dev.real_temp = tlv->value[i];
								break;
							case TLV_KPA:
								env.ctl_dev.kpa = tlv->value[i];
								break;
							case TLV_RH:
								env.ctl_dev.rh_value = tlv->value[i];
								break;
							default:
								break;
							}
						}
						dmsg.widget_id = WIDGET_AIR_FRY_DISPLAY;
						rt_mq_send(&env.bt_rev_msg_queue, &dmsg, sizeof(dmsg));
					}
				}
			}
		}
		//		UART_SendBytes(UART4, (uint8_t *)"uart init\r\n", 12);
		//		LOG_D("uart test");
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