## 项目目录

1 关于lvgl移植

2 CO5300驱动

	大体步骤就是：
	1)初始化QSPI 外设
	2)写AMOLED 初始化序列
	3)实现lcd_write_cmd/ lcd_write_data/lcd_set_window
	4)在disp_flush里调用这些函数把像素数据刷到屏幕
3 

