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
#include "aw2026_driver.h"
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <soc/oppo/oppo_project.h>

#define AW_I2C_NAME			"aw2026"
__attribute__((weak)) void global_i2c_mutex_lock(void)
{
    return;
}
__attribute__((weak)) void global_i2c_mutex_unlock(void)
{
    return;
}
extern void global_i2c_mutex_lock(void);
extern void global_i2c_mutex_unlock(void);

//brightness:xxx   		blink: 1:blink 0:no blink    			grppwm:onMS    		grpfreq: totalMS
//freq = totalMS / 50;
//pwm = (onMS * 255) / totalMS; 0---16;
//case LIGHT_FLASH_TIMED:
//onMS = state->flashOnMS;
//offMS = state->flashOffMS;
#define DEVINFO_NAME AW_I2C_NAME

#define log_fmt(fmt) "[line:%d][module:%s][%s] " fmt

#define AW_ERR(a,arg...)\
		printk(KERN_NOTICE log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

#define AW_MSG(a, arg...) \
		printk(KERN_INFO log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

#define DLED_NUM 2

static struct aw2026_chip *g_the_chip[DLED_NUM] = {NULL,NULL};
static struct aw2026_chip *g_chip = NULL;


static int time_edge_buf[16] = {0,130,260,380,510,770,1040,1600,2100,2600,3100,4200,5200,6200,7300,8300};
static int time_stable_buf[16] = {40,130,260,380,510,770,1040,1600,2100,2600,3100,4200,5200,6200,7300,8300};

void aw20xx_led_off(void);

//static struct regulator *vcc_i2c_1v8;

static int load_num = 0;

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
			.name		= "button-backlight",
			.brightness 	= MAX_BACKLIGHT_BRIGHTNESS,
			.brightness_set = set_button_backlight_brightness,
	},
	#endif
	#if CONFIG_SUPPORT_BREATHLIGHT
	{
			.name		= "led1",
			.brightness 	= MAX_BACKLIGHT_BRIGHTNESS,
			.brightness_set = set_breathlight_brightness,
	},
	#endif
};

#if CONFIG_SUPPORT_BUTTON_BACKLIGHT
static void set_button_backlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	AW_MSG("aw20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip[0]) {
		AW_ERR("the chip null");
		return;
	}

	if (value == 0) {
			aw20xx_led_off();
			g_the_chip[0]->color_D2 = 0;
	} else {
			aw20xx_led_on();
	}

}
#endif /* CONFIG_SUPPORT_BUTTON_BACKLIGHT  */
#define MAX_I2C_RETRY_TIME 2

int aw2026_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int retval = 0;
	int retry = 0;
	int i = 0;
	struct i2c_msg msgs[writelen];

	if (client == NULL) {
		AW_ERR("i2c_client == NULL!");
		return -1;
	}

	if (writelen > 0) {
		for (i = 0; i < writelen; i++) {
			msgs[i].addr = client->addr;
			msgs[i].flags = I2C_M_STOP;
			msgs[i].len = 2;
			msgs[i].buf = &writebuf[2*i];
		}
		msgs[writelen-1].flags = 0;
	};
	for (retry = 0; retry < MAX_I2C_RETRY_TIME; retry++) {
		if (i2c_transfer(client->adapter, msgs, writelen) == writelen) {
			retval = 1;
			break;
		}
		//msleep(20);
		msleep(2);
	}
	if (retry == MAX_I2C_RETRY_TIME) {
		AW_ERR("i2c_transfer(write) over retry limit\n");
		retval = -EIO;
	}

	return retval;
}


static int32_t aw2026_i2c_write_byte(struct aw2026_chip *chip, uint8_t reg, uint8_t val)
{
	int ret = 0;
	int flag_err = 0;
	struct i2c_client *client = chip->client;

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if(ret < 0){
		//printk("reset aw2026 led control ic fail,ret =%d\n",ret);
		do {
				msleep(1);
				ret = i2c_smbus_write_byte_data(client, reg, val);
				flag_err++;
			} while((ret < 0x00) && (flag_err < 1));
	}

	if(ret < 0)
		AW_ERR(" aw2026 i2c control ic fail, reg =%d, ret =%d, flag_err=%d\n", reg, ret, flag_err);
	return ret;
}

static int aw2026_lv8_enable_wq(bool enable) {
	int ret = -1;
	if (g_chip == NULL) {
		AW_ERR("g_chip is NULL\n");
		return ret;
	}

	if (g_chip->pcb_verison > HW_VERSION__12) {
		AW_ERR("the pcb verison is VPT pcb_verison = %d.\n", g_chip->pcb_verison);
		return 0;
	}

	if (enable) {
			if(regulator_count_voltages(g_chip->vcc_1v8) > 0) {
				ret = regulator_set_voltage(g_chip->vcc_1v8, 1800000,1800000);
				if (ret) {
					AW_ERR("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
				} else {
					ret = regulator_enable(g_chip->vcc_1v8);
					if(ret) {
						AW_ERR("Regulator vcc_i2c enable failed rc=%d\n", ret);
					}
				}
		} else {
			return 0;
		}
	} else {
			ret = regulator_disable(g_chip->vcc_1v8);
			if(ret) {
				AW_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
			}
	}
	return ret;
}


static int aw2026_lv8_enable(bool enable) {
	int ret = -1;
	if (g_chip == NULL) {
		AW_ERR("g_chip is NULL\n");
		return ret;
	}

	if (g_chip->pcb_verison > HW_VERSION__12) {
		AW_ERR("the pcb verison is VPT pcb_verison = %d.\n", g_chip->pcb_verison);
		return 0;
	}

	if (enable) {
		if(!g_chip->power_on_off) {
			if(regulator_count_voltages(g_chip->vcc_1v8) > 0) {
				ret = regulator_set_voltage(g_chip->vcc_1v8, 1800000,1800000);
				if (ret) {
					AW_ERR("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
				} else {
					ret = regulator_enable(g_chip->vcc_1v8);
					g_chip->power_on_off = 1;
					if(ret) {
						AW_ERR("Regulator vcc_i2c enable failed rc=%d\n", ret);
					}
				}
			}
		} else {
			return 0;
		}
	} else {
		if(g_chip->power_on_off) {
			ret = regulator_disable(g_chip->vcc_1v8);
			g_chip->power_on_off = 0;
			if(ret) {
				AW_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
			}
		} else {
			return 0;
		}
	}
	return ret;
}

#if CONFIG_SUPPORT_BREATHLIGHT
static void set_breathlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	#ifndef CONFIG_MTK_PTLATFORM
	AW_MSG("aw20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip[0]) {
		AW_ERR("the chip null");
		return;
	}

	if (value > MAX_BACKLIGHT_BRIGHTNESS) {
		value = MAX_BACKLIGHT_BRIGHTNESS;
	}

	if (!strcmp(led_cdev->name, "led1")) {
		g_the_chip[0]->color_D2 = value;
	} else {
		g_the_chip[0]->color_D2 = 0;
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

	AW_MSG("aw20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip[0]) {
		AW_ERR("the chip null");
		return;
	}
	if (!strcmp(led_cdev->name, "led1")) {
		if( value == 0) {//close
			g_the_chip[0]->color_D2 = 0;
			aw20xx_led_off();

		} else if (value == 1) { //blink
			EdgeMS_reg = timetoreg(onMS/3,time_edge_buf,sizeof(time_edge_buf)/4);//trise and tfall
			onMS_reg = timetoreg(onMS/3,time_stable_buf,sizeof(time_stable_buf)/4);//ton
			offMS_reg =  timetoreg(offMS_reg/3,time_stable_buf,sizeof(time_stable_buf)/4);//toff

			Trise_on =  ((EdgeMS_reg << 4 ) | onMS_reg) & 0xff;
			Tfall_off = ((EdgeMS_reg << 4 ) | offMS_reg) & 0xff;
			g_the_chip[0]->color_D2 = g_the_chip[0]->aw20xx_max_brightness;
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x00);// set LEDEN led output disalbe
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// set  output max current is 12.75mA

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x01);// LCFG2 led pattern mode
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x08, 0x00);//  independent mode

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x35, Trise_on); // PAT2_T1 Trise & Ton (7:4 rise  3:0 on) ris:0.512S on 0.512s
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x36, Tfall_off); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x37, 0x00);// PAT2_T3  Tdelay
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x39, 0x00); // Timer

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x02); //  PAT_RUN
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//  LEDEN led output enalbe
		} else if (value == 11) { //blink
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = 0xA0;  //2ma
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 12) { //blink
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = 0x90;  //1.8ma
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 13) { //blink
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = 0x80;  //1.6ma
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 14) { //blink
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = 0x70;  //1.4ma
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 15) { //blink
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = 0x60;  //1.2ma
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 16) { //blink
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = 0x50;  //1ma
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else if (value == 17) { //blink
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = 0x40;  //0.8ma
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		} else {//always on
			if(aw2026_lv8_enable(true)) {
				pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return;
			}
			g_the_chip[0]->color_D2 = g_the_chip[0]->aw20xx_max_brightness;
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
		}
	} else {
		g_the_chip[0]->color_D2  = 0;
	}
	#endif /*CONFIG_MTK_PTLATFORM*/

}

#endif /* CONFIG_SUPPORT_BREATHLIGHT */

  //100ms
#define RESET_TIMEOUT_TIME 6000 * 1000 * 1000
#define ONE_SECOND 380

static void sled_stop_led_work(void)
{
	if(aw2026_lv8_enable_wq(true)) {
		AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	usleep_range(g_chip->onMS * 1000, g_chip->onMS * 1000);
	AW_ERR("sled_stop_led_work start\n");

		if (g_the_chip[0] != NULL) {
			aw2026_i2c_write_byte(g_the_chip[0], 0x09, 0x70);
			aw2026_i2c_write_byte(g_the_chip[0], 0x00, 0x55);
		}
		if (g_the_chip[1] != NULL) {
			aw2026_i2c_write_byte(g_the_chip[1], 0x09, 0x70);//  PAT_STOP
			aw2026_i2c_write_byte(g_the_chip[1], 0x00, 0x55);//  PAT_STOP
		}
	if(aw2026_lv8_enable_wq(false)) {
		AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	AW_ERR("sled_stop_led_work end\n");
	return;
}


static void sled_bre_timeout_work(struct work_struct *work)
{
	if(aw2026_lv8_enable_wq(true)) {
		AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	usleep_range(g_chip->onMS * 1000, g_chip->onMS * 1000);
	AW_ERR("sled_bre_timeout_work start \n");

	if (g_the_chip[0] != NULL) {
		//i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x70);
		aw2026_i2c_write_byte(g_the_chip[0], 0x09, 0x70);
	}
	if (g_the_chip[1] != NULL) {
		//i2c_smbus_write_byte_data(g_the_chip[1]->client, 0x09, 0x70);//  PAT_STOP
		aw2026_i2c_write_byte(g_the_chip[1], 0x09, 0x70);
	}
	usleep_range(g_chip->offMS * 1000, g_chip->offMS * 1000);
	if (g_the_chip[0] != NULL) {
		//i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x07);
		//i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x07);
		aw2026_i2c_write_byte(g_the_chip[0], 0x09, 0x07);
		aw2026_i2c_write_byte(g_the_chip[0], 0x07, 0x07);
	}
	if (g_the_chip[1] != NULL) {
		//i2c_smbus_write_byte_data(g_the_chip[1]->client, 0x09, 0x07);//  PAT_RUN
		//i2c_smbus_write_byte_data(g_the_chip[1]->client, 0x07, 0x07);
		aw2026_i2c_write_byte(g_the_chip[1], 0x09, 0x07);
	}
	if(aw2026_lv8_enable_wq(false)) {
		AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	AW_ERR("sled_bre_timeout_work end\n");
	return;
}

static void sled_start_crl_bre_timer(void)
{
	 if (g_chip->crl_bre_watchdog_running == 0) {
		 AW_ERR("hrtimer_start!!\n");

		 hrtimer_start(&g_chip->watchdog,
				 ktime_set(0, 0),
				 HRTIMER_MODE_REL);
		 g_chip->crl_bre_watchdog_running = 1;
	 }
}

static void sled_stop_crl_bre_timer(void)
{
	 if (g_chip->crl_bre_watchdog_running == 1) {
		 AW_ERR("%s hrtimer_cancel!!\n", __func__);
		 hrtimer_cancel(&g_chip->watchdog);
		 g_chip->crl_bre_watchdog_running = 0;
	 }
}

static enum hrtimer_restart sled_bre_timeout(struct hrtimer *timer)
{

	struct aw2026_chip *chip = g_chip;

	//queue_work(&(chip->timeout_work));
	queue_work_on(smp_processor_id(), chip->timeout_workqueue, &chip->timeout_work);
	hrtimer_forward_now(&chip->watchdog, ktime_set(0, g_chip->totalMS * 1000 * 1000));

	return HRTIMER_RESTART;  //restart the timer
}

void aw20xx_lowbattery_breath_leds(void)
{
	/*
	 * flash time period: 2.5s, rise/fall 1s, sleep 0.5s
	 * reg5 = 0xaa, reg1 = 0x12
	 */
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x00);// set LEDEN led output disalbe
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->aw20xx_max_brightness);		// set  output max current is 12.75*I/255mA

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x01);// LCFG2 led pattern mode
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x08, 0x00);//  3 LED work in asynchronous mode with independent control

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x35, 0x46); // PAT2_T1 Trise & Ton (7:4 rise	3:0 on) ris:0.512S on 0.512s
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x36, 0x46); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x37, 0x00);// PAT2_T3	Tdelay
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x39, 0x00); // PATTERN Repeat Times
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x02); // SET led2 is PAT_RUN

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//  LEDEN led output enalbe

}
void aw20xx_events_breath_leds(void)
{
	/*
	 * flash time period: 5s, rise/fall 2s, sleep 1s
	 * reg5 = 0xaa, reg1 = 0x30
	 */
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x00);// set LEDEN led output disalbe
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->aw20xx_max_brightness);		// set  output max current is 12.75*I/255mA

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x01);// LCFG2 led pattern mode
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x08, 0x00);//  3 LED work in asynchronous mode with independent control

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x35, 0x55); // PAT2_T1 Trise & Ton (7:4 rise	3:0 on) ris:0.512S on 0.512s
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x36, 0x48); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x37, 0x00);// PAT2_T3	Tdelay
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x39, 0x00); // PATTERN Repeat Times
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x02); // SET led2 is PAT_RUN

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//  LEDEN led output enalbe
}
void aw20xx_led_on(void)
{
	//turn on led when 0x00 is 0x00
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);	//set active mode,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->aw20xx_max_brightness);		// set  output max current is 12.75*63/255 =3.15mA

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//  LEDEN led output enalbe

}

void aw20xx_led_breath(void)
{
	//turn on led when 0x00 is 0x00
	u8 breBuf[58] = {0x01, 0x01, 0x03, 3, 0x04, 0x01, 0x05, 0x01, 0x06, 0x01, 0x08, 0x00, 0x10, 0, 0x11,
						0, 0x12, 0, 0x1C, 0xFF, 0x1D, 0xFF, 0x1E, 0xFF, 0x30, 0x41, 0x35,
						0x41, 0x3A, 0x41, 0x31, 0x41, 0x36, 0x41, 0x3B, 0x41, 0x32, 0x00, 0x37, 0x00, 0x3C, 0x00, 0x33,
						0x00, 0x38, 0x00, 0x3D, 0x00, 0x34, 0x00, 0x39, 0x00, 0x3E, 0x00, 0x09, 0x07, 0x07, 0x07,};
	u8 offBuf[12] = {0x10, 0, 0x11, 0, 0x12, 0, 0x07, 0x00, 0x01, 0x00, 0x00, 0x55};
	AW_ERR("aw20xx_led_breath start\n");
	if(aw2026_lv8_enable(true)) {
		AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	if (g_the_chip[0] != NULL) {
		breBuf[3] = g_the_chip[0]->aw2026_max_current;
		breBuf[13] = g_the_chip[0]->col_D1;
		breBuf[15] = g_the_chip[0]->col_D2;
		breBuf[17] = g_the_chip[0]->col_D3;
	} else {
		breBuf[3] = g_the_chip[1]->aw2026_max_current;
		breBuf[13] = g_the_chip[1]->col_D1;
		breBuf[15] = g_the_chip[1]->col_D2;
		breBuf[17] = g_the_chip[1]->col_D3;
	}
	//AW_ERR("breBuf[13] = %d, breBuf[15] = %d,breBuf[17] = %d\n", breBuf[13], breBuf[15], breBuf[17]);
	if (g_the_chip[0] != NULL) {
		aw2026_i2c_write_byte(g_the_chip[0], 0x00, 0x55);// all reset
		msleep(2);
		aw2026_i2c_write(g_the_chip[0]->client, breBuf, 29);
	}

	if (g_the_chip[1] != NULL) {
		aw2026_i2c_write_byte(g_the_chip[1], 0x00, 0x55);// all reset
		msleep(2);
		aw2026_i2c_write(g_the_chip[1]->client, breBuf, 29);
	}
	//usleep_range(g_chip->onMS * 1000, g_chip->onMS * 1000);
	usleep_range(1150 * 1000, 1150 * 1000);

	if (g_the_chip[0] != NULL) {
		aw2026_i2c_write(g_the_chip[0]->client, offBuf, 6);
	}

	if (g_the_chip[1] != NULL) {
		aw2026_i2c_write(g_the_chip[1]->client, offBuf, 6);
	}
	if(aw2026_lv8_enable(false)) {
		AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	AW_ERR("aw20xx_led_breath end\n");
	return;

}

void aw20xx_led_off(void)
{
	//turn off led when 0x01 is  0x00,there is set to same as breath leds
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x00);				  //set close all led
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x00);	//set sleep/shutdown mode
	if(aw2026_lv8_enable(false)) {
		pr_err("set_breathlight_brightness Regulator disable vcc_i2c failed\n");
	}
	pr_err("oppo_led_debug %s  end \n", __func__);
}

void aw20xx_breath_leds_time(int blink)
{
	int EdgeMS_reg = 0;
	int onMS_reg = 0;
	int offMS_reg = 0;
	int Trise_on = 0;
	int Tfall_off = 0;

	onMS_reg = blink/2;
	offMS_reg = blink -onMS_reg;

	EdgeMS_reg = timetoreg(g_the_chip[0]->onMS/3,time_edge_buf,sizeof(time_edge_buf)/4);			//trise and tfall
	onMS_reg = timetoreg(g_the_chip[0]->onMS/3,time_stable_buf,sizeof(time_stable_buf)/4); 		//ton
	offMS_reg =  timetoreg(offMS_reg/3,time_stable_buf,sizeof(time_stable_buf)/4);		//toff
	Trise_on =	((EdgeMS_reg << 4 ) | onMS_reg) & 0xff;
	Tfall_off = ((EdgeMS_reg << 4 ) | offMS_reg) & 0xff;
	if(aw2026_lv8_enable(true)) {
		pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	//aw
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);		// set  output max current is 12.75mA
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->aw20xx_max_brightness);		//  set  output max current is 12.75*63/255 =3.15mA

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x01);// LCFG2 led pattern mode
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x08, 0x00);//  3 LED work in asynchronous mode with independent control

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x35, Trise_on); // PAT2_T1 Trise & Ton (7:4 rise	3:0 on) ris:0.512S on 0.512s
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x36, Tfall_off); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x37, 0x00);// PAT2_T3	Tdelay
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x39, 0x00); // PATTERN Repeat Times
	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x02); // SET led2 is PAT_RUN

	i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//  LEDEN led output enalbe
}


EXPORT_SYMBOL(aw20xx_lowbattery_breath_leds);
EXPORT_SYMBOL(aw20xx_events_breath_leds);
EXPORT_SYMBOL(aw20xx_led_on);
EXPORT_SYMBOL(aw20xx_led_off);
EXPORT_SYMBOL(aw20xx_breath_leds_time);

static ssize_t Breathled_switch_show ( struct device *dev,
									  struct device_attribute *attr, char *buf)
{
   return sprintf ( buf, "%d\n", g_the_chip[0]->breath_leds);
}

static ssize_t set_max_current_show ( struct device *dev,
									  struct device_attribute *attr, char *buf)
{
   return sprintf ( buf, "%d\n", g_chip->aw2026_max_current);
}

static ssize_t set_max_current_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count)
{
    if (NULL == buf)
        return -EINVAL;

    if (0 == count)
        return 0;

    if (sscanf(buf, "%d", &g_chip->aw2026_max_current) == 1) {
        AW_ERR("g_chip->aw2026_max_current = %d.\n", g_chip->aw2026_max_current);
        //g_chip->color_D2 = g_chip->aw20xx_max_brightness;
    } else {
        AW_ERR("g_chip->aw2026_max_current = %d faled.\n", g_chip->aw2026_max_current);
    }
    return count;
}


static ssize_t set_shutdown_colour_show ( struct device *dev,
										  struct device_attribute *attr, char *buf)
{
	if(g_the_chip[0] != NULL) {
		return sprintf ( buf, "%d,%d,%d", g_the_chip[0]->col_D1,g_the_chip[0]->col_D2,g_the_chip[0]->col_D3);
	} else {
		return sprintf ( buf, "%d,%d,%d", g_the_chip[1]->col_D1,g_the_chip[1]->col_D2,g_the_chip[1]->col_D3);
	}
}

static ssize_t set_shutdown_colour_store ( struct device *dev,
									   struct device_attribute *attr, const char *buf, size_t count)
{
	int data[3] = {0};

	if (NULL == buf)
		return -EINVAL;

	if (0 == count)
		return 0;

	if (sscanf(buf, "%d,%d,%d", &data[0],&data[1],&data[2]) == 3) {
		AW_ERR("data[0] = %d, data[2] = %d, data[3] = %d.\n", data[0],data[1],data[2]);
		if (g_the_chip[0] != NULL) {
			g_the_chip[0]->col_D1 = data[0];
			g_the_chip[0]->col_D2 = data[1];
			g_the_chip[0]->col_D3 = data[2];
		} else {
			g_the_chip[1]->col_D1 = data[0];
			g_the_chip[1]->col_D2 = data[1];
			g_the_chip[1]->col_D3 = data[2];
		}
	} else {
        AW_ERR("g_chip->aw2026_max_current = %d faled.\n", g_chip->aw2026_max_current);
    }
    return count;
}


static ssize_t set_breathlight_colour_show ( struct device *dev,
									  struct device_attribute *attr, char *buf)
{
	if(g_the_chip[0]->led_id == 1 && g_the_chip[1] != NULL) {
		return sprintf ( buf, "%d,%d,%d,%d,%d,%d", g_the_chip[1]->color_D1,g_the_chip[1]->color_D2,g_the_chip[1]->color_D3,g_the_chip[1]->freq,g_the_chip[1]->led_id,g_the_chip[1]->led_on);
	} else {
		return sprintf ( buf, "%d,%d,%d,%d,%d,%d", g_the_chip[0]->color_D1,g_the_chip[0]->color_D2,g_the_chip[0]->color_D3,g_the_chip[0]->freq,g_the_chip[0]->led_id,g_the_chip[0]->led_on);
	}
}


static ssize_t set_breathlight_colour_store ( struct device *dev,
									   struct device_attribute *attr, const char *buf, size_t count)
{
	int data[6] = {0};
	int totalMS = 0;
	int riseMS = 0;
	int onMS = 0;
	int fallMS = 0;
	int offMS = 0;
	int fallMS_reg = 0;
	int riseMS_reg = 0;
	int onMS_reg = 0;
	int offMS_reg = 0;
	int Trise_on = 0;
	int Tfall_off = 0;
	unsigned int id = 0;
	int led_num = 0;
	int i;
	u8 onBuf[24] = {0x01, 0x01, 0x03, 3, 0x10, 0, 0x11, 0, 0x12, 0, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x1C, 0xFF, 0x1D,
					0xFF, 0x1E, 0xFF, 0x07, 0x07};
	u8 breBuf[58] = {0x01, 0x01, 0x03, 3, 0x04, 0x01, 0x05, 0x01, 0x06, 0x01, 0x08, 0x00, 0x10, 0, 0x11, 0, 0x12, 0, 0x1C,
					0xFF, 0x1D, 0xFF, 0x1E, 0xFF, 0x30, Trise_on, 0x35, Trise_on, 0x3A, Trise_on, 0x31, Tfall_off, 0x36,
					Tfall_off, 0x3B, Tfall_off, 0x32, 0x00, 0x37, 0x00, 0x3C, 0x00, 0x33, 0x00, 0x38, 0x00, 0x3D, 0x00,
					0x34, 0x00, 0x39, 0x00, 0x3E, 0x00, 0x09, 0x07, 0x07, 0x07};
	u8 offBuf[12] = {0x10, 0, 0x11, 0, 0x12, 0, 0x07, 0x00, 0x01, 0x00, 0x00, 0x55};

	if (NULL == buf)
		return -EINVAL;

	if (0 == count)
		return 0;

	if (sscanf(buf, "%d,%d,%d,%d,%d,%d", &data[0],&data[1],&data[2],&data[3],&data[4],&data[5]) == 6) {
		totalMS = data[3] * 50;//freq = 100;
		onMS = 80 * totalMS / 255;//pwm = 80;
		AW_ERR("data[0]=%d, data[1]=%d, data[2]=%d,data[3]=%d,data[4]=%d,data[5]=%d,\n", data[0],
			data[1],data[2],data[3],data[4],data[5]);

		if(data[4] < 2) {
			id = data[4];
			led_num = 1;
		} else {
			led_num = 2;
		}

		if (0 == data[5]) {
			sled_stop_crl_bre_timer();

			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return count;
			}
			for (i = 0; i < led_num; i++) {
				if (led_num >= DLED_NUM) {
					id = i;
				}
				if (g_the_chip[id] == NULL) {
					continue;
				}
				g_the_chip[id]->color_D1 = data[0];
				g_the_chip[id]->color_D2 = data[1];
				g_the_chip[id]->color_D3 = data[2];
				g_the_chip[id]->freq = data[3];
				g_the_chip[id]->led_id = data[4];
				g_the_chip[id]->led_on = data[5];
				onBuf[3] = g_the_chip[id]->aw2026_max_current;
				onBuf[5] = g_the_chip[id]->color_D1;
				onBuf[7] = g_the_chip[id]->color_D2;
				onBuf[9] = g_the_chip[id]->color_D3;

				//aw2026_i2c_write_byte(g_the_chip[id], 0x00, 0x55);// all reset
				//msleep(2);
				aw2026_i2c_write(g_the_chip[id]->client, onBuf, 12);
				/*aw2026_i2c_write_byte(g_the_chip[id], 0x01, 0x01);// GCR the device enters active state
				//aw2026_i2c_write_byte(g_the_chip[id], 0x03, g_the_chip[id]->aw2026_max_current);// IMAX max led output Current Select
				aw2026_i2c_write_byte(g_the_chip[id], 0x03, g_the_chip[id]->aw2026_max_current);// set  output max current is 12.75mA
				AW_ERR("g_the_chip[%d]->aw2026_max_current = %d\n", i, g_the_chip[id]->aw2026_max_current);
				aw2026_i2c_write_byte(g_the_chip[id], 0x10, g_the_chip[id]->color_D1);// R
				aw2026_i2c_write_byte(g_the_chip[id], 0x11, g_the_chip[id]->color_D2);// G
				aw2026_i2c_write_byte(g_the_chip[id], 0x12, g_the_chip[id]->color_D3); // B

				aw2026_i2c_write_byte(g_the_chip[id], 0x04, 0x00);// LCFG1 led operating mode select
				aw2026_i2c_write_byte(g_the_chip[id], 0x05, 0x00);// LCFG2
				aw2026_i2c_write_byte(g_the_chip[id], 0x06, 0x00); // LCFG3

				aw2026_i2c_write_byte(g_the_chip[id], 0x1C, 0xFF); // PWM1
				aw2026_i2c_write_byte(g_the_chip[id], 0x1D, 0xFF); // PWM2
				aw2026_i2c_write_byte(g_the_chip[id], 0x1E, 0xFF); // PWM3
				aw2026_i2c_write_byte(g_the_chip[id], 0x07, 0x07);*///   LEDEN led output enalbe
			}
		} else if (1 == data[5]){
			sled_stop_crl_bre_timer();
			if (33 == data[3]) {
				totalMS = 1000;
				riseMS = 300;
				onMS = 100;
				fallMS = 200;
				offMS = 230;
			} else if (22 == data[3]) {
				totalMS = 1250;
				riseMS = 500;
				onMS = 100;
				fallMS = 500;
				offMS = 100;
			} else if (44 == data[3]) {
				totalMS = 870;
				riseMS = -10;
				onMS = 200;
				fallMS = 500;
				offMS = 100;
			}
			//offMS_reg = totalMS - onMS;
			riseMS_reg = timetoreg(riseMS,time_edge_buf,sizeof(time_edge_buf)/4);//trise and tfall
			onMS_reg = timetoreg(onMS,time_stable_buf,sizeof(time_stable_buf)/4);//ton
			fallMS_reg = timetoreg(fallMS,time_edge_buf,sizeof(time_edge_buf)/4);//tfall
			offMS_reg =  timetoreg(offMS-10,time_stable_buf,sizeof(time_stable_buf)/4);//toff
			Trise_on =  ((riseMS_reg << 4 ) | onMS_reg) & 0xff;
			Tfall_off = ((fallMS_reg << 4 ) | offMS_reg) & 0xff;
			AW_ERR("Trise_on = 0x%x, Tfall_off = 0x%x.\n", Trise_on, Tfall_off);
			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return count;
			}
			global_i2c_mutex_lock();
			msleep(5);
			for (i = 0; i< led_num; i++) {
				if (led_num >= DLED_NUM) {
					id = i;
				}
				if (g_the_chip[id] == NULL) {
					continue;
				}
				g_the_chip[id]->totalMS = totalMS;
				g_the_chip[id]->onMS = time_edge_buf[riseMS_reg]+time_stable_buf[onMS_reg]+time_edge_buf[fallMS_reg];
				g_the_chip[id]->offMS = offMS;
				AW_ERR("g_the_chip[id]->totalMS=%d, g_the_chip[id]->onMS=%d, g_the_chip[id]->offMS=%d,riseMS=%d,onMS=%d,fallMS=%d \n", g_the_chip[id]->totalMS, g_the_chip[id]->onMS,
					g_the_chip[id]->offMS, riseMS, onMS, fallMS);

				g_the_chip[id]->color_D1 = data[0];
				g_the_chip[id]->color_D2 = data[1];
				g_the_chip[id]->color_D3 = data[2];
				g_the_chip[id]->freq = data[3];
				g_the_chip[id]->led_id = data[4];
				g_the_chip[id]->led_on = data[5];
				breBuf[3] =  g_the_chip[id]->aw2026_max_current;
				breBuf[13] = g_the_chip[id]->color_D1;
				breBuf[15] = g_the_chip[id]->color_D2;
				breBuf[17] = g_the_chip[id]->color_D3;
				breBuf[25] = breBuf[27] = breBuf[29] = Trise_on;
				breBuf[31] = breBuf[33] = breBuf[35] = Tfall_off;

				//aw2026_i2c_write_byte(g_the_chip[id], 0x00, 0x55);// all reset
				//msleep(2);
				aw2026_i2c_write(g_the_chip[id]->client, breBuf, 29);
				//aw2026_i2c_write_byte(g_the_chip[id], 0x07, 0x00);// set LEDEN led output disalbe
				/*aw2026_i2c_write_byte(g_the_chip[id], 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
				//aw2026_i2c_write_byte(g_the_chip[id], 0x03, g_the_chip[id]->aw2026_max_current);// set  output max current is 12.75mA
				aw2026_i2c_write_byte(g_the_chip[id], 0x03, g_chip->aw2026_max_current);// set  output max current is 12.75mA
				AW_ERR("g_the_chip[%d]->aw2026_max_current = %d\n", i, g_chip->aw2026_max_current);
				aw2026_i2c_write_byte(g_the_chip[id], 0x04, 0x01);// LCFG1 led pattern mode
				aw2026_i2c_write_byte(g_the_chip[id], 0x05, 0x01);// LCFG2 led pattern mode
				aw2026_i2c_write_byte(g_the_chip[id], 0x06, 0x01); // LCFG3 led pattern mode
				aw2026_i2c_write_byte(g_the_chip[id], 0x08, 0x00);//  independent mode

				aw2026_i2c_write_byte(g_the_chip[id], 0x10, g_the_chip[id]->color_D1);// R
				aw2026_i2c_write_byte(g_the_chip[id], 0x11, g_the_chip[id]->color_D2);// G
				aw2026_i2c_write_byte(g_the_chip[id], 0x12, g_the_chip[id]->color_D3); // B
				aw2026_i2c_write_byte(g_the_chip[id], 0x1C, 0xFF); // PWM1
				aw2026_i2c_write_byte(g_the_chip[id], 0x1D, 0xFF); // PWM2
				aw2026_i2c_write_byte(g_the_chip[id], 0x1E, 0xFF); // PWM3

				aw2026_i2c_write_byte(g_the_chip[id], 0x30, Trise_on); // PAT1_T1 Trise & Ton (7:4 rise  3:0 on) ris:0.512S on 0.512s
				aw2026_i2c_write_byte(g_the_chip[id], 0x35, Trise_on); // PAT2_T1 Trise & Ton (7:4 rise  3:0 on) ris:0.512S on 0.512s
				aw2026_i2c_write_byte(g_the_chip[id], 0x3A, Trise_on); // PAT3_T1 Trise & Ton (7:4 rise  3:0 on) ris:0.512S on 0.512s
				aw2026_i2c_write_byte(g_the_chip[id], 0x31, Tfall_off); //PAT1_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
				aw2026_i2c_write_byte(g_the_chip[id], 0x36, Tfall_off); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
				aw2026_i2c_write_byte(g_the_chip[id], 0x3B, Tfall_off); //PAT3_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s

				aw2026_i2c_write_byte(g_the_chip[id], 0x32, 0x00);// PAT1_T3  Tdelay
				aw2026_i2c_write_byte(g_the_chip[id], 0x37, 0x00);// PAT2_T3  Tdelay
				aw2026_i2c_write_byte(g_the_chip[id], 0x3C, 0x00);// PAT3_T3  Tdelay
				aw2026_i2c_write_byte(g_the_chip[id], 0x33, 0x00);// PAT1_T4  PAT_CTR & Color
				aw2026_i2c_write_byte(g_the_chip[id], 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
				aw2026_i2c_write_byte(g_the_chip[id], 0x3D, 0x00);// PAT3_T4  PAT_CTR & Color
				aw2026_i2c_write_byte(g_the_chip[id], 0x34, 0x00); // PAT1_T5 Timer
				aw2026_i2c_write_byte(g_the_chip[id], 0x39, 0x00); // PAT2_T5 Timer
				aw2026_i2c_write_byte(g_the_chip[id], 0x3E, 0x00); // PAT3_T5 Timer

				aw2026_i2c_write_byte(g_the_chip[id], 0x09, 0x07); //  PAT_RUN
				aw2026_i2c_write_byte(g_the_chip[id], 0x07, 0x07);*///  LEDEN led output enalbe
				AW_ERR("g_the_chip[%d] crl end.\n", i);
			}
			msleep(5);
			global_i2c_mutex_unlock();
			if (33 == data[3]) {
				sled_start_crl_bre_timer();
			} else {
				sled_stop_led_work();
			}

		} else {

			if(aw2026_lv8_enable(true)) {
				AW_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return count;
			}
			sled_stop_crl_bre_timer();
			for (i = 0; i< led_num; i++) {
				if (led_num >= DLED_NUM) {
					id = i;
				}
				if (g_the_chip[id] == NULL) {
					continue;
				}
				//g_the_chip[id]->color_D1 = 0;
				//g_the_chip[id]->color_D2 = 0;
				//g_the_chip[id]->color_D3 = 0;
				g_the_chip[id]->freq = 0;
				g_the_chip[id]->led_id = 0;
				g_the_chip[id]->led_on = 2;
				aw2026_i2c_write(g_the_chip[id]->client, offBuf, 6);
				/*aw2026_i2c_write_byte(g_the_chip[id], 0x10, g_the_chip[id]->color_D1);// R
				aw2026_i2c_write_byte(g_the_chip[id], 0x11, g_the_chip[id]->color_D2);// G
				aw2026_i2c_write_byte(g_the_chip[id], 0x12, g_the_chip[id]->color_D3); // B
				//turn off led when 0x01 is  0x00,there is set to same as breath leds
				aw2026_i2c_write_byte(g_the_chip[id], 0x07, 0x00);				  //set close all led
				aw2026_i2c_write_byte(g_the_chip[id], 0x01, 0x00);	//set sleep/shutdown mode
				aw2026_i2c_write_byte(g_the_chip[id], 0x00, 0x55);*/// all reset
				AW_ERR("oppo_led_debug   end \n");
			}
			if(aw2026_lv8_enable(false)) {
				AW_ERR("set_breathlight_brightness Regulator disable vcc_i2c failed\n");
				return count;
			}
		}
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
		g_the_chip[0]->breath_leds = 0;
		g_the_chip[0]->led_on_off = 0;
		aw20xx_led_off();
	 }
	 else if( buf[0] == '1'){
	 	pr_err("oppo_led_debug %s  is lowbattery led on \n", __func__);
		g_the_chip[0]->breath_leds = 1;
		g_the_chip[0]->led_on_off = 1;
		aw20xx_lowbattery_breath_leds();
	 }
	 else if( buf[0] == '2'){
	 	pr_err("oppo_led_debug %s  is events led on\n", __func__);
		g_the_chip[0]->breath_leds = 2;
		g_the_chip[0]->led_on_off = 2;
		aw20xx_events_breath_leds();
	 }
	 else{
	 	pr_err("oppo_led_debug %s  is  led on\n", __func__);
		g_the_chip[0]->breath_leds = 255;
		g_the_chip[0]->led_on_off = 3;
		aw20xx_led_on();
	 }
	return count;

}

static ssize_t Breathled_switch_show2 ( struct device *dev,
									  struct device_attribute *attr, char *buf )
{
   return sprintf ( buf, "value :%ld\n", g_the_chip[0]->value);
}
static ssize_t Breathled_switch_store2 ( struct device *dev,
									   struct device_attribute *attr, const char *buf, size_t count )
{
	//unsigned long value = 0;
	int ret;

	if (NULL == buf)
		return -EINVAL;

	ret = kstrtoul(buf, 0, &g_the_chip[0]->value);
	if(ret < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, ret=%d\n", __func__, ret);
		return ret;
	}
	//sprintf ( buf, "%d\n", sn3191_breath_led);
	aw20xx_breath_leds_time(g_the_chip[0]->value);
	g_the_chip[0]->breath_leds = 1;

	return count;

}

//on: 1s off:2s
static ssize_t sled_grppwm_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);
	g_the_chip[0]->onMS = value * g_the_chip[0]->totalMS / 255;
	if(g_the_chip[0]->onMS > 8000)
	 g_the_chip[0]->onMS = 8000;
	return count;
}

static ssize_t sled_grpfreq_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);
	g_the_chip[0]->totalMS = value * 50;
	if(g_the_chip[0]->totalMS > 16000)
		g_the_chip[0]->totalMS = 16000;
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

	offMS_reg = g_the_chip[0]->totalMS - g_the_chip[0]->onMS;

	if (value == 1 && g_the_chip[0]->totalMS && g_the_chip[0]->onMS) {
		EdgeMS_reg = timetoreg(g_the_chip[0]->onMS/3,time_edge_buf,sizeof(time_edge_buf)/4);			//trise and tfall
		onMS_reg = timetoreg(g_the_chip[0]->onMS/3,time_stable_buf,sizeof(time_stable_buf)/4);			//ton
		offMS_reg =  timetoreg(offMS_reg/3,time_stable_buf,sizeof(time_stable_buf)/4);		//toff

		Trise_on =  ((EdgeMS_reg << 4 ) | onMS_reg) & 0xff;
		Tfall_off = ((EdgeMS_reg << 4 ) | offMS_reg) & 0xff;

		pr_err("oppo_led_debug %s  totMS=%d,onMS=%d,EdgeMS_reg=%x,onMS_reg=%x,color_D1=%d,color_D2=%d,color_D3=%d,Trise_on=%x,Tfall_off=%x \n",
			__func__,g_the_chip[0]->totalMS,g_the_chip[0]->onMS,EdgeMS_reg,onMS_reg, g_the_chip[0]->color_D1, g_the_chip[0]->color_D2, g_the_chip[0]->color_D3, Trise_on, Tfall_off);

			//we fix the breath time because of different IC has different time value
		//aw
		if(aw2026_lv8_enable(true)) {
			pr_err("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
			return -1;
		}
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x00);// set LEDEN led output disalbe
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state,pwm frq is 122Hz
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// set  output max current is 12.75mA

		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x01);// LCFG2 led pattern mode
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x08, 0x00);//  independent mode

		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2

		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x35, Trise_on); // PAT2_T1 Trise & Ton (7:4 rise  3:0 on) ris:0.512S on 0.512s
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x36, Tfall_off); //PAT2_T2  Tfall & Toff(7:4 fall  3:0 off) fall: 0.512S off:2.1s
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x37, 0x00);// PAT2_T3  Tdelay
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x38, 0x00);// PAT2_T4  PAT_CTR & Color
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x39, 0x00); // Timer

		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x02); //  PAT_RUN
		i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//  LEDEN led output enalbe

	}else {
		if((g_the_chip[0]->color_D1+g_the_chip[0]->color_D2+g_the_chip[0]->color_D3) == 0) {
	  		aw20xx_led_off();
		}else {

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x01, 0x01);// GCR the device enters active state
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x03, g_the_chip[0]->aw2026_max_current);// IMAX max led output Current Select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x11, g_the_chip[0]->color_D2);// G

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x04, 0x00);// LCFG1 led operating mode select
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x05, 0x00);// LCFG2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x06, 0x00); // LCFG3

			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x1D, 0xFF); // PWM2
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x07, 0x02);//   LEDEN led output enalbe
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

static ssize_t sled_shutdown_bre_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret;
	int value;
	if (NULL == buf)
		return -EINVAL;

	ret = sscanf(buf, "%d", &value);
	if(ret < 0){
		printk(KERN_ERR "%s:strict_strtoul failed, ret=%d\n", __func__, ret);
		return ret;
	}
	AW_ERR("value = %d,\n", value);
	if (value == 1) {
		aw20xx_led_breath();
	}
	return count;
}

static ssize_t sled_reg_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned int databuf[2];

	if (NULL == buf)
		return -EINVAL;

	if(2 == sscanf(buf, "%x %x", &databuf[0], &databuf[1])) {
		if (g_the_chip[0] != NULL) {
			i2c_smbus_write_byte_data(g_the_chip[0]->client, databuf[0], databuf[1]); // write reg
		}
		if (g_the_chip[1] != NULL) {
			i2c_smbus_write_byte_data(g_the_chip[1]->client, databuf[0], databuf[1]); //  write reg
		}
	} else {
        AW_ERR("reg write faled.\n");
    }

	return count;
}


static ssize_t sled_power_enable_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret;
	int value;
	if (NULL == buf)
		return -EINVAL;

	ret = sscanf(buf, "%d", &value);
	if(ret < 0){
		printk(KERN_ERR "%s:strict_strtoul failed, ret=%d\n", __func__, ret);
		return ret;
	}
	AW_ERR("value = %d,\n", value);
	if (value == 1) {
		aw2026_lv8_enable(true);
	} else {
		aw2026_lv8_enable(false);
	}
	return count;
}


static ssize_t sled_bre_shutdown_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret;
	int value;
	if (NULL == buf)
		return -EINVAL;

	ret = sscanf(buf, "%d", &value);
	if(ret < 0){
		printk(KERN_ERR "%s:strict_strtoul failed, ret=%d\n", __func__, ret);
		return ret;
	}
	pr_err("%s, value=%x \n",__func__,value);
	if (value == 1) {
		if (g_the_chip[0] != NULL) {
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x07); //  PAT_RUN
		}
		if (g_the_chip[1] != NULL) {
			i2c_smbus_write_byte_data(g_the_chip[1]->client, 0x09, 0x07); //  PAT_RUN
		}
	} else {
		if (g_the_chip[0] != NULL) {
			i2c_smbus_write_byte_data(g_the_chip[0]->client, 0x09, 0x70); //  PAT_STOP
		}
		if (g_the_chip[1] != NULL) {
			i2c_smbus_write_byte_data(g_the_chip[1]->client, 0x09, 0x70); //  PAT_STOP
		}
	}
	return count;
}

static ssize_t sled_bre_offMS_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret = 0;
	int value = 0;

	if (NULL == buf)
		return -EINVAL;

	if (0 == count)
		return 0;

	ret = sscanf(buf, "%d", &value);
	if(ret < 0){
		printk(KERN_ERR "%s:strict_strtoul failed, ret=%d\n", __func__, ret);
		return ret;
	}
	AW_ERR("value=%x \n", value);
	g_chip->offMS = value * 10;
	g_chip->totalMS = g_chip->offMS + g_chip->onMS;
	AW_ERR("g_chip->totalMS = %d, g_chip->offMS = %d \n", g_chip->totalMS, g_chip->offMS);

	return count;
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
static DEVICE_ATTR(bre_shutdown, S_IWUSR | S_IRUGO, NULL, sled_bre_shutdown_store);
static DEVICE_ATTR(bre_offMS, S_IWUSR | S_IRUGO, NULL, sled_bre_offMS_store);
static DEVICE_ATTR(max_current, S_IWUSR | S_IRUGO, set_max_current_show, set_max_current_store);
static DEVICE_ATTR(shutdown_color, S_IWUSR | S_IRUGO, set_shutdown_colour_show, set_shutdown_colour_store);
static DEVICE_ATTR(shutdown_bre, S_IWUSR | S_IRUGO, NULL, sled_shutdown_bre_store);
static DEVICE_ATTR(power_enable, S_IWUSR | S_IRUGO, NULL, sled_power_enable_store);
static DEVICE_ATTR(reg, S_IWUSR | S_IRUGO, NULL, sled_reg_store);



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
	&dev_attr_bre_shutdown.attr,
	&dev_attr_bre_offMS.attr,
	&dev_attr_max_current.attr,
	NULL
};
static const struct attribute_group blink_attr_group = {
	.attrs = blink_attributes,
};


static struct attribute *leds_attributes[] = {
	&dev_attr_color.attr,
	&dev_attr_shutdown_color.attr,
	&dev_attr_shutdown_bre.attr,
	&dev_attr_power_enable.attr,
	&dev_attr_reg.attr,
	NULL
};
static const struct attribute_group leds_attr_group = {
	.attrs = leds_attributes,
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
		g_the_chip[0]->breath_leds = 0;
		g_the_chip[0]->led_on_off = 0;
	   aw20xx_led_off();
	 }
	 else if( buf[0] == '1'){
		g_the_chip[0]->breath_leds = 1;
		g_the_chip[0]->led_on_off = 1;
		aw20xx_lowbattery_breath_leds();
	 }
	 else if( buf[0] == '2'){
		g_the_chip[0]->breath_leds = 2;
		g_the_chip[0]->led_on_off = 2;
		aw20xx_events_breath_leds();
	 }
	 else{
		g_the_chip[0]->breath_leds = 255;
		g_the_chip[0]->led_on_off = 3;
		aw20xx_led_on();
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
	AW_ERR("oppo_led_debug is probe start \n");

	chip = devm_kzalloc(&client->dev,
		sizeof(struct aw2026_chip), GFP_KERNEL);

	if (!chip) {
		AW_ERR(" devm_kzalloc() failed\n");
		return -ENOMEM;
	}
	load_num ++;
	chip->client = client;
	chip->led_on_off = 255;
	chip->power_on_off = 0;
	chip->breath_leds = 0;
	chip->aw20xx_max_brightness = 0xFF;
	chip->aw2026_max_current = 0x02;
	chip->onMS = 0;
	chip->totalMS = 0;
	chip->offMS = 0;
	chip->value = 0;
	chip->color_D1 = 0;
	chip->color_D2 = 0;
	chip->color_D3 = 0;
	chip->col_D1 = 0;
	chip->col_D2 = 0;
	chip->col_D3 = 0;
	chip->pcb_verison = -1;
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
	ret = of_property_read_u32(client->dev.of_node, "aw2026,id", &tmp);
	if (!ret)
	{
		chip->id = tmp;   //id
	} else {
		chip->id = load_num -1;   //<0>;
	}

	chip->pcb_verison = get_PCB_Version();
	AW_ERR("the pcb_verison is %d.\n", chip->pcb_verison);

	chip->vcc_1v8 = regulator_get(&client->dev, "vdd_1v8");
	if( IS_ERR(chip->vcc_1v8) ){
		ret = PTR_ERR(chip->vcc_1v8);
		AW_ERR("Regulator get ss failed vcc_i2c rc=%d\n", ret);
	}
	if (chip->pcb_verison <= HW_VERSION__12) {
		if (!IS_ERR(chip->vcc_1v8)){
			if(regulator_count_voltages(chip->vcc_1v8) > 0) {
				ret = regulator_set_voltage(chip->vcc_1v8, 1800000,1800000);
				if (ret) {
					AW_ERR("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
				}
				else{
					ret = regulator_enable(chip->vcc_1v8);
					if(ret) {
						AW_ERR("Regulator vcc_i2c enable failed rc=%d\n", ret);
					}
				}
			}
		}
	}
	/* create i2c struct, when receve and transmit some byte */
	AW_ERR("oppo_led_debug is probe start1 \n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev,
				"%s: check_functionality failed.", __func__);
		ret = -ENODEV;
		if (chip->pcb_verison <= HW_VERSION__12) {
			ret = regulator_disable(chip->vcc_1v8);
			if(ret) {
				AW_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
			}
		}
		goto err_i2c_check;
	}
	AW_ERR("oppo_led_debug is probe start2 \n");

	ret = i2c_smbus_write_byte_data(chip->client, 0x00, 0x55);//reset reg
	if(ret < 0){
		printk("reset aw2026 led control ic fail,ret =%d\n",ret);
		mdelay(2);
		ret = i2c_smbus_write_byte_data(chip->client, 0x00, 0x55);//reset reg
		if(ret < 0){
			if (chip->pcb_verison <= HW_VERSION__12) {
				ret = regulator_disable(chip->vcc_1v8);
				if(ret) {
					AW_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
				}
			}
			goto err_i2c_check;
		}
	}

	ret = i2c_smbus_write_byte_data(chip->client, 0x03, chip->aw2026_max_current);		// set	output max current is 12.75mA
	ret = i2c_smbus_write_byte_data(chip->client, 0x11, chip->aw20xx_max_brightness);		// set	output max current is 2.048mA
	ret = i2c_smbus_write_byte_data(chip->client, 0x07, 0x00);				  //set close all led
	ret = i2c_smbus_write_byte_data(chip->client, 0x01, 0x00);	//set sleep/shutdown mode
	if(ret < 0){
		printk("can't find aw2026 led control ic!ret =%d",ret);
		goto err_i2c_check;
	}
	if (chip->pcb_verison <= HW_VERSION__12) {
		ret = regulator_disable(chip->vcc_1v8);
		if(ret) {
			AW_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
		}
	}
	g_chip = chip;

	/*************Added by Tong.han for same interface with 14037*******************/
	AW_ERR("oppo_led_debug is probe start3 \n");
	if(load_num == 1) {
		for(i = 0; i < 1; i ++ ) {
			if (led_classdev_register(&client->dev, &AW2026_lcds[i])) {
						printk(KERN_ERR "led_classdev_register failed of AW2026_lcds!\n");
			}
		}
	}
	ret = sysfs_create_group(&client->dev.kobj, &blink_attr_group);
	if (ret) {
		AW_ERR( "sysfs_create_group failed!\n");
		goto err_group_register;
	}
	AW_ERR("oppo_led_debug is probe start4 \n");

	if(load_num == 1) {
		AW_ERR("oppo_led_debug is probe start5 \n");
		chip->leds_func_kobj = kobject_create_and_add("leds", NULL);
		if (chip->leds_func_kobj == NULL) {
			printk(KERN_ERR "%s: kobject_create_and_add failed\n", __func__);
		} else {
			ret = sysfs_create_group(chip->leds_func_kobj, &leds_attr_group);
			if (ret < 0) {
				printk(KERN_ERR "%s: sysfs_create_group failed\n", __func__);
				kobject_put(chip->leds_func_kobj);
			}
		}
	}

	/* create device node sys/class/ktd20xx/ktd2206/led
	 * led: 0:breath led 1:open led  2:close led
	 */
	chip->timeout_workqueue = create_singlethread_workqueue("led_timeout");
	if (!chip->timeout_workqueue) {
		ret = -ENOMEM;
		goto err_class_create;
	}

	hrtimer_init(&chip->watchdog, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	chip->watchdog.function = sled_bre_timeout;
	INIT_WORK(&chip->timeout_work, sled_bre_timeout_work);

	chip->major = register_chrdev(0, "ktd20xx", &aw20xx_fops);
	if(load_num == 1) {
		chip->aw20xx_cls = class_create(THIS_MODULE, "aw20xx");
		if (IS_ERR(chip->aw20xx_cls)) {
			ret = IS_ERR(chip->aw20xx_cls);
			AW_ERR("perfmap: Error in class_create aw20xx_cls\n");
			goto err_class_create;
		AW_ERR("oppo_led_debug is probe start5 \n");
		chip->aw20xx_dev = device_create(chip->aw20xx_cls, NULL, MKDEV(chip->major, 0), NULL, "aw2026"); /* /dev/ktd20xx */

		if (device_create_file(chip->aw20xx_dev, &dev_attr_led) < 0)
			AW_ERR("Failed to create device file(%s)!\n", dev_attr_led.attr.name);
		if (device_create_file(chip->aw20xx_dev, &dev_attr_breath_led) < 0)
			AW_ERR("Failed to create device file(%s)!\n", dev_attr_led.attr.name);
		}
	}
	AW_ERR("oppo_led_debug is probe start6 \n");
	if (chip->id < DLED_NUM) {
		g_the_chip[chip->id] = chip;
		AW_ERR("aw20xx id[%d], client addr [0x%02x]\n",chip->id, client->addr);
	} else {
		AW_ERR("aw20xx id[%d] >= DLED_NUM[%d],fail!\n",chip->id, DLED_NUM);
		goto err_class_create;
	}
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
	//printk("%s\n",__func__);
	AW_ERR("call\n");
	//aw20xx_led_off();
	//aw20xx_led_breath();
	return;
}

static int aw20xx_remove(struct i2c_client *client){

	AW_ERR("%s\n",__func__);
	return 0;
}


static const struct i2c_device_id aw20xx_id[] = {
	{AW_I2C_NAME, 0},
	{ }
};


static struct of_device_id aw20xx_match_table[] = {
		{ .compatible = "aw,aw2026",},
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

static int __init aw20xx_init(void)
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

static void __exit aw20xx_exit(void)
{
	printk("%s\n",__func__);
	i2c_del_driver(&aw20xx_driver);
}

module_init(aw20xx_init);
module_exit(aw20xx_exit);

MODULE_LICENSE("GPL");

