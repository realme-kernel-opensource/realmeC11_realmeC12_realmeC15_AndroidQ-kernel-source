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
**         v1.0  	2017/4/20         Fei.Mo@EXP.BSP.Sensor  	Add for KTD2026 driver
**         v1.1  	2017/7/08         Fei.Mo@EXP.BSP.Sensor  	Modify to support MTK platform
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
#include "oppo_ktd2026.h"
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <soc/oppo/oppo_project.h>

#define KTD_I2C_NAME "ktd2026"

#define DEVINFO_NAME KTD_I2C_NAME

#define log_fmt(fmt) "[line:%d][module:%s][%s] " fmt

#define KTD_ERR(a,arg...)\
		printk(KERN_NOTICE log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

#define KTD_MSG(a, arg...) \
		printk(KERN_INFO log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

#define DLED_NUM 2
static struct ktd2026_chip *g_the_chip[DLED_NUM] = {NULL,NULL};
static struct ktd2026_chip *g_chip = NULL;
static int time_edge_buf[16] = {2,128,256,384,512,640,768,896,1024,1152,1280,1408,1536,1664,1792,1920};
static int time_stable_buf[16] = {128,384,512,640,768,896,1024,1152,1280,1408,1536,1664,1792,1920,2048,2176};
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

void ktd22xx_events_breath_leds(void);

#if CONFIG_SUPPORT_BUTTON_BACKLIGHT
static void set_button_backlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value);
#endif

#if CONFIG_SUPPORT_BREATHLIGHT
static void set_breathlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value);
#endif


static struct led_classdev KTD2026_lcds[] = {
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


static DEFINE_MUTEX(ktd22xx_i2c_access);

static int32_t ktd22xx_i2c_write_byte(struct ktd2026_chip *chip, uint8_t reg, uint8_t val)
{
	int32_t ret = 0;
	struct i2c_client *client = chip->client;

	mutex_lock(&ktd22xx_i2c_access);

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		KTD_ERR("failed to write register %x err= %d\n", reg, ret);
	}

	mutex_unlock(&ktd22xx_i2c_access);

	return ret;
}
static int ktd22xx_reset(struct ktd2026_chip *chip)
{
	int32_t ret = 0;

	//ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x07);//reset reg
	ret |= ktd22xx_i2c_write_byte(chip, 0x06, 0x00);//set current is 0.125mA
	if(ret < 0){
		KTD_ERR("reset ktd2026 led control ic fail,ret =%d\n",ret);
		return ret;
	}

	ret |= ktd22xx_i2c_write_byte(chip, 0x07, 0x00);//set current is 0.125mA
	ret |= ktd22xx_i2c_write_byte(chip, 0x08, 0x00);//set current is 0.125mA
	ret |= ktd22xx_i2c_write_byte(chip, 0x04, 0x00);//turn off leds
	ret |= ktd22xx_i2c_write_byte(chip, 0x00, 0x08);//set sleep mode
	return ret;
}
#define MAX_I2C_RETRY_TIME 2

int ktd2026_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int retval = 0;
	int retry = 0;
	int i = 0;
	struct i2c_msg msgs[writelen];

	if (client == NULL) {
		KTD_ERR("i2c_client == NULL!");
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
		KTD_ERR("i2c_transfer(write) over retry limit\n");
		retval = -EIO;
	}

	return retval;
}

void ktd2xx_led_off(void)
{
	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return;
	}

	/* turn on led when 0x02 is not 0x00,
	     there is set to same as breath leds  */
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x06, 0x00);//set current is 0.125mA
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x07, 0x00);//set current is 0.125mA
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x08, 0x00);//set current is 0.125mA
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x04, 0x00);//turn off leds
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x28);//set sleep mode
}

EXPORT_SYMBOL(ktd2xx_led_off);

static void ktd22xx_close_one_led(struct ktd2026_chip *chip, int led, int EnableReg)
{
	chip->EnableReg &= ~EnableReg;

	ktd22xx_i2c_write_byte(chip, 0x04, chip->EnableReg);

	switch (led) {
	case LED1:
		ktd22xx_i2c_write_byte(chip, 0x06, 0x00);//set LED1 current is 00 mA
		break;
	case LED2:
		ktd22xx_i2c_write_byte(chip, 0x07, 0x00);//set LED1 current is 00 mA
		break;
	case LED3:
		ktd22xx_i2c_write_byte(chip, 0x08, 0x00);//set LED1 current is 00 mA
		break;
	default:
		break;
	}
}

static void ktd22xx_open_one_led(struct ktd2026_chip *chip, int led, int EnableReg)
{
	chip->EnableReg |= EnableReg;

	ktd22xx_i2c_write_byte(chip, 0x00, 0x20);// initialization LED off
	ktd22xx_i2c_write_byte(chip, 0x04, chip->EnableReg);

	switch (led) {
	case LED1:
		ktd22xx_i2c_write_byte(chip, 0x06, chip->value[0]);//set LED1 current
		break;
	case LED2:
		ktd22xx_i2c_write_byte(chip, 0x07, chip->value[1]);//set LED2 current
		break;
	case LED3:
		ktd22xx_i2c_write_byte(chip, 0x08, chip->value[2]);//set LED3 current
		break;
	default:
		break;
	}
}

void ktd22xx_breath_leds_time(struct ktd2026_chip *chip, int blink)
{
	/* set breath led flash time from blink */
	int period = 0;
	int flashtime = 0;
	period = (blink >> 8) & 0xff;
	flashtime = blink & 0xff;

	KTD_ERR("ktd20xx led write blink = %x, period = %x, flashtime = %x\n", blink, period, flashtime);

	/*initialization LED off*/
	ktd22xx_i2c_write_byte(chip, 0x00, 0x20);

	/*rase time*/
	ktd22xx_i2c_write_byte(chip, 0x05, period);

	/*dry flash period*/
	ktd22xx_i2c_write_byte(chip, 0x01, flashtime);

	/*reset internal counter*/
	ktd22xx_i2c_write_byte(chip, 0x02, 0x00);

	/*allocate led1 to timer1*/
	ktd22xx_open_one_led(chip, BREATHLIGHT_LED, BREATHLIGHT_PWM);

	/*led flashing(curerent ramp-up and down countinuously)*/
	ktd22xx_i2c_write_byte(chip, 0x02, 0x56);

}

#if CONFIG_SUPPORT_BUTTON_BACKLIGHT
static void set_button_backlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	KTD_MSG("ktd20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return;
	}

	if (value == 0) {
		ktd22xx_close_one_led(g_the_chip[0],BUTTON_BACKLIGHT_LED, BUTTON_BACKLIGHT_ALON);
		g_the_chip[0]->value[BUTTON_BACKLIGHT_LED] = 0;
	} else {
		g_the_chip[0]->value[BUTTON_BACKLIGHT_LED] = OUTPUT_10AM;
		ktd22xx_open_one_led(g_the_chip[0],BUTTON_BACKLIGHT_LED, BUTTON_BACKLIGHT_ALON);
	}

}
#endif /* CONFIG_SUPPORT_BUTTON_BACKLIGHT  */

static int ktd2026_lv8_enable_wq(bool enable)
{
	int ret = -1;
	if (g_chip == NULL) {
		KTD_ERR("g_chip is NULL\n");
		return ret;
	}

	if (g_chip->pcb_verison > HW_VERSION__12) {
		KTD_ERR("the pcb verison is VPT pcb_verison = %d.\n", g_chip->pcb_verison);
		return 0;
	}

	if (enable) {
			if(regulator_count_voltages(g_chip->vcc_1v8) > 0) {
				ret = regulator_set_voltage(g_chip->vcc_1v8, 1800000,1800000);
				if (ret) {
					KTD_ERR("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
				} else {
					ret = regulator_enable(g_chip->vcc_1v8);
					if(ret) {
						KTD_ERR("Regulator vcc_i2c enable failed rc=%d\n", ret);
					}
				}
		} else {
			return 0;
		}
	} else {
			ret = regulator_disable(g_chip->vcc_1v8);
			if(ret) {
				KTD_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
			}
	}
	return ret;
}

static int ktd2026_lv8_enable(bool enable)
{
	int ret = -1;
	if (g_chip == NULL) {
		KTD_ERR("g_chip is NULL\n");
		return ret;
	}

	if (g_chip->pcb_verison > HW_VERSION__12) {
		KTD_ERR("the pcb verison is VPT pcb_verison = %d.\n", g_chip->pcb_verison);
		return 0;
	}

	if (enable) {
		if(!g_chip->power_on_off) {
			if(regulator_count_voltages(g_chip->vcc_1v8) > 0) {
				ret = regulator_set_voltage(g_chip->vcc_1v8, 1800000,1800000);
				if (ret) {
					KTD_ERR("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
				} else {
					ret = regulator_enable(g_chip->vcc_1v8);
					g_chip->power_on_off = 1;
					if(ret) {
						KTD_ERR("Regulator vcc_i2c enable failed rc=%d\n", ret);
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
				KTD_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
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
	KTD_MSG("ktd20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return;
	}

	if (value > MAX_BACKLIGHT_BRIGHTNESS) {
		value = MAX_BACKLIGHT_BRIGHTNESS;
	}

	if (!strcmp(led_cdev->name, "led1")) {
		g_the_chip[0]->value[BREATHLIGHT_LED] = value;
	} else {
		g_the_chip[0]->value[BREATHLIGHT_LED] = 0;
	}

	#else /*CONFIG_MTK_PTLATFORM*/

	int Ttotal_ms = 80 * 50;//freq = 80
	int Ton_ms = 128 * Ttotal_ms / 255;//pwm = 128
	int totalMS_reg = 0;
	int onMS_reg = 0;
	int stableMS_reg = 0;

	totalMS_reg = (Ttotal_ms - 256) / 128;
	onMS_reg = (Ton_ms / 3 / 192) & 0x0F;
	stableMS_reg = Ton_ms * 250 / 3 / Ttotal_ms * 2;
	onMS_reg = onMS_reg|(onMS_reg << 4);

	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return;
	}

	KTD_MSG("ktd20xx name:%s value:%d \n",led_cdev->name, value);

	if (!strcmp(led_cdev->name, "white")) {
		if( value == 0) {//close
			g_the_chip[0]->value[BREATHLIGHT_LED] = 0;
			ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED, (BREATHLIGHT_PWM | BREATHLIGHT_ALON));
		} else if (value == 1) {//blink
			g_the_chip[0]->value[BREATHLIGHT_LED] = BREATHLIGHT_CURRENT;
			ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x20);// initialization LED off
			ktd22xx_i2c_write_byte(g_the_chip[0], 0x05, onMS_reg);//rase time
			ktd22xx_i2c_write_byte(g_the_chip[0], 0x01, totalMS_reg);//dry flash period
			ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, 0x00);//reset internal counter stableMS_reg
			ktd22xx_open_one_led(g_the_chip[0], BREATHLIGHT_LED, BREATHLIGHT_PWM);//allocate led1 to timer1
			ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, stableMS_reg);//led flashing(curerent ramp-up and down countinuously)
		} else {//always on
			g_the_chip[0]->value[BREATHLIGHT_LED] = BREATHLIGHT_CURRENT;
			ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED,BREATHLIGHT_PWM);
			ktd22xx_open_one_led(g_the_chip[0], BREATHLIGHT_LED,BREATHLIGHT_ALON);
		}
	} else {
		g_the_chip[0]->value[BREATHLIGHT_LED] = 0;
	}
	#endif

}

#endif /* CONFIG_SUPPORT_BREATHLIGHT */

#define RESET_TIMEOUT_TIME 6000 * 1000 * 1000
#define ONE_SECOND 380

static void sled_stop_led_work(void)
{
	if(ktd2026_lv8_enable_wq(true)) {
		KTD_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	usleep_range(g_chip->onMS * 1000, g_chip->onMS * 1000);
	KTD_ERR("sled_stop_led_work start\n");

	if (g_the_chip[0] != NULL) {
		//ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x06); //  timer reset
		ktd22xx_i2c_write_byte(g_the_chip[0], 0x04, 0x00); //   reset
	}
	if (g_the_chip[1] != NULL) {
			//ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x06); //  timer reset
		ktd22xx_i2c_write_byte(g_the_chip[1], 0x04, 0x00); //   reset
	}
	if(ktd2026_lv8_enable_wq(false)) {
		KTD_ERR("set_breathlight_brightness Regulator disable vcc_i2c failed\n");
		return;
	}
	KTD_ERR("sled_stop_led_work end\n");
	return;
}

static void sled_bre_timeout_work(struct work_struct *work)
{
	if(ktd2026_lv8_enable_wq(true)) {
		KTD_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
		return;
	}
	usleep_range(g_chip->onMS * 1000, g_chip->onMS * 1000);

	//usleep_range(384 * 2 * 1000, 384 * 2 * 1000);
	KTD_ERR("sled_bre_timeout_work start\n");

	//ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x06);           //  timer reset
	//ktd22xx_i2c_write_byte(g_the_chip[1], 0x00, 0x06);           //  timer reset

	//usleep_range(232 * 1000, 232 * 1000);
	usleep_range(g_chip->offMS * 1000, g_chip->offMS * 1000);
	if (g_the_chip[0]->led_on) {
		if (g_the_chip[0] != NULL) {
			ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x06); //  timer reset
		}
		if (g_the_chip[1] != NULL) {
			ktd22xx_i2c_write_byte(g_the_chip[1], 0x00, 0x06); //  timer reset
		}
	} else {
		KTD_ERR("ctl brethea timeout work-----\n");
	}
	if(ktd2026_lv8_enable_wq(false)) {
		KTD_ERR("set_breathlight_brightness Regulator disable vcc_i2c failed\n");
		return;
	}
	KTD_ERR("sled_bre_timeout_work end\n");
	return;
}

static void sled_start_crl_bre_timer(void)
{
	 if (g_chip->crl_bre_watchdog_running == 0) {
		 KTD_ERR("hrtimer_start!!\n");

		 hrtimer_start(&g_chip->watchdog,
				 ktime_set(0, 0),
				 HRTIMER_MODE_REL);
		 g_chip->crl_bre_watchdog_running = 1;
	 }
}

static void sled_stop_crl_bre_timer(void)
{
	 if (g_chip->crl_bre_watchdog_running == 1) {
		 KTD_ERR("hrtimer_cancel!!\n");
		 hrtimer_cancel(&g_chip->watchdog);
		 g_chip->crl_bre_watchdog_running = 0;
	 }
}

static enum hrtimer_restart sled_bre_timeout(struct hrtimer *timer)
{

	struct ktd2026_chip *chip = g_chip;

	//queue_work(&(chip->timeout_work));
	queue_work_on(smp_processor_id(), chip->timeout_workqueue, &chip->timeout_work);
	//hrtimer_forward_now(&chip->watchdog, ktime_set(0, g_chip->totalMS * 1000 * 1000));
	hrtimer_forward_now(&chip->watchdog, ktime_set(0, g_chip->totalMS * 1000 * 1000));
	return HRTIMER_RESTART;  //restart the timer
}

void ktd22xx_lowbattery_breath_leds(void)
{
	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return;
	}
	/*
	 * flash time period: 2.5s, rise/fall 1s, sleep 0.5s
	 * reg5 = 0xaa, reg1 = 0x12
	 */
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x05, 0xaa);//rase time
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x01, 0x12);//dry flash period
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, 0x00);//reset internal counter
	ktd22xx_open_one_led(g_the_chip[0],BREATHLIGHT_LED,BREATHLIGHT_PWM);//allocate led1 to timer1
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, 0x56);//led flashing(curerent ramp-up and down countinuously)

}

void ktd22xx_events_breath_leds(void)
{
	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return;
	}
	/*
	 * flash time period: 5s, rise/fall 2s, sleep 1s
	 * reg5 = 0xaa, reg1 = 0x30
	 */
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x05, 0xaa);//rase time
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x01, 0x30);//dry flash period
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, 0x00);//reset internal counter
	ktd22xx_open_one_led(g_the_chip[0], BREATHLIGHT_LED, BREATHLIGHT_PWM);//allocate led1 to timer1
	ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, 0x56);//led flashing(curerent ramp-up and down countinuously)
}

static ssize_t Breathled_switch_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return sprintf(buf, "%d\n", 0);
	}
	return sprintf ( buf, "%d\n", g_the_chip[0]->breath_leds);
}

static ssize_t Breathled_switch_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count )
{
	if (NULL == buf) {
		return -EINVAL;
	}

	if (0 == count) {
		return 0;
	}

	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return -EINVAL;
	}

	if( buf[0] == '0') {
		ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED, (BREATHLIGHT_PWM | BREATHLIGHT_ALON));
		g_the_chip[0]->breath_leds = 0;
	} else if ( buf[0] == '1') {
		ktd22xx_lowbattery_breath_leds();
		g_the_chip[0]->breath_leds = 1;
	} else if ( buf[0] == '2') {
		ktd22xx_events_breath_leds();
		g_the_chip[0]->breath_leds = 2;
	} else {
		ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED,BREATHLIGHT_PWM);
		ktd22xx_open_one_led(g_the_chip[0], BREATHLIGHT_LED,BREATHLIGHT_ALON);
		g_the_chip[0]->breath_leds = 255;
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
	int EdgeMS_reg = 0;
	int totalMS_reg =0;
	int fallMS_reg = 0;
	int riseMS_reg = 0;
	int onMS_reg = 0;
	//int offMS_reg = 0;
	//int Trise_on = 0;
	//int Tfall_off = 0;
	unsigned int id = 0;
	int led_num = 0;
	int i;
	u8 onBuf[10] = {0x04, 0x00, 0x06, g_the_chip[id]->color_D1, 0x07, g_the_chip[id]->color_D2, 0x08, g_the_chip[id]->color_D3, 0x04, 0x15};
	u8 breBuf[22] = {0x04, 0x00, 0x00, 0x00, 0x01, totalMS_reg, 0x02, onMS_reg, 0x05, EdgeMS_reg, 0x06, g_the_chip[id]->color_D1,
		0x07, g_the_chip[id]->color_D2, 0x08, g_the_chip[id]->color_D3, 0x00, 0x06, 0x00, 0x00, 0x04, 0x2A};
	u8 offBuf[8] = {0x04, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00};

	if (NULL == buf)
		return -EINVAL;

	if (0 == count)
		return 0;

	if (sscanf(buf, "%d,%d,%d,%d,%d,%d", &data[0],&data[1],&data[2],&data[3],&data[4],&data[5]) == 6) {
		totalMS = data[3] * 50;//freq = 100;
		onMS = 80 * totalMS / 255;//pwm = 80;
		KTD_ERR("data[0]=%d, data[1]=%d, data[2]=%d,data[3]=%d,data[4]=%d,data[5]=%d,\n", data[0],
			data[1],data[2],data[3],data[4],data[5]);

		if(data[4] < 2) {
			id = data[4];
			led_num = 1;
		} else {
			led_num = 2;
		}
		if (0 == data[5]) {
			sled_stop_crl_bre_timer();

			if(ktd2026_lv8_enable(true)) {
				KTD_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return count;
			}
			for (i = 0; i< led_num; i++) {
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
				onBuf[3] = g_the_chip[id]->color_D1;
				onBuf[5] = g_the_chip[id]->color_D2;
				onBuf[7] = g_the_chip[id]->color_D3;

				//g_the_chip[id]->value[BREATHLIGHT_LED] = BREATHLIGHT_CURRENT;
				//ktd22xx_close_one_led(g_the_chip[id], BREATHLIGHT_LED,BREATHLIGHT_PWM);
				//g_the_chip[id]->EnableReg &= ~EnableReg;
				//ktd22xx_i2c_write_byte(chip, 0x04, chip->EnableReg);
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x00, 0x07);
				msleep(2);
				ktd2026_i2c_write(g_the_chip[id]->client, onBuf, 5);
				/*ktd22xx_i2c_write_byte(g_the_chip[id], 0x00, 0x07);
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x04, 0x00);
				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x06, 0x00);//set LED1 current is 00 mA
				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x07, 0x00);//set LED1 current is 00 mA
				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x08, 0x00);//set LED1 current is 00 mA

				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x00, 0x20);// initialization LED off
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x06, g_the_chip[id]->color_D1);//set LED1 current  R
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x07, g_the_chip[id]->color_D2);//set LED2 current  G
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x08, g_the_chip[id]->color_D3);//set LED3 current  B
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x04, 0x15);*/
			}
		} else if (1 == data[5]){
			sled_stop_crl_bre_timer();
			if (33 == data[3]) {
				totalMS = 1000;
				riseMS = 300;
				onMS = 130;
				fallMS = 200;
				offMS = 230;
			} else if (22 == data[3]) {
				totalMS = 1250;
				riseMS = 500;
				onMS = 130;
				fallMS = 500;
				offMS = 100;
			} else if (44 == data[3]) {
				totalMS = 870;
				riseMS = 0;
				onMS = 300;
				fallMS = 300;
				offMS = 100;
			}
			totalMS_reg = timetoreg(totalMS,time_stable_buf,sizeof(time_stable_buf)/4); //tall
			riseMS_reg = timetoreg(riseMS,time_edge_buf,sizeof(time_edge_buf)/4);//trise and tfall
			fallMS_reg = timetoreg(fallMS,time_edge_buf,sizeof(time_edge_buf)/4);//tfall
			//offMS_reg =  timetoreg(offMS-10,time_stable_buf,sizeof(time_stable_buf)/4);//toff
			onMS_reg = (onMS +  time_edge_buf[riseMS_reg])*250/time_stable_buf[totalMS_reg];//ton

			EdgeMS_reg =  ((fallMS_reg << 4 ) | riseMS_reg) & 0xff;
			//Tfall_off = ((EdgeMS_reg << 4 ) | offMS_reg) & 0xff;
			KTD_ERR("g_chip->totalMS=%d, g_chip->onMS=%d, g_chip->offMS=%d,\n", g_chip->totalMS, g_chip->onMS, g_chip->offMS);
			if(ktd2026_lv8_enable(true)) {
				KTD_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
				return count;
			}
			for (i = 0; i< led_num; i++) {
				if (led_num >= DLED_NUM) {
					id = i;
				}
				g_chip->totalMS = totalMS;
				g_chip->onMS = time_stable_buf[totalMS_reg]*onMS_reg/250 + time_edge_buf[fallMS_reg];
				g_chip->offMS = totalMS - g_chip->onMS;
				KTD_ERR("g_chip->totalMS=%d, g_chip->onMS=%d, g_chip->offMS=%d,riseMS=%d,onMS=%d,fallMS=%d \n", g_chip->totalMS, g_chip->onMS,
					g_chip->offMS, riseMS, onMS, fallMS);
				KTD_ERR("totalMS_reg=%d, riseMS_reg=%d, fallMS_reg=%d,onMS_reg=%d\n", totalMS_reg, riseMS_reg, fallMS_reg,onMS_reg);
				KTD_ERR("riseMS=%d, fallMS=%d\n", time_edge_buf[riseMS_reg], time_edge_buf[fallMS_reg]);
				KTD_ERR("g_chip->totalMS=%d, g_chip->onMS=%d, g_chip->offMS=%d,\n", g_chip->totalMS, g_chip->onMS, g_chip->offMS);
				if (g_the_chip[id] == NULL) {
					continue;
				}
				g_the_chip[id]->color_D1 = data[0];
				g_the_chip[id]->color_D2 = data[1];
				g_the_chip[id]->color_D3 = data[2];
				g_the_chip[id]->freq = data[3];
				g_the_chip[id]->led_id = data[4];
				g_the_chip[id]->led_on = data[5];
				breBuf[5] = totalMS_reg;
				breBuf[7] = onMS_reg;
				breBuf[9] = EdgeMS_reg;
				breBuf[11] = g_the_chip[id]->color_D1;
				breBuf[13] = g_the_chip[id]->color_D2;
				breBuf[15] = g_the_chip[id]->color_D3;

				ktd2026_i2c_write(g_the_chip[id]->client, breBuf, 11);

				/*ktd22xx_i2c_write_byte(g_the_chip[id], 0x00, 0x06); //  timer reset
				//g_the_chip[id]->value[BREATHLIGHT_LED] = BREATHLIGHT_CURRENT;
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x00, 0x00);// initialization LED off

				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x05, EdgeMS_reg);//rase time
				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x05, 0x33);//rase time 384ms + 184ms
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x05, EdgeMS_reg);//rase time 384ms + 256ms

				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x01, totalMS_reg);//dry flash period
				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x01, 0x06);//dry flash period   1024ms
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x01, totalMS_reg);//dry flash period   1024ms

				ktd22xx_i2c_write_byte(g_the_chip[id], 0x02, 0x00);//reset internal counter stableMS_reg
				//ktd22xx_open_one_led(g_the_chip[id], BREATHLIGHT_LED, BREATHLIGHT_PWM);//allocate led1 to timer1
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x06, g_the_chip[id]->color_D1);//set LED1 current  R
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x07, g_the_chip[id]->color_D2);//set LED2 current  G
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x08, g_the_chip[id]->color_D3);//set LED3 current  B
				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x02, stableMS_reg);//led flashing(curerent ramp-up and down countinuously)
				//ktd22xx_i2c_write_byte(g_the_chip[id], 0x02, 0x5E);//led flashing(curerent ramp-up and down countinuously)  384/1024/0.4=94
				//td22xx_i2c_write_byte(g_the_chip[id], 0x02, 0x5E);//led flashing(curerent ramp-up and down countinuously)  384/1024/0.4=94
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x02, onMS_reg);//led flashing(curerent ramp-up and down countinuously)  384/1024/0.4=94
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x04, 0x2A);*/
				KTD_ERR("g_the_chip[%d] crl end.\n", i);
			}
			if (33 == data[3]) {
				sled_start_crl_bre_timer();
			} else {
				sled_stop_led_work();
			}

		} else {

			if(ktd2026_lv8_enable(true)) {
				KTD_ERR("set_breathlight_brightness Regulator enable vcc_i2c failed\n");
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
				g_the_chip[id]->freq = 0;
				g_the_chip[id]->led_id = 0;
				g_the_chip[id]->led_on = 2;
				ktd2026_i2c_write(g_the_chip[id]->client, offBuf, 4);
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x00, 0x07);
				/*ktd22xx_i2c_write_byte(g_the_chip[id], 0x04, 0x00);
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x06, g_the_chip[id]->color_D1);//set LED1 current is 00 mA
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x07, g_the_chip[id]->color_D2);//set LED1 current is 00 mA
				ktd22xx_i2c_write_byte(g_the_chip[id], 0x08, g_the_chip[id]->color_D3);*///set LED1 current is 00 mA


				//ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED, (BREATHLIGHT_PWM | BREATHLIGHT_ALON));
				//turn off led when 0x01 is  0x00,there is set to same as breath leds
				//i2c_smbus_write_byte_data(g_the_chip[id]->client, 0x07, 0x00);				  //set close all led
				//i2c_smbus_write_byte_data(g_the_chip[id]->client, 0x01, 0x00);	//set sleep/shutdown mode
				KTD_ERR("oppo_led_debug end \n");
			}

			//usleep_range(g_chip->onMS * 1000, g_chip->onMS * 1000);
			if(ktd2026_lv8_enable(false)) {
				KTD_ERR("set_breathlight_brightness Regulator disable vcc_i2c failed\n");
				return count;
			}
		}
	} else {
		KTD_ERR("set_breathlight_colour fail\n");
	}
	return count;
}

static ssize_t Breathled_switch_show2 ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return sprintf(buf, "%d\n", 0);
	}
	return sprintf ( buf, "value :%ld\n", g_the_chip[0]->breath_time);
}
static ssize_t Breathled_switch_store2 ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count )
{
	int ret = 0;

	if (NULL == buf) {
       		return -EINVAL;
    	}

	if (!g_the_chip[0]) {
		KTD_ERR("the chip null");
		return -EINVAL;
	}

	ret = kstrtoul(buf, 0, &g_the_chip[0]->breath_time);
	if (ret < 0) {
		KTD_ERR("kstrtoul failed, ret=%d\n", ret);
		return ret;
	}

	ktd22xx_breath_leds_time(g_the_chip[0],g_the_chip[0]->breath_time);

	return count;

}

static ssize_t sled_grppwm_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	if (!g_the_chip[0]) {
		 KTD_ERR("the chip null");
		 return count;
	}

	g_the_chip[0]->Ton_ms = value * g_the_chip[0]->Ttotal_ms / 255;

	if(g_the_chip[0]->Ton_ms > 9000) {
		g_the_chip[0]->Ton_ms = 9000;
	}

	return count;
}

static ssize_t sled_grpfreq_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	if(!g_the_chip[0]){
		 KTD_ERR("the chip null");
		 return count;
	}

	g_the_chip[0]->Ttotal_ms = value * 50;
	if (g_the_chip[0]->Ttotal_ms > 16000) {
		g_the_chip[0]->Ttotal_ms = 16000;
	}

    return count;
}

static ssize_t sled_blink_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	#ifndef CONFIG_MTK_PTLATFORM
	unsigned long value = simple_strtoul(buf, NULL, 10);
	int totalMS_reg = 0;
	int onMS_reg = 0;
	int stableMS_reg = 0;

	if (!g_the_chip[0]) {
		 KTD_ERR("the chip null");
		 return count;
	}

	if (~(~value) && g_the_chip[0]->Ttotal_ms && g_the_chip[0]->Ton_ms) {
		totalMS_reg = (g_the_chip[0]->Ttotal_ms - 256) / 128;
		onMS_reg = (g_the_chip[0]->Ton_ms / 3 / 192) & 0x0F;
		stableMS_reg = g_the_chip[0]->Ton_ms * 250 / 3 / g_the_chip[0]->Ttotal_ms * 2;
		onMS_reg = onMS_reg|(onMS_reg << 4);

		KTD_ERR("total_MS:%d,onMS:%d,totalMS_reg:%d,onMS_reg:%d,stableMS_reg:%x\n",
			g_the_chip[0]->Ttotal_ms, g_the_chip[0]->Ton_ms, totalMS_reg, onMS_reg, stableMS_reg);

		ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x20);// initialization LED off
		ktd22xx_i2c_write_byte(g_the_chip[0], 0x05, onMS_reg);//rase time
		ktd22xx_i2c_write_byte(g_the_chip[0], 0x01, totalMS_reg);//dry flash period
		ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, 0x00);//reset internal counter stableMS_reg
		ktd22xx_open_one_led(g_the_chip[0], BREATHLIGHT_LED, BREATHLIGHT_PWM);//allocate led1 to timer1
		ktd22xx_i2c_write_byte(g_the_chip[0], 0x02, stableMS_reg);//led flashing(curerent ramp-up and down countinuously)
	} else {
		if ((g_the_chip[0]->value[0] + g_the_chip[0]->value[2] + g_the_chip[0]->value[3]) == 0) {
	  		ktd2xx_led_off();
		} else {
			ktd22xx_i2c_write_byte(g_the_chip[0], 0x00, 0x20);// initialization LED off
			ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED, BREATHLIGHT_PWM);//free  timer1 from led2
			ktd22xx_open_one_led(g_the_chip[0], BREATHLIGHT_LED, BREATHLIGHT_ALON);
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
static DEVICE_ATTR(ledtest, S_IWUSR | S_IRUGO, NULL, sled_test);
static DEVICE_ATTR(ledreset, S_IWUSR | S_IRUGO, NULL, sled_reset);
static DEVICE_ATTR(showing, S_IWUSR | S_IRUGO, NULL, sled_showing);
static DEVICE_ATTR(charging, S_IWUSR | S_IRUGO, NULL, sled_charging);
static DEVICE_ATTR(grppwm, S_IWUSR | S_IRUGO, NULL, sled_grppwm_store);
static DEVICE_ATTR(grpfreq, S_IWUSR | S_IRUGO, NULL, sled_grpfreq_store);
static DEVICE_ATTR(color, S_IWUSR | S_IRUGO, set_breathlight_colour_show, set_breathlight_colour_store);
static DEVICE_ATTR(blink, S_IWUSR | S_IRUGO, NULL, sled_blink_store);

static struct attribute *blink_attributes[] = {
	&dev_attr_grppwm.attr,
	&dev_attr_grpfreq.attr,
	&dev_attr_blink.attr,
	&dev_attr_charging.attr,
	&dev_attr_showing.attr,
	&dev_attr_ledreset.attr,
	&dev_attr_ledtest.attr,
	&dev_attr_color.attr,
	NULL
};

static const struct attribute_group blink_attr_group = {
	.attrs = blink_attributes,
};


static struct attribute *leds_attributes[] = {
	&dev_attr_color.attr,
	NULL
};
static const struct attribute_group leds_attr_group = {
	.attrs = leds_attributes,
};


static ssize_t ktd20xx_read(struct file *file, char __user *buf, size_t size, loff_t * offset)
 {
	 return 0;
 }

static ssize_t ktd20xx_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{

    static unsigned char rec_data[2];
	memset(rec_data, 0, 2);

	if (copy_from_user(rec_data, buf, 1)) {
		return -EFAULT;
	}

	if (buf[0] == '0') {
		 ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED, (BREATHLIGHT_PWM | BREATHLIGHT_ALON));
		g_the_chip[0]->breath_leds = 0;
	} else if ( buf[0] == '1') {
		ktd22xx_lowbattery_breath_leds();
		g_the_chip[0]->breath_leds = 1;
	} else if ( buf[0] == '2') {
		ktd22xx_events_breath_leds();
		g_the_chip[0]->breath_leds = 2;
	} else {
		ktd22xx_close_one_led(g_the_chip[0], BREATHLIGHT_LED, BREATHLIGHT_PWM);
		ktd22xx_open_one_led(g_the_chip[0], BREATHLIGHT_LED, BREATHLIGHT_ALON);
		g_the_chip[0]->breath_leds = 255;
	}

    return 1;
}


static struct file_operations ktd20xx_fops = {
	.owner = THIS_MODULE,
	.read  = ktd20xx_read,
	.write = ktd20xx_write,
};

static int ktd20xx_probe(struct i2c_client *client, const struct i2c_device_id *id){

	int ret = -1;
	int i, tmp = 0;

	struct ktd2026_chip *chip = NULL;
	KTD_ERR("oppo_led_debug is probe start \n");
	KTD_MSG("call\n");


	chip = devm_kzalloc(&client->dev,
			sizeof(struct ktd2026_chip), GFP_KERNEL);

	if (!chip) {
		KTD_ERR(" devm_kzalloc() failed\n");
		return -ENOMEM;
	}
	load_num ++;
	chip->client = client;
	//chip->led_on_off = 255;
	chip->power_on_off = 0;
	chip->breath_leds = 0;
	chip->onMS = 0;
	chip->totalMS = 0;
	chip->offMS = 0;
	//chip->value = 0;
	chip->color_D1 = 0;
	chip->color_D2 = 0;
	chip->color_D3 = 0;
	chip->pcb_verison = -1;

	ret = of_property_read_u32(client->dev.of_node, "ktd2026,id", &tmp);
	if (!ret)
	{
		chip->id = tmp;   //id
	} else {
		chip->id = load_num -1;   //<0>;
	}

	chip->pcb_verison = get_PCB_Version();
	KTD_ERR("the pcb_verison is %d.\n", chip->pcb_verison);

	chip->vcc_1v8 = regulator_get(&client->dev, "vdd_1v8");
	if( IS_ERR(chip->vcc_1v8) ){
		ret = PTR_ERR(chip->vcc_1v8);
		KTD_ERR("Regulator get ss failed vcc_i2c rc=%d\n", ret);
	}
	if (chip->pcb_verison <= HW_VERSION__12) {
		if (!IS_ERR(chip->vcc_1v8)){
			if(regulator_count_voltages(chip->vcc_1v8) > 0) {
				ret = regulator_set_voltage(chip->vcc_1v8, 1800000,1800000);
				if (ret) {
					KTD_ERR("Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
				} else {
					ret = regulator_enable(chip->vcc_1v8);
					if(ret) {
						KTD_ERR("Regulator vcc_i2c enable failed rc=%d\n", ret);
					}
				}
			}
		}
	}
	/* create i2c struct, when receve and transmit some byte */
	KTD_ERR("oppo_led_debug is probe start1 \n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		KTD_ERR("i2c smbus byte data unsupported\n");
		if (chip->pcb_verison <= HW_VERSION__12) {
			ret = regulator_disable(chip->vcc_1v8);
			if(ret) {
				KTD_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
			}
		}
		devm_kfree(&client->dev,chip);
		return -EOPNOTSUPP;
	}
	if (ktd22xx_reset(chip) != 0) {
		KTD_ERR("reset failed\n");
		if (chip->pcb_verison <= HW_VERSION__12) {
			ret = regulator_disable(chip->vcc_1v8);
			if(ret) {
				KTD_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
			}
		}
		devm_kfree(&client->dev,chip);
		return -EFAULT;
	}

	if (chip->pcb_verison <= HW_VERSION__12) {
		ret = regulator_disable(chip->vcc_1v8);
		if(ret) {
			KTD_ERR("Regulator vcc_i2c disable failed rc=%d\n", ret);
		}
	}
	g_chip = chip;

	/*************Added by Tong.han for same interface with 14037*******************/
	KTD_ERR("oppo_led_debug is probe start3 \n");
	if(load_num == 1) {
		for(i = 0; i < 1; i ++ ) {
			if (led_classdev_register(&client->dev, &KTD2026_lcds[i])) {
				KTD_ERR("led_classdev_register failed!\n");
			}
		}
	}
	ret = sysfs_create_group(&client->dev.kobj, &blink_attr_group);
	if (ret) {
		KTD_ERR( "sysfs_create_group failed!\n");
		goto err_group_register;
	}
	KTD_ERR("oppo_led_debug is probe start4 \n");

	if(load_num == 1) {
		KTD_ERR("oppo_led_debug is probe start5 \n");
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

	chip->major = register_chrdev(0, "ktd20xx", &ktd20xx_fops);
	if(load_num == 1) {
		chip->ktd20xx_cls = class_create(THIS_MODULE, "ktd20xx");

		if (IS_ERR(chip->ktd20xx_cls)) {
			KTD_ERR("perfmap: Error in class_create ktd20xx_cls\n");
			goto err_class_create;

			chip->ktd22xx_dev = device_create(chip->ktd20xx_cls, NULL, MKDEV(chip->major, 0), NULL, "ktd2026"); /* /dev/ktd20xx */

			if (chip->ktd22xx_dev) {
				if (device_create_file(chip->ktd22xx_dev, &dev_attr_led) < 0)
					KTD_ERR("Failed to create device file(%s)!\n", dev_attr_led.attr.name);
				if (device_create_file(chip->ktd22xx_dev, &dev_attr_breath_led) < 0)
					KTD_ERR("Failed to create device file(%s)!\n", dev_attr_led.attr.name);
			}
		}
	}

	KTD_ERR("oppo_led_debug is probe start6 \n");
	if (chip->id < DLED_NUM) {
		g_the_chip[chip->id] = chip;
		KTD_ERR("ktd20xx id[%d], client addr [0x%02x]\n",chip->id, client->addr);
	} else {
		KTD_ERR("ktd20xx id[%d] >= DLED_NUM[%d],fail!\n",chip->id, DLED_NUM);
		goto err_class_create;
	}
//	g_the_chip = chip;

//	for (i = 0; i < 3; i ++ ) {
//		g_the_chip->value[i] = 0;
//	}


	KTD_MSG("ok\n");

	return 0;

err_class_create:
	//unregister_chrdev(chip->major, "ktd20xx");
	class_destroy(chip->ktd20xx_cls);
err_group_register:
	for(i = 0; i < 1; i ++ )
	//for(i = 0; i < sizeof(KTD2026_lcds)/sizeof(struct led_classdev); i ++ ) {
		led_classdev_unregister(&KTD2026_lcds[i]);
	//}

	devm_kfree(&client->dev,chip);
	//g_the_chip = NULL;

	return ret;

}
static void ktd20xx_shutdown(struct i2c_client *client)
{
	KTD_MSG("call\n");
	//ktd2xx_led_off();
}

static int ktd20xx_remove(struct i2c_client *client){

	//int i = 0;

	KTD_MSG("call \n");

	//if (g_the_chip) {
	//	unregister_chrdev(g_the_chip->major, "ktd20xx");

	//	for(i = 0; i < sizeof(KTD2026_lcds) / sizeof(struct led_classdev); i ++) {
	//		led_classdev_unregister(&KTD2026_lcds[i]);
	//	}

	//	class_destroy(g_the_chip->ktd20xx_cls);
	//}

	return 0;
}

static const struct i2c_device_id ktd2xx_id[] = {
	{KTD_I2C_NAME, 0},
	{ }
};

static struct of_device_id ktd20xx_match_table[] = {
	{.compatible = "oppo,ktd2026"},
	{},
};

static struct i2c_driver ktd20xx_driver = {
	.driver = {
		.name  = KTD_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ktd20xx_match_table,
	},
	.probe = ktd20xx_probe,
	.remove = ktd20xx_remove,
	.shutdown = ktd20xx_shutdown,
	.id_table = ktd2xx_id,
};

static int __init ktd20xx_init(void)
{
	int err;

	printk("%s\n",__func__);
	err = i2c_add_driver(&ktd20xx_driver);
	if (err) {
		printk(KERN_WARNING "ktd20xx driver failed "
				"(errno = %d)\n", err);
	} else {
		printk( " added driver is end %s\n",
				ktd20xx_driver.driver.name);
	}
	return err;
}

static void __exit ktd20xx_exit(void)
{
	KTD_MSG("call \n");
	i2c_del_driver(&ktd20xx_driver);
}

module_init(ktd20xx_init);
module_exit(ktd20xx_exit);

MODULE_LICENSE("GPL");
