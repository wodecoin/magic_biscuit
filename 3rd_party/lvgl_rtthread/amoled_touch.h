#ifndef AMOLED_TOUCH_H
#define AMOLED_TOUCH_H

/*********************
 *      DEFINES
 *********************/
#define CST820_I2C_ADDR 0x15 // I2C 7位地址 (未左移1位)
#define CST820_I2C_SCL_PIN GPIO_Pin_10
#define CST820_I2C_SDA_PIN GPIO_Pin_11
#define CST820_I2C_GPIO GPIOB
#define CST820_I2C I2C2
/*********************************************************************
 * 1. 手势识别相关寄存器 (文档P1-P2)
 *********************************************************************/
// 手势ID寄存器地址 (@0x01)
#define CST820_REG_GESTURE_ID 0x01
// 手势ID对应值 (Bit7~Bit0)
#define CST820_GESTURE_NONE 0x00         // 无手势
#define CST820_GESTURE_UP 0x01           // 上滑
#define CST820_GESTURE_DOWN 0x02         // 下滑
#define CST820_GESTURE_LEFT 0x03         // 左滑
#define CST820_GESTURE_RIGHT 0x04        // 右滑
#define CST820_GESTURE_CLICK 0x05        // 单击
#define CST820_GESTURE_DOUBLE_CLICK 0x0B // 双击
#define CST820_GESTURE_LONG_PRESS 0x0C   // 长按
#define CST820_GESTURE_PALM 0xAA         // 大手掌

// 手指个数寄存器地址 (@0x02)
#define CST820_REG_FINGER_NUM 0x02
// 手指个数对应值 (Bit7~Bit0)
#define CST820_FINGER_NUM_NONE 0x00 // 无手指
#define CST820_FINGER_NUM_ONE 0x01  // 1个手指

/*********************************************************************
 * 2. 触摸坐标相关寄存器 (文档P2)
 *********************************************************************/
// X坐标高4位寄存器 (@0x03)：Bit7~Bit6=触摸状态，Bit3~Bit0=Xpos[11:8]
#define CST820_REG_XPOS_H 0x03
// XposH位掩码
#define CST820_XPOS_H_EVENT_MASK (0x3 << 6) // 触摸状态掩码 (Bit7~Bit6)
#define CST820_XPOS_H_XPOS_MASK 0x0F        // X坐标高4位掩码 (Bit3~Bit0)
// 触摸状态对应值 (Bit7~Bit6)
#define CST820_EVENT_PRESS (0x00 << 6)     // 按下
#define CST820_EVENT_RELEASE (0x01 << 6)   // 抬起
#define CST820_EVENT_HOLD_MOVE (0x02 << 6) // 保持或移动
#define CST820_EVENT_RESERVED (0x03 << 6)  // 保留

// X坐标低8位寄存器 (@0x04)：Xpos[7:0]
#define CST820_REG_XPOS_L 0x04

// Y坐标高4位寄存器 (@0x05)：Bit3~Bit0=Ypos[11:8]
#define CST820_REG_YPOS_H 0x05
#define CST820_YPOS_H_YPOS_MASK 0x0F // Y坐标高4位掩码 (Bit3~Bit0)

// Y坐标低8位寄存器 (@0x06)：Ypos[7:0]
#define CST820_REG_YPOS_L 0x06

// 12位坐标组合宏 (高4位 + 低8位)
#define CST820_GET_XPOS(xh, xl) (((xh) & CST820_XPOS_H_XPOS_MASK) << 8) | (xl)
#define CST820_GET_YPOS(yh, yl) (((yh) & CST820_YPOS_H_YPOS_MASK) << 8) | (yl)

/*********************************************************************
 * 3. 芯片信息相关寄存器 (文档P2)
 *********************************************************************/
// 芯片型号寄存器 (@0xA7)：ChipID (Bit7~Bit0)
#define CST820_REG_CHIP_ID 0xA7

// 工程编号寄存器 (@0xA8)：ProjID (Bit7~Bit0)
#define CST820_REG_PROJ_ID 0xA8

// 软件版本号寄存器 (@0xA9)：FwVersion (Bit7~Bit0)
#define CST820_REG_FW_VERSION 0xA9

// TP厂家ID寄存器 (@0xAA)：FactoryID (Bit7~Bit0)
#define CST820_REG_FACTORY_ID 0xAA

/*********************************************************************
 * 4. 低功耗与复位控制寄存器 (文档P2-P3)
 *********************************************************************/
// 休眠模式控制寄存器 (@0xE5)：SleepMode (Bit7~Bit0)
#define CST820_REG_SLEEP_MODE 0xE5
// 休眠模式配置值
#define CST820_SLEEP_MODE_EXIT 0x00         // 退出休眠 (亮屏后写入)
#define CST820_SLEEP_MODE_GESTURE_WAKE 0x01 // 支持手势唤醒 (灭屏前写入)
#define CST820_SLEEP_MODE_NO_WAKE 0x03      // 无触摸唤醒的休眠

// 错误复位控制寄存器 (@0xEA)：Bit2=长按复位使能，Bit1=大面积触摸复位使能，Bit0=双指复位使能
#define CST820_REG_ERR_RESET_CTL 0xEA
// 错误复位使能位
#define CST820_ERR_RESET_EN_LT (1 << 2)  // 使能长按复位
#define CST820_ERR_RESET_EN_FAT (1 << 1) // 使能大面积触摸复位
#define CST820_ERR_RESET_EN_2F (1 << 0)  // 使能双指复位

/*********************************************************************
 * 5. 手势与中断控制寄存器 (文档P3)
 *********************************************************************/
// 手势掩码寄存器 (@0xEC)：EnDClick (Bit7~Bit0，控制双击功能)
#define CST820_REG_MOTION_MASK 0xEC
// 双击功能配置值
#define CST820_MOTION_DCLICK_DIS 0x00    // 禁止双击功能
#define CST820_MOTION_DCLICK_EN 0x01     // 使能双击功能
#define CST820_MOTION_DCLICK_EN_IMM 0x11 // 使能双击 + 双击抬起后立马上报

// 中断脉冲宽度寄存器 (@0xED)：IrqPluseWidth (Bit7~Bit0，单位1ms，可选1~5)
#define CST820_REG_IRQ_PULSE_WIDTH 0xED
// 中断脉冲宽度配置值
#define CST820_IRQ_PULSE_1MS 0x01 // 默认值：1ms
#define CST820_IRQ_PULSE_2MS 0x02 // 2ms
#define CST820_IRQ_PULSE_3MS 0x03 // 3ms
#define CST820_IRQ_PULSE_4MS 0x04 // 4ms
#define CST820_IRQ_PULSE_5MS 0x05 // 5ms

// 中断控制寄存器 (@0xFA)：Bit7=测试使能，Bit6=触摸中断，Bit5=状态变化中断，Bit4=手势中断
#define CST820_REG_IRQ_CTL 0xFA
// 中断使能位
#define CST820_IRQ_CTL_EN_TEST (1 << 7)   // 使能中断测试 (周期发低脉冲)
#define CST820_IRQ_CTL_EN_TOUCH (1 << 6)  // 触摸时周期发低脉冲
#define CST820_IRQ_CTL_EN_CHANGE (1 << 5) // 触摸状态变化发低脉冲
#define CST820_IRQ_CTL_EN_MOTION (1 << 4) // 检测到手势发低脉冲

/*********************************************************************
 * 6. 自动低功耗控制寄存器 (文档P3)
 *********************************************************************/
// 禁止自动休眠寄存器 (@0xFE)：DisAutoSleep (Bit7~Bit0)
#define CST820_REG_DIS_AUTO_SLEEP 0xFE
// 自动低功耗配置值
#define CST820_AUTO_SLEEP_EN 0x00  // 使能自动进入低功耗
#define CST820_AUTO_SLEEP_DIS 0x01 // 禁止自动进入低功耗 (默认：非0即可)

#endif