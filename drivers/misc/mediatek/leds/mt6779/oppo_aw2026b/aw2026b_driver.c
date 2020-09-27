/************************************************************************************
** File: - kernel-4.4\drivers\misc\mediatek\leds\mt6763\oppo_aw2026\aw2026_driver.c
** VENDOR_EDIT
** Copyright (C), 2008-2016, OPPO Mobile Comm Corp., Ltd
**
** Description:
**      breath-led aw2026's driver for oppo
**      can support three kinds of leds if HW allows.
** Version: 1.0
** Date created: 06/28/2017
** Author:Qiao.Hu@EXP.BSP.CHG.basicr
**
** --------------------------- Revision History: --------------------------------
**     <version>	     <date>	<author>              			   <desc>
**         v1.0  	2017/6/28         Qiao.Hu@EXP.BSP.CHG.basic            Add this driver for breath-LED for project 17311
**         v1.1  	2017/7/08         Fei.Mo@EXP.BSP.Sensor  	          Modify to support MTK platform
************************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <asm/errno.h>
#include <linux/cdev.h>
#include <linux/leds.h>
#include <linux/regulator/consumer.h>
#include "aw2026b_driver.h"

#define AW_I2C_NAME			"aw2026b"

//brightness:xxx   		blink: 1:blink 0:no blink    			grppwm:onMS    		grpfreq: totalMS
//freq = totalMS / 50;
//pwm = (onMS * 255) / totalMS; 0---16;
//case LIGHT_FLASH_TIMED:
//onMS = state->flashOnMS;
//offMS = state->flashOffMS;
#define DEVINFO_NAME AW_I2C_NAME

#define log_fmt(fmt) "[line:%d][module:%s][%s] " fmt

#define KTD_ERR(a,arg...)\
		printk(KERN_NOTICE log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

#define KTD_MSG(a, arg...) \
		printk(KERN_INFO log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

static struct aw2026_chip *g_the_chip = NULL;

static int time_edge_buf[16] = {0,130,260,380,510,770,1040,1600,2100,2600,3100,4200,5200,6200,7300,8300};
static int time_stable_buf[16] = {40,130,260,380,510,770,1040,1600,2100,2600,3100,4200,5200,6200,7300,8300};

void aw20xxb_led_off(void);
static struct regulator *vcc_i2c_1v8;

static int timetoreg(int value,int *buf,int length)
{
    int i = 0;

	for(i = 0; i < length; i++)
	{
		if( value < buf[i] || value < 0 )
			break;
    }
	if(i == length)
		i = i - 1;

	return i;

}
#if CONFIG_SUPPORT_BUTTON_BACKLIGHT
static void set_button_backlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value);
#endif

#if CONFIG_SUPPORT_BREATHLIGHT
static void set_breathlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value);
#endif


static struct led_classdev AW2026_lcds[] = {
	#if CONFIG_SUPPORT_BUTTON_BACKLIGHT
	{
	        .name		= "button-backlight-b",
	        .brightness 	= MAX_BACKLIGHT_BRIGHTNESS,
	        .brightness_set = set_button_backlight_brightness,
	},
	#endif
	#if CONFIG_SUPPORT_BREATHLIGHT
	{
	        .name		= "led2",
	        .brightness 	= MAX_BACKLIGHT_BRIGHTNESS,
	        .brightness_set = set_breathlight_brightness,
	},
	#endif
};

#if CONFIG_SUPPORT_BUTTON_BACKLIGHT
static void set_button_backlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	KTD_MSG("aw20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}

	if (value == 0) {
			aw20xxb_led_off();
			g_the_chip->color_D2 = 0;
	} else {
			aw20xxb_led_on();
	}

}
#endif /* CONFIG_SUPPORT_BUTTON_BACKLIGHT  */

static int aw2026_lv8_enable(bool enable) {
	int ret = -1;
	if (enable) {
		if(regulator_count_voltages(vcc_i2c_1v8) > 0) {
			ret = regulator_set_voltage(vcc_i2c_1v8, 1800000,1800000);
			if (ret) {
				pr_err("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
			}
			else{

				ret = regulator_enable(vcc_i2c_1v8);
				if(ret) {
					pr_err("Regulator vcc_i2c enable failed rc=%d\n", ret);
				}
			}
		}
	} else {
		ret = regulator_disable(vcc_i2c_1v8);
		if(ret) {
			pr_err("Regulator vcc_i2c disable failed rc=%d\n", ret);
		}
	}
	return ret;
}

#if CONFIG_SUPPORT_BREATHLIGHT
static void set_breathlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	#ifndef CONFIG_MTK_PTLATFORM
	KTD_MSG("aw20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}

	if (value > MAX_BACKLIGHT_BRIGHTNESS) {
		value = MAX_BACKLIGHT_BRIGHTNESS;
	}

	if (!strcmp(led_cdev->name, "led2")) {
		g_the_chip->color_D2 = value;
	} else {
		g_the_chip->color_D2 = 0;
	}

	#else /*CONFIG_MTK_PTLATFORM*/

	int totalMS = 100 * 50;//freq = 100;
	int onMS = 80 * totalMS / 255;//pwm = 80;
	int EdgeMS_reg = 0;
	int onMS_reg = 0;
	int offMS_reg = 0;
	int Trise_on = 0;
	int Tfall_off = 0;

	offMS_reg = totalMS - onMS;

	KTD_MSG("aw20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}
	if (!strcmp(led_cdev->name, "led2")) {
		if( value == 0) {//close
			g_the_chip->color_D2 = 0;
			aw20xxb_led_off();

		} else if (value == 1) { //blink
			EdgeMS_reg = timetoreg(onMS/3,time_edge_buf,sizeof(time_edge_buf)/4);//trise and tfall
			onMS_reg = timetoreg(onMS/3,time_stable_buf,sizeof(time_stable_buf)/4);//ton
			offMS_reg =  timetoreg(offMS_reg/3,time_stable_buf,sizeof(time_stable_buf)/4);//toff

			Trise_on =  ((EdgeMS_reg << 4 ) | onMS_reg) & 0xff;
			Tfall_off = ((EdgeMS_reg << 4 ) | offMS_reg) & 0xff;
			g_the_chip->color_D2 = g_the_chip->aw20xx_max_brightness;
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}

			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x00);// set LEDEN led output disalbe
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// set  output max current is 12.75mA

			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x01);// LCFG2 led pattern mode
			i2c_smbus_write_byte_data(g_the_chip->client, 0x08, 0x00);//  independent mode

			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G
			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2

			i2c_smbus_write_byte_data(g_the_chip->client, 0x35, Trise_on); // PAT2_T1 Trise & Ton (7:4 rise  3:0 on) ris:0.512S on 0.512s
			i2c_smbus_write_byte_data(g_the_chip->client, 0x36, Tfall_off); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
			i2c_smbus_write_byte_data(g_the_chip->client, 0x37, 0x00);// PAT2_T3  Tdelay
			i2c_smbus_write_byte_data(g_the_chip->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
			i2c_smbus_write_byte_data(g_the_chip->client, 0x39, 0x00); // Timer

			i2c_smbus_write_byte_data(g_the_chip->client, 0x09, 0x02); //  PAT_RUN
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//  LEDEN led output enalbe
		} else if (value == 11) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = 0xA0;  //2ma
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 12) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = 0x90;  //1.8ma
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 13) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = 0x80;  //1.6ma
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 14) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = 0x70;  //1.4ma
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 15) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = 0x60;  //1.2ma
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 16) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = 0x50;  //1ma
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 17) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = 0x40;  //0.8ma
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else {//always on
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip->color_D2 = g_the_chip->aw20xx_max_brightness;
			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
		}
	} else {
		g_the_chip->color_D2  = 0;
	}
	#endif /*CONFIG_MTK_PTLATFORM*/

}

#endif /* CONFIG_SUPPORT_BREATHLIGHT */



void aw20xxb_lowbattery_breath_leds(void)
{
	/*
	 * flash time period: 2.5s, rise/fall 1s, sleep 0.5s
	 * reg5 = 0xaa, reg1 = 0x12
	 */
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x00);// set LEDEN led output disalbe
	i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->aw20xx_max_brightness);		// set  output max current is 12.75*I/255mA

	i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x01);// LCFG2 led pattern mode
	i2c_smbus_write_byte_data(g_the_chip->client, 0x08, 0x00);//  3 LED work in asynchronous mode with independent control

	i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
	i2c_smbus_write_byte_data(g_the_chip->client, 0x35, 0x46); // PAT2_T1 Trise & Ton (7:4 rise	3:0 on) ris:0.512S on 0.512s
	i2c_smbus_write_byte_data(g_the_chip->client, 0x36, 0x46); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
	i2c_smbus_write_byte_data(g_the_chip->client, 0x37, 0x00);// PAT2_T3	Tdelay
	i2c_smbus_write_byte_data(g_the_chip->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
	i2c_smbus_write_byte_data(g_the_chip->client, 0x39, 0x00); // PATTERN Repeat Times
	i2c_smbus_write_byte_data(g_the_chip->client, 0x09, 0x02); // SET led2 is PAT_RUN

	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//  LEDEN led output enalbe

}
void aw20xxb_events_breath_leds(void)
{
	/*
	 * flash time period: 5s, rise/fall 2s, sleep 1s
	 * reg5 = 0xaa, reg1 = 0x30
	 */
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x00);// set LEDEN led output disalbe
	i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->aw20xx_max_brightness);		// set  output max current is 12.75*I/255mA

	i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x01);// LCFG2 led pattern mode
	i2c_smbus_write_byte_data(g_the_chip->client, 0x08, 0x00);//  3 LED work in asynchronous mode with independent control

	i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
	i2c_smbus_write_byte_data(g_the_chip->client, 0x35, 0x55); // PAT2_T1 Trise & Ton (7:4 rise	3:0 on) ris:0.512S on 0.512s
	i2c_smbus_write_byte_data(g_the_chip->client, 0x36, 0x48); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
	i2c_smbus_write_byte_data(g_the_chip->client, 0x37, 0x00);// PAT2_T3	Tdelay
	i2c_smbus_write_byte_data(g_the_chip->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
	i2c_smbus_write_byte_data(g_the_chip->client, 0x39, 0x00); // PATTERN Repeat Times
	i2c_smbus_write_byte_data(g_the_chip->client, 0x09, 0x02); // SET led2 is PAT_RUN

	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//  LEDEN led output enalbe
}
void aw20xxb_led_on(void)
{
	//turn on led when 0x00 is 0x00
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);	//set active mode,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->aw20xx_max_brightness);		// set  output max current is 12.75*63/255 =3.15mA

	i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
	i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
	i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3
	i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2

	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//  LEDEN led output enalbe

}
void aw20xxb_led_off(void)
{
	//turn off led when 0x01 is  0x00,there is set to same as breath leds
	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x00);	              //set close all led
	i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x00);	//set sleep/shutdown mode
	if(aw2026_lv8_enable(false)) {
		pr_err("set_breathlight_brightness Regulator disable vcc_i2c failed\n");
	}
	pr_err("oppo_led_debug %s  end \n", __func__);
}

void aw20xxb_breath_leds_time(int blink)
{
	int EdgeMS_reg = 0;
	int onMS_reg = 0;
	int offMS_reg = 0;
	int Trise_on = 0;
	int Tfall_off = 0;

	onMS_reg = blink/2;
	offMS_reg = blink -onMS_reg;

	EdgeMS_reg = timetoreg(g_the_chip->onMS/3,time_edge_buf,sizeof(time_edge_buf)/4);			//trise and tfall
	onMS_reg = timetoreg(g_the_chip->onMS/3,time_stable_buf,sizeof(time_stable_buf)/4); 		//ton
	offMS_reg =  timetoreg(offMS_reg/3,time_stable_buf,sizeof(time_stable_buf)/4);		//toff
	Trise_on =	((EdgeMS_reg << 4 ) | onMS_reg) & 0xff;
	Tfall_off = ((EdgeMS_reg << 4 ) | offMS_reg) & 0xff;
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	//aw
	i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->aw20xx_max_brightness);		//  set  output max current is 12.75*63/255 =3.15mA

	i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x01);// LCFG2 led pattern mode
	i2c_smbus_write_byte_data(g_the_chip->client, 0x08, 0x00);//  3 LED work in asynchronous mode with independent control

	i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
	i2c_smbus_write_byte_data(g_the_chip->client, 0x35, Trise_on); // PAT2_T1 Trise & Ton (7:4 rise	3:0 on) ris:0.512S on 0.512s
	i2c_smbus_write_byte_data(g_the_chip->client, 0x36, Tfall_off); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
	i2c_smbus_write_byte_data(g_the_chip->client, 0x37, 0x00);// PAT2_T3	Tdelay
	i2c_smbus_write_byte_data(g_the_chip->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
	i2c_smbus_write_byte_data(g_the_chip->client, 0x39, 0x00); // PATTERN Repeat Times
	i2c_smbus_write_byte_data(g_the_chip->client, 0x09, 0x02); // SET led2 is PAT_RUN

	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//  LEDEN led output enalbe
}


EXPORT_SYMBOL(aw20xxb_lowbattery_breath_leds);
EXPORT_SYMBOL(aw20xxb_events_breath_leds);
EXPORT_SYMBOL(aw20xxb_led_on);
EXPORT_SYMBOL(aw20xxb_led_off);
EXPORT_SYMBOL(aw20xxb_breath_leds_time);

static ssize_t Breathled_switch_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
   return sprintf ( buf, "%d\n", g_the_chip->breath_leds);
}

static ssize_t set_breathlight_colour_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
   return sprintf ( buf, "%d,%d,%d,%d", g_the_chip->color_D1,g_the_chip->color_D2,g_the_chip->color_D3,g_the_chip->aw2026_max_current);
}


static ssize_t set_breathlight_colour_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count)
{
    int data[4] = {0};
    if (NULL == buf)
        return -EINVAL;

    if (0 == count)
        return 0;

	if (sscanf(buf, "%d,%d,%d,%d", &data[0],&data[1],&data[2],&data[3]) == 4) {
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return count;
	}
	g_the_chip->color_D1 = data[0];
	g_the_chip->color_D2 = data[1];
	g_the_chip->color_D3 = data[2];
	g_the_chip->aw2026_max_current = data[3];
	//g_the_chip->color_D2 = g_the_chip->aw20xx_max_brightness;

	i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
	i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
	i2c_smbus_write_byte_data(g_the_chip->client, 0x10, g_the_chip->color_D1);// R
	i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G
	i2c_smbus_write_byte_data(g_the_chip->client, 0x12, g_the_chip->color_D3); // B

	i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
	i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
	i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

	i2c_smbus_write_byte_data(g_the_chip->client, 0x1C, 0xFF); // PWM1
	i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
	i2c_smbus_write_byte_data(g_the_chip->client, 0x1E, 0xFF); // PWM3
	i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x07);//   LEDEN led output enalbe
	} else {
		pr_err("set_breathlight_colour fail\n");
	}
	return count;
}

static ssize_t Breathled_switch_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count )
{
    if (NULL == buf)
        return -EINVAL;

    if (0 == count)
        return 0;

	 if(buf[0] == '0'){
		pr_err("oppo_led_debug %s  is close led \n", __func__);
		g_the_chip->breath_leds = 0;
		g_the_chip->led_on_off = 0;
		aw20xxb_led_off();
	 }
	 else if( buf[0] == '1'){
	 	pr_err("oppo_led_debug %s  is lowbattery led on \n", __func__);
		g_the_chip->breath_leds = 1;
		g_the_chip->led_on_off = 1;
		aw20xxb_lowbattery_breath_leds();
	 }
	 else if( buf[0] == '2'){
	 	pr_err("oppo_led_debug %s  is events led on\n", __func__);
		g_the_chip->breath_leds = 2;
		g_the_chip->led_on_off = 2;
		aw20xxb_events_breath_leds();
	 }
     else{
	 	pr_err("oppo_led_debug %s  is  led on\n", __func__);
		g_the_chip->breath_leds = 255;
		g_the_chip->led_on_off = 3;
		aw20xxb_led_on();
	 }
	return count;

}

static ssize_t Breathled_switch_show2 ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
   return sprintf ( buf, "value :%ld\n", g_the_chip->value);
}
static ssize_t Breathled_switch_store2 ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count )
{
	//unsigned long value = 0;
	int ret;

    if (NULL == buf)
        return -EINVAL;

	ret = kstrtoul(buf, 0, &g_the_chip->value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=%d\n", __func__, ret);
		return ret;
	}
	//sprintf ( buf, "%d\n", sn3191_breath_led);
	aw20xxb_breath_leds_time(g_the_chip->value);
	g_the_chip->breath_leds = 1;

	return count;

}

//on: 1s off:2s
static ssize_t sled_grppwm_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
	g_the_chip->onMS = value * g_the_chip->totalMS / 255;
	if(g_the_chip->onMS > 8000)
	 g_the_chip->onMS = 8000;
    return count;
}

static ssize_t sled_grpfreq_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
	g_the_chip->totalMS = value * 50;
    if(g_the_chip->totalMS > 16000)
        g_the_chip->totalMS = 16000;
    return count;
}

static ssize_t sled_blink_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	#ifndef CONFIG_MTK_PTLATFORM
	unsigned long value = simple_strtoul(buf, NULL, 10);
	int EdgeMS_reg = 0;
	int onMS_reg = 0;
	int offMS_reg = 0;
	int Trise_on = 0;
	int Tfall_off = 0;

	offMS_reg = g_the_chip->totalMS - g_the_chip->onMS;

	if (value == 1 && g_the_chip->totalMS && g_the_chip->onMS) {
		EdgeMS_reg = timetoreg(g_the_chip->onMS/3,time_edge_buf,sizeof(time_edge_buf)/4);			//trise and tfall
		onMS_reg = timetoreg(g_the_chip->onMS/3,time_stable_buf,sizeof(time_stable_buf)/4);			//ton
		offMS_reg =  timetoreg(offMS_reg/3,time_stable_buf,sizeof(time_stable_buf)/4);		//toff

		Trise_on =  ((EdgeMS_reg << 4 ) | onMS_reg) & 0xff;
		Tfall_off = ((EdgeMS_reg << 4 ) | offMS_reg) & 0xff;

		pr_err("oppo_led_debug %s  totMS=%d,onMS=%d,EdgeMS_reg=%x,onMS_reg=%x,color_D1=%d,color_D2=%d,color_D3=%d,Trise_on=%x,Tfall_off=%x \n",
			__func__,g_the_chip->totalMS,g_the_chip->onMS,EdgeMS_reg,onMS_reg, g_the_chip->color_D1, g_the_chip->color_D2, g_the_chip->color_D3, Trise_on, Tfall_off);

        	//we fix the breath time because of different IC has different time value
		//aw
		if(aw2026_lv8_enable(true)) {
			pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
			return -1;
		}
		i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x00);// set LEDEN led output disalbe
		i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
		i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// set  output max current is 12.75mA

		i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x01);// LCFG2 led pattern mode
		i2c_smbus_write_byte_data(g_the_chip->client, 0x08, 0x00);//  independent mode

		i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G
		i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2

		i2c_smbus_write_byte_data(g_the_chip->client, 0x35, Trise_on); // PAT2_T1 Trise & Ton (7:4 rise  3:0 on) ris:0.512S on 0.512s
		i2c_smbus_write_byte_data(g_the_chip->client, 0x36, Tfall_off); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
		i2c_smbus_write_byte_data(g_the_chip->client, 0x37, 0x00);// PAT2_T3  Tdelay
		i2c_smbus_write_byte_data(g_the_chip->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
		i2c_smbus_write_byte_data(g_the_chip->client, 0x39, 0x00); // Timer

		i2c_smbus_write_byte_data(g_the_chip->client, 0x09, 0x02); //  PAT_RUN
		i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//  LEDEN led output enalbe

	}else {
		if((g_the_chip->color_D1+g_the_chip->color_D2+g_the_chip->color_D3) == 0) {
	  		aw20xx_led_off();
		}else {

			i2c_smbus_write_byte_data(g_the_chip->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip->client, 0x03, g_the_chip->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x11, g_the_chip->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip->client, 0x07, 0x02);//   LEDEN led output enalbe
			if(aw2026_lv8_enable(false)) {
				pr_err("set_breathlight_brightness Regulator disable vcc_i2c failed\n");
			}

		}
	}
	return count;
	#else
    	return count;
	#endif
}

static ssize_t sled_showing(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    if (value >= 100) {
        return count;
    } else if ( (value < 100 ) && ( value > 20) ) {
        return count;
    } else {
        return count;
    }
}
static ssize_t sled_charging(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);

    if (value >= 100) {
        return count;
    } else {
        return count;
    }
}

static ssize_t sled_reset(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
    unsigned long value = simple_strtoul(buf, NULL, 10);
    if (value == 1) {
        return count;
    } else {
        return count;
    }
}

static ssize_t sled_test(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
    return count;
}

static DEVICE_ATTR(led, S_IWUSR | S_IRUGO, Breathled_switch_show, Breathled_switch_store);
static DEVICE_ATTR(breath_led, S_IWUSR | S_IRUGO, Breathled_switch_show2, Breathled_switch_store2);
static DEVICE_ATTR(grpfreq, S_IWUSR | S_IRUGO, NULL, sled_grpfreq_store);
static DEVICE_ATTR(grppwm, S_IWUSR | S_IRUGO, NULL, sled_grppwm_store);
static DEVICE_ATTR(blink, S_IWUSR | S_IRUGO, NULL, sled_blink_store);
static DEVICE_ATTR(showing, S_IWUSR | S_IRUGO, NULL, sled_showing);
static DEVICE_ATTR(charging, S_IWUSR | S_IRUGO, NULL, sled_charging);
static DEVICE_ATTR(ledreset, S_IWUSR | S_IRUGO, NULL, sled_reset);
static DEVICE_ATTR(ledtest, S_IWUSR | S_IRUGO, NULL, sled_test);
static DEVICE_ATTR(color, S_IWUSR | S_IRUGO, set_breathlight_colour_show, set_breathlight_colour_store);

static struct attribute *blink_attributes[] = {
    &dev_attr_grppwm.attr,
    &dev_attr_grpfreq.attr,
    &dev_attr_blink.attr,
    &dev_attr_charging.attr,
    &dev_attr_showing.attr,
    &dev_attr_ledreset.attr,
    &dev_attr_ledtest.attr,
    &dev_attr_color.attr,
    &dev_attr_led.attr,
    NULL
};
static const struct attribute_group blink_attr_group = {
    .attrs = blink_attributes,
};



 static ssize_t aw20xx_read(struct file *file, char __user *buf, size_t size, loff_t * offset)
 {
	 return 0;
 }

 static ssize_t aw20xx_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
 {

	 static unsigned char rec_data[2];
	 memset(rec_data, 0, 2);

	 if (copy_from_user(rec_data, buf, 1))
	 {
		 return -EFAULT;
	 }

	 if(buf[0] == '0'){
		g_the_chip->breath_leds = 0;
		g_the_chip->led_on_off = 0;
	   aw20xxb_led_off();
	 }
	 else if( buf[0] == '1'){
		g_the_chip->breath_leds = 1;
		g_the_chip->led_on_off = 1;
		aw20xxb_lowbattery_breath_leds();
	 }
	 else if( buf[0] == '2'){
		g_the_chip->breath_leds = 2;
		g_the_chip->led_on_off = 2;
		aw20xxb_events_breath_leds();
	 }
	 else{
		g_the_chip->breath_leds = 255;
		g_the_chip->led_on_off = 3;
		aw20xxb_led_on();
	 }

	 return 1;
 }


 static struct file_operations aw20xx_fops = {
	 .owner = THIS_MODULE,
	 .read	= aw20xx_read,
	 .write = aw20xx_write,
 };

static int aw20xx_probe(struct i2c_client *client, const struct i2c_device_id *id){

	int ret = -1;
	int i, tmp = 0;
	struct aw2026_chip *chip = NULL;
	pr_err("oppo_led_debug %s is probe start \n", __func__);
	chip = devm_kzalloc(&client->dev,
		sizeof(struct aw2026_chip), GFP_KERNEL);

	if (!chip) {
		KTD_ERR(" devm_kzalloc() failed\n");
		return -ENOMEM;
	}

	chip->client = client;
	chip->led_on_off = 255;
	chip->breath_leds = 0;
	chip->aw20xx_max_brightness = 0xFF;
	chip->aw2026_max_current = 0x02;
	chip->onMS = 0;
	chip->totalMS = 0;
	chip->value = 0;
	chip->color_D1 = 0;
	chip->color_D2 = 0;
	chip->color_D3 = 0;
	//client = client;
    ret = of_property_read_u32(client->dev.of_node, "aw2026,max_brightness", &tmp);
	if (!ret)
       {
	    	chip->aw20xx_max_brightness = tmp;   //<0x3f>;  //current:  3.15mA
       }
	else
	{
		chip->aw20xx_max_brightness = 0x3f;   //<0x3f>;
	}
	ret = of_property_read_u32(client->dev.of_node, "aw2026,max_current", &tmp);
	if (!ret)
       {
	    	chip->aw2026_max_current = tmp;	//<0x02>; //current:  12.75mA
       }
	else
	{
		chip->aw2026_max_current = 0x02;   //<0x02>;
	}
	vcc_i2c_1v8 = regulator_get(&client->dev, "vcamio");
	if( IS_ERR(vcc_i2c_1v8) ){
		ret = PTR_ERR(vcc_i2c_1v8);
		pr_err("Regulator get ss failed vcc_i2c rc=%d\n", ret);
	}

	if (!IS_ERR(vcc_i2c_1v8)){
		if(regulator_count_voltages(vcc_i2c_1v8) > 0) {
			ret = regulator_set_voltage(vcc_i2c_1v8, 1800000,1800000);
			if (ret) {
				pr_err("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
			}
			else{
				ret = regulator_enable(vcc_i2c_1v8);
				if(ret) {
					pr_err("Regulator vcc_i2c enable failed rc=%d\n", ret);
				}
			}
		}
	}
	/* create i2c struct, when receve and transmit some byte */
	pr_err("oppo_led_debug %s is probe start1 \n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev,
				"%s: check_functionality failed.", __func__);
		ret = -ENODEV;
		goto err_i2c_check;
	}
	pr_err("oppo_led_debug %s is probe start2 \n", __func__);

	ret = i2c_smbus_write_byte_data(chip->client, 0x00, 0x55);//reset reg
	if(ret < 0){
		printk("reset aw2026 led control ic fail,ret =%d\n",ret);
		goto err_i2c_check;
	}

	ret = i2c_smbus_write_byte_data(chip->client, 0x03, chip->aw2026_max_current);		// set	output max current is 12.75mA
	ret = i2c_smbus_write_byte_data(chip->client, 0x11, chip->aw20xx_max_brightness);		// set	output max current is 2.048mA
	ret = i2c_smbus_write_byte_data(chip->client, 0x07, 0x00);				  //set close all led
	ret = i2c_smbus_write_byte_data(chip->client, 0x01, 0x00);	//set sleep/shutdown mode
	if(ret < 0){
		printk("can't find aw2026 led control ic!ret =%d",ret);
		goto err_i2c_check;
	}

	ret = regulator_disable(vcc_i2c_1v8);
	if(ret) {
		pr_err("Regulator vcc_i2c disable failed rc=%d\n", ret);
	}

	/*************Added by Tong.han for same interface with 14037*******************/
	pr_err("oppo_led_debug %s is probe start3 \n", __func__);
	for(i = 0; i < 1; i ++ ) {
		if (led_classdev_register(&client->dev, &AW2026_lcds[i])) {
            		printk(KERN_ERR "led_classdev_register failed of AW2026_lcds!\n");
		}
	}
	ret = sysfs_create_group(&client->dev.kobj, &blink_attr_group);
	if (ret) {
		pr_err( "%s : sysfs_create_group failed!\n", __func__);
		goto err_group_register;
	}
	pr_err("oppo_led_debug %s is probe start4 \n", __func__);
	/* create device node sys/class/ktd20xx/ktd2206/led
	 * led: 0:breath led 1:open led  2:close led
	 */
	chip->major = register_chrdev(0, "ktd20xx", &aw20xx_fops);
	chip->aw20xx_cls = class_create(THIS_MODULE, "aw20xxb");
	if (IS_ERR(chip->aw20xx_cls)) {
		ret = IS_ERR(chip->aw20xx_cls);
		pr_err("perfmap: Error in class_create aw20xx_cls\n");
		goto err_class_create;
	pr_err("oppo_led_debug %s is probe start5 \n", __func__);
	chip->aw20xx_dev = device_create(chip->aw20xx_cls, NULL, MKDEV(chip->major, 0), NULL, "aw2026"); /* /dev/ktd20xx */

	if (device_create_file(chip->aw20xx_dev, &dev_attr_led) < 0)
	    pr_err("Failed to create device file(%s)!\n", dev_attr_led.attr.name);
	if (device_create_file(chip->aw20xx_dev, &dev_attr_breath_led) < 0)
	    pr_err("Failed to create device file(%s)!\n", dev_attr_led.attr.name);
	}
	pr_err("oppo_led_debug %s is probe start6 \n", __func__);
	g_the_chip = chip;
	return 0;

err_class_create:
	class_destroy(chip->aw20xx_cls);
err_group_register:
    for(i = 0; i < 1; i ++ )
        led_classdev_unregister(&AW2026_lcds[i]);
err_i2c_check:
	devm_kfree(&client->dev,chip);
	return ret;
}
static void aw20xx_shutdown(struct i2c_client *client)
{
	printk("%s\n",__func__);
	aw20xxb_led_off();
}

static int aw20xx_remove(struct i2c_client *client){

	printk("%s\n",__func__);
	return 0;
}


static const struct i2c_device_id aw20xx_id[] = {
	{AW_I2C_NAME, 0},
	{ }
};


static struct of_device_id aw20xx_match_table[] = {
        { .compatible = "aw,aw2026b",},
        { },
};

static struct i2c_driver aw20xx_driver = {
	.driver = {
		.name	= AW_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = aw20xx_match_table,
	},
	.probe = aw20xx_probe,
	.remove = aw20xx_remove,
	.shutdown = aw20xx_shutdown,
	.id_table = aw20xx_id,
};

static int __init aw20xxb_init(void)
{
	int err;

	printk("%s\n",__func__);

	err = i2c_add_driver(&aw20xx_driver);
	if (err) {
		printk(KERN_WARNING "aw20xx driver failed "
		       "(errno = %d)\n", err);
	} else {
		printk( " added driver is end %s\n",
		          aw20xx_driver.driver.name);
	}
	return err;
}

static void __exit aw20xxb_exit(void)
{
	printk("%s\n",__func__);
	i2c_del_driver(&aw20xx_driver);
}

module_init(aw20xxb_init);
module_exit(aw20xxb_exit);

MODULE_LICENSE("GPL");

