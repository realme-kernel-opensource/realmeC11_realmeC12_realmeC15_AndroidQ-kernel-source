/************************************************************************************
** File: - android\kernel\drivers\misc\ktd_shineled\ktd2026_driver.c
** VENDOR_EDIT
** Copyright (C), 2008-2017, OPPO Mobile Comm Corp., Ltd
**
** Description:
**      breath-led KTD2026's driver for oppo
**      can support three kinds of leds if HW allows.
** Version: 1.0
** Date created: 14:08,2017/4/20
** Author: mofei@Prd6.BaseDrv.Sensor
**
** --------------------------- Revision History: --------------------------------
**     <version>	     <date>	<author>              			   <desc>
**         v1.0  	2017/4/20         Fei.Mo@EXP.BSP.Sensor  	add for KTD2026 driver
**         v1.1  	2017/7/08         Fei.Mo@EXP.BSP.Sensor  	Modify to support MTK platform
************************************************************************************/
#ifndef _KTD2026_H_
#define _KTD2026_H_

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
#define BREATHLIGHT_CURRENT  0x08 //1.06ma = 8*0.125

#define MAX_BACKLIGHT_BRIGHTNESS 0xff

#define CONFIG_SUPPORT_BUTTON_BACKLIGHT 0
#define CONFIG_SUPPORT_BREATHLIGHT      1
#define CONFIG_MTK_PTLATFORM

#define BUTTON_BACKLIGHT_LED LED1
#define BREATHLIGHT_LED LED2

#define BUTTON_BACKLIGHT_ALON LED1_OUT_ALON
#define BUTTON_BACKLIGHT_PWM LED1_OUT_PWM1

#define BREATHLIGHT_ALON LED2_OUT_ALON
#define BREATHLIGHT_PWM LED2_OUT_PWM1

enum KTD2026_LED{
	LED1 = 0,
	LED2 = 1,
	LED3 = 2,
};

struct ktd2026_chip {
	struct i2c_client	*client;
	struct class		*ktd20xx_cls;
	struct device		*ktd22xx_dev;
	int			major;
	u8			EnableReg;
	unsigned long 		breath_time ;
	int			breath_leds;
	int			Ton_ms;
	int			Ttotal_ms;
	int			value[3];
	bool power_on_off;
	int onMS;
	int totalMS;
	int color_D1, color_D2, color_D3;
	int led_id;
	int freq;
	int led_on;
	unsigned int id;
	int offMS;
	int pcb_verison;
	bool crl_bre_watchdog_running;
	struct hrtimer watchdog;
	struct work_struct timeout_work;
	struct workqueue_struct *timeout_workqueue;
	struct regulator *vcc_1v8;
	struct kobject * leds_func_kobj;
};

#endif	/* _KTD2026_H_ */