/************************************************************************************
** File: - kernel-4.4\drivers\misc\mediatek\leds\mt6763\oppo_aw2026\aw2026_driver.h
** VENDOR_EDIT
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd
**
** Description:
**      breath-led aw2026's driver for oppo
**      can support three kinds of leds if HW allows.
** Version: 1.0
** Date created: 06/28/2017
** Author:Qiao.Hu@EXP.BSP.CHG.basic
**
** --------------------------- Revision History: --------------------------------
**     <version>	     <date>	<author>              			   <desc>
**         v1.0  	2017/6/28         Qiao.Hu@EXP.BSP.CHG.basic            Add this driver for breath-LED for project 17311
**         v1.1  	2017/7/08         Fei.Mo@EXP.BSP.Sensor  	          Modify to support MTK platform
************************************************************************************/
#ifndef _AW2026_H_
#define _AW2026_H_

#define LED1_OUT_ALON  	0x1 << 0   //always on
#define LED1_OUT_PWM1 	0x2 << 0
#define LED1_OUT_PWM2 	0x3 << 0
#define LED2_OUT_ALON  	0x1 << 2
#define LED2_OUT_PWM1 	0x2 << 2
#define LED2_OUT_PWM2 	0x3 << 2
#define LED3_OUT_ALON  	0x1 << 4
#define LED3_OUT_PWM1 	0x2 << 4
#define LED3_OUT_PWM2 	0x3 << 4

#define OUTPUT_10AM 	0x4f
#define OUTPUT_24AM 	0xbf

#define MAX_BACKLIGHT_BRIGHTNESS 0xff

#define CONFIG_SUPPORT_BUTTON_BACKLIGHT 0
#define CONFIG_SUPPORT_BREATHLIGHT      1
#define CONFIG_MTK_PTLATFORM

#define BUTTON_BACKLIGHT_LED LED1
#define BREATHLIGHT_LED LED2

#define BUTTON_BACKLIGHT_ALON LED1_OUT_ALON
#define BUTTON_BACKLIGHT_PWM1 LED1_OUT_PWM1
#define BUTTON_BACKLIGHT_PWM2 LED1_OUT_PWM2

#define BREATHLIGHT_ALON LED2_OUT_ALON
#define BREATHLIGHT_PWM1 LED2_OUT_PWM1
#define BREATHLIGHT_PWM2 LED2_OUT_PWM2

enum KTD2026_LED{
	LED1 = 0,
	LED2 = 1,
	LED3 = 2,
};

struct aw2026_chip {
	struct i2c_client *client;

	struct class *aw20xx_cls;
	struct device *aw20xx_dev;
	int major;
	int breath_leds;
	int led_on_off;
	bool power_on_off;
	unsigned long value;
	u8 EnableReg;
	int aw20xx_max_brightness;
	int aw2026_max_current;

	int onMS;
	int totalMS;
	int color_D1, color_D2, color_D3;
	int col_D1, col_D2, col_D3;
	int led_id;
	int freq;
	int led_on;
	unsigned int id;
	int offMS;
	int pcb_verison;
	bool crl_bre_watchdog_running;
    struct regulator *vcc_1v8;                      /*power 1v8*/
	struct hrtimer watchdog;
	struct work_struct timeout_work;
	struct workqueue_struct *timeout_workqueue;

	struct kobject * leds_func_kobj;
};

#endif	/* _AW2026_H_ */
