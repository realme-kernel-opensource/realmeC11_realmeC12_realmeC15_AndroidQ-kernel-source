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

#include "oppo_ktd2026b.h"

#define KTD_I2C_NAME "ktd2026b"

#define DEVINFO_NAME KTD_I2C_NAME

#define log_fmt(fmt) "[line:%d][module:%s][%s] " fmt

#define KTD_ERR(a,arg...)\
		printk(KERN_NOTICE log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

#define KTD_MSG(a, arg...) \
		printk(KERN_INFO log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg);

static struct ktd2026_chip *g_the_chip = NULL;

void ktd22xxb_events_breath_leds(void);

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
	        .name		= "button-backlight-b",
		.brightness 	= MAX_BACKLIGHT_BRIGHTNESS,
	        .brightness_set = set_button_backlight_brightness,
	},
	#endif
	#if CONFIG_SUPPORT_BREATHLIGHT
	{
	        .name		= "white-b",
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

	//ktd22xx_i2c_write_byte(g_the_chip, 0x00, 0x07);//reset reg
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

void ktd2xxb_led_off(void)
{
	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}

	/* turn on led when 0x02 is not 0x00,
	     there is set to same as breath leds  */
	ktd22xx_i2c_write_byte(g_the_chip, 0x06, 0x00);//set current is 0.125mA
	ktd22xx_i2c_write_byte(g_the_chip, 0x07, 0x00);//set current is 0.125mA
	ktd22xx_i2c_write_byte(g_the_chip, 0x08, 0x00);//set current is 0.125mA
	ktd22xx_i2c_write_byte(g_the_chip, 0x04, 0x00);//turn off leds
	ktd22xx_i2c_write_byte(g_the_chip, 0x00, 0x28);//set sleep mode
}

EXPORT_SYMBOL(ktd2xxb_led_off);

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

void ktd22xxb_breath_leds_time(struct ktd2026_chip *chip, int blink)
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

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}

	if (value == 0) {
		ktd22xx_close_one_led(g_the_chip,BUTTON_BACKLIGHT_LED, BUTTON_BACKLIGHT_ALON);
		g_the_chip->value[BUTTON_BACKLIGHT_LED] = 0;
	} else {
		g_the_chip->value[BUTTON_BACKLIGHT_LED] = OUTPUT_10AM;
		ktd22xx_open_one_led(g_the_chip,BUTTON_BACKLIGHT_LED, BUTTON_BACKLIGHT_ALON);
	}

}
#endif /* CONFIG_SUPPORT_BUTTON_BACKLIGHT  */

#if CONFIG_SUPPORT_BREATHLIGHT
static void set_breathlight_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	#ifndef CONFIG_MTK_PTLATFORM
	KTD_MSG("ktd20xx name:%s value:%d \n",led_cdev->name, value);

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}

	if (value > MAX_BACKLIGHT_BRIGHTNESS) {
		value = MAX_BACKLIGHT_BRIGHTNESS;
	}

	if (!strcmp(led_cdev->name, "white-b")) {
		g_the_chip->value[BREATHLIGHT_LED] = value;
	} else {
		g_the_chip->value[BREATHLIGHT_LED] = 0;
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

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}

	KTD_MSG("ktd20xx name:%s value:%d \n",led_cdev->name, value);

	if (!strcmp(led_cdev->name, "white-b")) {
		if( value == 0) {//close
			g_the_chip->value[BREATHLIGHT_LED] = 0;
			ktd22xx_close_one_led(g_the_chip, BREATHLIGHT_LED, (BREATHLIGHT_PWM | BREATHLIGHT_ALON));
		} else if (value == 1) {//blink
			g_the_chip->value[BREATHLIGHT_LED] = BREATHLIGHT_CURRENT;
			ktd22xx_i2c_write_byte(g_the_chip, 0x00, 0x20);// initialization LED off
			ktd22xx_i2c_write_byte(g_the_chip, 0x05, onMS_reg);//rase time
			ktd22xx_i2c_write_byte(g_the_chip, 0x01, totalMS_reg);//dry flash period
			ktd22xx_i2c_write_byte(g_the_chip, 0x02, 0x00);//reset internal counter stableMS_reg
			ktd22xx_open_one_led(g_the_chip, BREATHLIGHT_LED, BREATHLIGHT_PWM);//allocate led1 to timer1
			ktd22xx_i2c_write_byte(g_the_chip, 0x02, stableMS_reg);//led flashing(curerent ramp-up and down countinuously)
		} else {//always on
			g_the_chip->value[BREATHLIGHT_LED] = BREATHLIGHT_CURRENT;
			ktd22xx_close_one_led(g_the_chip, BREATHLIGHT_LED,BREATHLIGHT_PWM);
			ktd22xx_open_one_led(g_the_chip, BREATHLIGHT_LED,BREATHLIGHT_ALON);
		}
	} else {
		g_the_chip->value[BREATHLIGHT_LED] = 0;
	}
	#endif

}

#endif /* CONFIG_SUPPORT_BREATHLIGHT */

void ktd22xxb_lowbattery_breath_leds(void)
{
	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}
	/*
	 * flash time period: 2.5s, rise/fall 1s, sleep 0.5s
	 * reg5 = 0xaa, reg1 = 0x12
	 */
	ktd22xx_i2c_write_byte(g_the_chip, 0x05, 0xaa);//rase time
	ktd22xx_i2c_write_byte(g_the_chip, 0x01, 0x12);//dry flash period
	ktd22xx_i2c_write_byte(g_the_chip, 0x02, 0x00);//reset internal counter
	ktd22xx_open_one_led(g_the_chip,BREATHLIGHT_LED,BREATHLIGHT_PWM);//allocate led1 to timer1
	ktd22xx_i2c_write_byte(g_the_chip, 0x02, 0x56);//led flashing(curerent ramp-up and down countinuously)

}

void ktd22xxb_events_breath_leds(void)
{
	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return;
	}
	/*
	 * flash time period: 5s, rise/fall 2s, sleep 1s
	 * reg5 = 0xaa, reg1 = 0x30
	 */
	ktd22xx_i2c_write_byte(g_the_chip, 0x05, 0xaa);//rase time
	ktd22xx_i2c_write_byte(g_the_chip, 0x01, 0x30);//dry flash period
	ktd22xx_i2c_write_byte(g_the_chip, 0x02, 0x00);//reset internal counter
	ktd22xx_open_one_led(g_the_chip, BREATHLIGHT_LED, BREATHLIGHT_PWM);//allocate led1 to timer1
	ktd22xx_i2c_write_byte(g_the_chip, 0x02, 0x56);//led flashing(curerent ramp-up and down countinuously)
}

static ssize_t Breathled_switch_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return sprintf(buf, "%d\n", 0);
	}
	return sprintf ( buf, "%d\n", g_the_chip->breath_leds);
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

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return -EINVAL;
	}

	if( buf[0] == '0') {
		ktd22xx_close_one_led(g_the_chip, BREATHLIGHT_LED, (BREATHLIGHT_PWM | BREATHLIGHT_ALON));
		g_the_chip->breath_leds = 0;
	} else if ( buf[0] == '1') {
		ktd22xxb_lowbattery_breath_leds();
		g_the_chip->breath_leds = 1;
	} else if ( buf[0] == '2') {
		ktd22xxb_events_breath_leds();
		g_the_chip->breath_leds = 2;
	} else {
		ktd22xx_close_one_led(g_the_chip, BREATHLIGHT_LED,BREATHLIGHT_PWM);
		ktd22xx_open_one_led(g_the_chip, BREATHLIGHT_LED,BREATHLIGHT_ALON);
		g_the_chip->breath_leds = 255;
	}

	return count;

}

static ssize_t Breathled_switch_show2 ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return sprintf(buf, "%d\n", 0);
	}
	return sprintf ( buf, "value :%ld\n", g_the_chip->breath_time);
}
static ssize_t Breathled_switch_store2 ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count )
{
	int ret = 0;

	if (NULL == buf) {
       		return -EINVAL;
    	}

	if (!g_the_chip) {
		KTD_ERR("the chip null");
		return -EINVAL;
	}

	ret = kstrtoul(buf, 0, &g_the_chip->breath_time);
	if (ret < 0) {
		KTD_ERR("kstrtoul failed, ret=%d\n", ret);
		return ret;
	}

	ktd22xxb_breath_leds_time(g_the_chip,g_the_chip->breath_time);

	return count;

}

static ssize_t sled_grppwm_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	if (!g_the_chip) {
		 KTD_ERR("the chip null");
		 return count;
	}

	g_the_chip->Ton_ms = value * g_the_chip->Ttotal_ms / 255;

	if(g_the_chip->Ton_ms > 9000) {
		g_the_chip->Ton_ms = 9000;
	}

	return count;
}

static ssize_t sled_grpfreq_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	if(!g_the_chip){
		 KTD_ERR("the chip null");
		 return count;
	}

	g_the_chip->Ttotal_ms = value * 50;
	if (g_the_chip->Ttotal_ms > 16000) {
        	g_the_chip->Ttotal_ms = 16000;
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

	if (!g_the_chip) {
		 KTD_ERR("the chip null");
		 return count;
	}

	if (~(~value) && g_the_chip->Ttotal_ms && g_the_chip->Ton_ms) {
		totalMS_reg = (g_the_chip->Ttotal_ms - 256) / 128;
		onMS_reg = (g_the_chip->Ton_ms / 3 / 192) & 0x0F;
		stableMS_reg = g_the_chip->Ton_ms * 250 / 3 / g_the_chip->Ttotal_ms * 2;
		onMS_reg = onMS_reg|(onMS_reg << 4);

		KTD_ERR("total_MS:%d,onMS:%d,totalMS_reg:%d,onMS_reg:%d,stableMS_reg:%x\n",
			g_the_chip->Ttotal_ms, g_the_chip->Ton_ms, totalMS_reg, onMS_reg, stableMS_reg);

		ktd22xx_i2c_write_byte(g_the_chip, 0x00, 0x20);// initialization LED off
		ktd22xx_i2c_write_byte(g_the_chip, 0x05, onMS_reg);//rase time
		ktd22xx_i2c_write_byte(g_the_chip, 0x01, totalMS_reg);//dry flash period
		ktd22xx_i2c_write_byte(g_the_chip, 0x02, 0x00);//reset internal counter stableMS_reg
		ktd22xx_open_one_led(g_the_chip, BREATHLIGHT_LED, BREATHLIGHT_PWM);//allocate led1 to timer1
		ktd22xx_i2c_write_byte(g_the_chip, 0x02, stableMS_reg);//led flashing(curerent ramp-up and down countinuously)
	} else {
		if ((g_the_chip->value[0] + g_the_chip->value[2] + g_the_chip->value[3]) == 0) {
	  		ktd2xxb_led_off();
		} else {
			ktd22xx_i2c_write_byte(g_the_chip, 0x00, 0x20);// initialization LED off
			ktd22xx_close_one_led(g_the_chip, BREATHLIGHT_LED, BREATHLIGHT_PWM);//free  timer1 from led2
			ktd22xx_open_one_led(g_the_chip, BREATHLIGHT_LED, BREATHLIGHT_ALON);
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
static DEVICE_ATTR(blink, S_IWUSR | S_IRUGO, NULL, sled_blink_store);

static struct attribute *blink_attributes[] = {
	&dev_attr_grppwm.attr,
	&dev_attr_grpfreq.attr,
	&dev_attr_blink.attr,
	&dev_attr_charging.attr,
	&dev_attr_showing.attr,
	&dev_attr_ledreset.attr,
	&dev_attr_ledtest.attr,
	NULL
};

static const struct attribute_group blink_attr_group = {
	.attrs = blink_attributes,
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
		 ktd22xx_close_one_led(g_the_chip, BREATHLIGHT_LED, (BREATHLIGHT_PWM | BREATHLIGHT_ALON));
		g_the_chip->breath_leds = 0;
	} else if ( buf[0] == '1') {
		ktd22xxb_lowbattery_breath_leds();
		g_the_chip->breath_leds = 1;
	} else if ( buf[0] == '2') {
		ktd22xxb_events_breath_leds();
		g_the_chip->breath_leds = 2;
	} else {
		ktd22xx_close_one_led(g_the_chip, BREATHLIGHT_LED, BREATHLIGHT_PWM);
		ktd22xx_open_one_led(g_the_chip, BREATHLIGHT_LED, BREATHLIGHT_ALON);
		g_the_chip->breath_leds = 255;
	}

    return 1;
}


static struct file_operations ktd20xx_fops = {
	.owner = THIS_MODULE,
	.read  = ktd20xx_read,
	.write = ktd20xx_write,
};

static int ktd20xx_probe(struct i2c_client *client, const struct i2c_device_id *id){

	int ret = 0;
	int i = 0;

	struct ktd2026_chip *chip = NULL;

	KTD_MSG("call\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		KTD_ERR("i2c smbus byte data unsupported\n");
		return -EOPNOTSUPP;
	}

	chip = devm_kzalloc(&client->dev,
			sizeof(struct ktd2026_chip), GFP_KERNEL);

	if (!chip) {
		KTD_ERR(" devm_kzalloc() failed\n");
		return -ENOMEM;
	}

	chip->client = client;

	if (ktd22xx_reset(chip) != 0) {
		KTD_ERR("reset failed\n");
		devm_kfree(&client->dev,chip);
		return -EFAULT;
	}

	for (i = 0; i < sizeof(KTD2026_lcds) / sizeof(struct led_classdev); i ++ ) {
		if (led_classdev_register(&client->dev, &KTD2026_lcds[i])) {
			KTD_ERR("led_classdev_register failed!\n");
		}
	}

	ret = sysfs_create_group(&client->dev.kobj, &blink_attr_group);
	if (ret) {
		KTD_ERR( "sysfs_create_group failed!\n");
		goto err_group_register;
	}

	chip->major = register_chrdev(0, "ktd20xx", &ktd20xx_fops);

	chip->ktd20xx_cls = class_create(THIS_MODULE, "ktd20xx");

	if (IS_ERR(chip->ktd20xx_cls)) {
		KTD_ERR("perfmap: Error in class_create ktd20xx_cls\n");
		goto err_class_create;
	}

	chip->ktd22xx_dev = device_create(chip->ktd20xx_cls, NULL, MKDEV(chip->major, 0), NULL, "ktd2026"); /* /dev/ktd20xx */

	if (chip->ktd22xx_dev) {
		device_create_file(chip->ktd22xx_dev, &dev_attr_led);
		device_create_file(chip->ktd22xx_dev, &dev_attr_breath_led);
	}

	g_the_chip = chip;

	for (i = 0; i < 3; i ++ ) {
		g_the_chip->value[i] = 0;
	}


	KTD_MSG("ok\n");

	return 0;

err_class_create:
	unregister_chrdev(chip->major, "ktd20xx");
err_group_register:

	for(i = 0; i < sizeof(KTD2026_lcds)/sizeof(struct led_classdev); i ++ ) {
		led_classdev_unregister(&KTD2026_lcds[i]);
	}

	devm_kfree(&client->dev,chip);
	g_the_chip = NULL;

	return ret;

}
static void ktd20xx_shutdown(struct i2c_client *client)
{
	KTD_MSG("call\n");
	ktd2xxb_led_off();
}

static int ktd20xx_remove(struct i2c_client *client){

	int i = 0;

	KTD_MSG("call \n");

	if (g_the_chip) {
		unregister_chrdev(g_the_chip->major, "ktd20xx");

		for(i = 0; i < sizeof(KTD2026_lcds) / sizeof(struct led_classdev); i ++) {
			led_classdev_unregister(&KTD2026_lcds[i]);
		}

		class_destroy(g_the_chip->ktd20xx_cls);
	}

	return 0;
}

static const struct i2c_device_id ktd2xx_id[] = {
	{KTD_I2C_NAME, 0},
	{ }
};

static struct of_device_id ktd20xx_match_table[] = {
	{.compatible = "oppo,ktd2026b"},
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

static int __init ktd20xxb_init(void)
{
	KTD_MSG("call \n");

	i2c_add_driver(&ktd20xx_driver);

	return 0;
}

static void __exit ktd20xxb_exit(void)
{
	KTD_MSG("call \n");
	i2c_del_driver(&ktd20xx_driver);
}

module_init(ktd20xxb_init);
module_exit(ktd20xxb_exit);

MODULE_LICENSE("GPL");
