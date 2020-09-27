/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": %s: " fmt, __func__

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>

#include "flashlight-core.h"
#include "flashlight-dt.h"
#include <mt-plat/mtk_pwm.h>

#ifdef CONFIG_PM_WAKELOCKS
#include <linux/pm_wakeup.h>
#endif

/* define device tree */
#ifndef PASCAL_DTNAME
#define PASCAL_DTNAME "mediatek,flashlights_pascal"
#endif

#define PASCAL_NAME "flashlights_pascal"

#ifndef FLASHLIGHT_BRIGHTNESS_ADD
#define FLASHLIGHT_BRIGHTNESS_ADD
#endif
static int is_decrease_voltage = 0;

/* Shipei.Chen@Cam.Drv, 20200606, modify the way for log print!*/
#define PR_INFO_IF(cond, ...)      do { if ( (cond) ) { pr_info(__VA_ARGS__); } }while(0)
#define PR_DEBUG_IF(cond, ...)     do { if ( (cond) ) { pr_debug(__VA_ARGS__); } }while(0)
#define PR_ERR_IF(cond, ...)       do { if ( (cond) ) { pr_err(__VA_ARGS__); } }while(0)
static bool dumpEnable = 0;

/* define registers */

/* define mutex and work queue */
#ifdef ODM_WT_EDIT
//Sidong.Zhao@WT.CHG,2020/6/29,add for disable highvolt charger when flahlights on
static DEFINE_MUTEX(fl_hv_mutex);
#endif /* ODM_WT_EDIT */
static DEFINE_MUTEX(pascal_mutex);
static struct work_struct pascal_work;
//static struct pwm_spec_config pwm_setting;

/* Shipei.Chen@Cam.Drv, 20200515,  modify for torch to gpio driver!*/
#define PASCAL_LED_MODE_DUTY 26
#define PASCAL_TORCH_MODE_DUTY 27

/*wenhui.chen@Camera.Driver, 20200609, modify for flashlight current*/
#define PASCAL_LEVEL_NUM 24
#define PASCAL_LEVEL_TORCH 1
static int g_duty_array[PASCAL_LEVEL_NUM] = {8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100};

/*wenhui.chen@Camera.Driver, 20200423, add for flashlight calibration*/
static const int *pascal_current;
static const int sgm3785_current[PASCAL_LEVEL_NUM] = {
        80,120,160,200,240,280,320,360,400,440,480,520,560,600,640,680,720,760,
        800,840,880,920,960,1000
};

/* define pinctrl */
#define PASCAL_PINCTRL_PIN_FLASH_EN 0
#define PASCAL_PINCTRL_PIN_PWM_EN 1
#define PASCAL_PINCTRL_PIN_PWM_GPIO 2
#define PASCAL_PINCTRL_PIN_STATE_LOW 0
#define PASCAL_PINCTRL_PIN_STATE_HIGH 1
#define PASCAL_PINCTRL_STATE_FLASH_EN_HIGH "flash_light_en_pin_1"
#define PASCAL_PINCTRL_STATE_FLASH_EN_LOW  "flash_light_en_pin_0"
#define PASCAL_PINCTRL_STATE_PWM_GPIO_HIGH "flash_light_flash_pin_1"
#define PASCAL_PINCTRL_STATE_PWM_GPIO_LOW  "flash_light_flash_pin_0"
#define PASCAL_PINCTRL_STATE_PWM "flash_light_pwm_pin"

static struct pinctrl *pascal_pinctrl;
static struct pinctrl_state *pascal_flash_en_high;
static struct pinctrl_state *pascal_flash_en_low;
static struct pinctrl_state *pascal_pwm_gpio_high;
static struct pinctrl_state *pascal_pwm_gpio_low;
static struct pinctrl_state *pascal_flash_pwm;

/* define usage count */
static int use_count;

static int g_flash_duty = -1;

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source flashlight_wake_lock;
#endif

/* platform data */
struct pascal_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int pascal_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	pascal_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(pascal_pinctrl)) {
		PR_ERR_IF(dumpEnable, "Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(pascal_pinctrl);
	}

	/*  Flashlight pin initialization */
	pascal_flash_en_high = pinctrl_lookup_state(
			pascal_pinctrl, PASCAL_PINCTRL_STATE_FLASH_EN_HIGH);
	if (IS_ERR(pascal_flash_en_high)) {
		PR_ERR_IF(dumpEnable, "Failed to init (%s)\n", PASCAL_PINCTRL_STATE_FLASH_EN_HIGH);
		ret = PTR_ERR(pascal_flash_en_high);
	}
	pascal_flash_en_low = pinctrl_lookup_state(
			pascal_pinctrl, PASCAL_PINCTRL_STATE_FLASH_EN_LOW);
	if (IS_ERR(pascal_flash_en_low)) {
		PR_ERR_IF(dumpEnable, "Failed to init (%s)\n", PASCAL_PINCTRL_STATE_FLASH_EN_LOW);
		ret = PTR_ERR(pascal_flash_en_low);
	}
	pascal_pwm_gpio_high = pinctrl_lookup_state(
			pascal_pinctrl, PASCAL_PINCTRL_STATE_PWM_GPIO_HIGH);
	if (IS_ERR(pascal_pwm_gpio_high)) {
		PR_ERR_IF(dumpEnable, "Failed to init (%s)\n", PASCAL_PINCTRL_STATE_PWM_GPIO_HIGH);
		ret = PTR_ERR(pascal_pwm_gpio_high);
	}
	pascal_pwm_gpio_low = pinctrl_lookup_state(
			pascal_pinctrl, PASCAL_PINCTRL_STATE_PWM_GPIO_LOW);
	if (IS_ERR(pascal_pwm_gpio_low)) {
		PR_ERR_IF(dumpEnable, "Failed to init (%s)\n", PASCAL_PINCTRL_STATE_PWM_GPIO_LOW);
		ret = PTR_ERR(pascal_pwm_gpio_low);
	}
	pascal_flash_pwm = pinctrl_lookup_state(
			pascal_pinctrl, PASCAL_PINCTRL_STATE_PWM);
	if (IS_ERR(pascal_flash_pwm)) {
		PR_ERR_IF(dumpEnable, "Failed to init (%s)\n", PASCAL_PINCTRL_STATE_PWM);
		ret = PTR_ERR(pascal_flash_pwm);
	}

	return ret;
}

static int pascal_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(pascal_pinctrl)) {
		PR_ERR_IF(dumpEnable, "pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
		case PASCAL_PINCTRL_PIN_FLASH_EN:
			if (state == PASCAL_PINCTRL_PIN_STATE_LOW &&!IS_ERR(pascal_flash_en_low))
				ret = pinctrl_select_state(pascal_pinctrl, pascal_flash_en_low);
			else if (state == PASCAL_PINCTRL_PIN_STATE_HIGH &&!IS_ERR(pascal_flash_en_high))
				ret = pinctrl_select_state(pascal_pinctrl, pascal_flash_en_high);
			else
				PR_ERR_IF(dumpEnable, "set err, pin(%d) state(%d)\n", pin, state);
			break;
		case PASCAL_PINCTRL_PIN_PWM_EN:
			ret =pinctrl_select_state(pascal_pinctrl, pascal_flash_pwm);
			break;
		case PASCAL_PINCTRL_PIN_PWM_GPIO:
			if (state == PASCAL_PINCTRL_PIN_STATE_LOW &&!IS_ERR(pascal_pwm_gpio_low))
				ret = pinctrl_select_state(pascal_pinctrl, pascal_pwm_gpio_low);
			else if (state == PASCAL_PINCTRL_PIN_STATE_HIGH &&!IS_ERR(pascal_pwm_gpio_high))
				ret = pinctrl_select_state(pascal_pinctrl, pascal_pwm_gpio_high);
			else
				PR_ERR_IF(dumpEnable, "set err, pin(%d) state(%d)\n", pin, state);
			break;
		default:
			PR_ERR_IF(dumpEnable, "set err, pin(%d) state(%d)\n", pin, state);
			break;
	}
	PR_DEBUG_IF(dumpEnable, "pin(%d) state(%d), ret:%d\n", pin, state, ret);

	return ret;
}


/* flashlight enable  pwm function */
/* 52M/32/100 = 16KHZ  actually 21KHZ for SGM3785*/
/*wenhui.chen@Camera.Driver, 20200426, modify for pwm freq*/
int mt_flashlight_led_set_pwm(int pwm_num,u32 level )
{
	struct pwm_spec_config pwm_setting;
	memset(&pwm_setting, 0, sizeof(struct pwm_spec_config));
	pwm_setting.pwm_no = pwm_num; /* PWM0 set 0,PWM1 set 1,PWM2 set 2,PWM3 set 3 */
	pwm_setting.mode = PWM_MODE_OLD;
	pwm_setting.pmic_pad = 0;
	pwm_setting.clk_div = CLK_DIV32;
	pwm_setting.clk_src = PWM_CLK_OLD_MODE_BLOCK;
	pwm_setting.PWM_MODE_OLD_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_OLD_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_OLD_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
	pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH = 100;
	pwm_setting.PWM_MODE_OLD_REGS.THRESH = level;

	pwm_set_spec_config(&pwm_setting);

	return 0;
}



/******************************************************************************
 * pascal operations
 *****************************************************************************/
/*wenhui.chen@Camera.Driver, 20200423, add for flashlight calibration*/
static int pascal_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= PASCAL_LEVEL_NUM)
		level = PASCAL_LEVEL_NUM - 1;

	return level;
}

/* flashlight enable function */
/*wenhui.chen@Camera.Driver, 20200514, modify for torch*/
#ifdef FLASHLIGHT_BRIGHTNESS_ADD
bool fl_state=false;
#endif

static int pascal_enable(void)
{
	int tempPWM = 0;

#ifdef CONFIG_PM_WAKELOCKS
	__pm_stay_awake(&flashlight_wake_lock);
#endif

#ifdef FLASHLIGHT_BRIGHTNESS_ADD
	if (fl_state){
		g_flash_duty=25;
		printk("fl_state=true\n");
	}
#endif

	//g_flash_duty = 25;                                                               //for test
	/* wenhui.Chen@Cam.Drv, 20200714,  modify for torch current */
	if (g_flash_duty == PASCAL_LED_MODE_DUTY || g_flash_duty == PASCAL_TORCH_MODE_DUTY) {//led mode
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN,0);
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_GPIO, 1);
		mdelay(6);                                          //delay more than 5ms
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_EN,1);    //set pwm mode
		mt_flashlight_led_set_pwm(3,80);                    //torch pwm 40%
	} else if (g_flash_duty < PASCAL_LEVEL_TORCH) {//flash mode
		//pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN,0);
		//pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_GPIO,1);//torch max
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN,0);
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_GPIO, 1);
		mdelay(6);                                          //delay more than 5ms
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_EN,1);    //set pwm mode
		mt_flashlight_led_set_pwm(3,40);                    //torch pwm 40%
	} else {//flash mode

		if (g_flash_duty < PASCAL_LEVEL_NUM)
			tempPWM = g_duty_array[g_flash_duty];
		else
			tempPWM = g_duty_array[PASCAL_LEVEL_NUM - 1];   //flash mode max

		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN,0);
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_EN,1);
		mt_flashlight_led_set_pwm(3,tempPWM);               //flash pwm tempPWM%
		udelay(500);
		pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN,1);
	}

		PR_DEBUG_IF(dumpEnable, "PASCAL Flash %s g_flash_duty = %d,%d\n",__FUNCTION__,g_flash_duty,tempPWM);

	return 0;
}

/* flashlight disable function */
static int pascal_disable(void)
{
	int state = PASCAL_PINCTRL_PIN_STATE_LOW;

	pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN, state);
	pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_GPIO, state);

#ifdef CONFIG_PM_WAKELOCKS
	__pm_relax(&flashlight_wake_lock);
#endif
	return 0;
}

#ifdef FLASHLIGHT_BRIGHTNESS_ADD
int pascal_enable_flash(int level)
{
	if (level==0) {
		fl_state=false;
		printk("flash mode close\n");
	}
	else {
		fl_state=true;//flash 1A
		printk("flash mode open\n");
	}
	return 0;
}
EXPORT_SYMBOL(pascal_enable_flash);
#endif

/* set flashlight level */
static int pascal_set_level(int level)
{
	g_flash_duty = level;
	return 0;
}
extern void oppo_chg_set_camera_status(bool val);

#ifdef ODM_WT_EDIT
//Sidong.Zhao@WT.CHG,2020/6/29,add for disable highvolt charger when flahlights on
static int pascal_set_scenario(int scenario)
{
	/* notify charger to increase or decrease voltage */
	mutex_lock(&fl_hv_mutex);
	if (scenario & FLASHLIGHT_SCENARIO_CAMERA_MASK) {
		if (!is_decrease_voltage) {
			pr_info("Decrease voltage level.\n");
			oppo_chg_set_camera_status(true);
			is_decrease_voltage = 1;
		}
	} else {
		if (is_decrease_voltage) {
			pr_info("Increase voltage level.\n");
			oppo_chg_set_camera_status(false);
			is_decrease_voltage = 0;
		}
	}
	mutex_unlock(&fl_hv_mutex);

	return 0;
}
#endif /* ODM_WT_EDIT*/
/* flashlight init */
static int pascal_init(void)
{
	int state = PASCAL_PINCTRL_PIN_STATE_LOW;

	pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN, state);
	pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_GPIO, state);
	return 0;
}

/* flashlight uninit */
static int pascal_uninit(void)
{
	int state = PASCAL_PINCTRL_PIN_STATE_LOW;

	pascal_pinctrl_set(PASCAL_PINCTRL_PIN_FLASH_EN, state);
	pascal_pinctrl_set(PASCAL_PINCTRL_PIN_PWM_GPIO, state);
	return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer pascal_timer;
static unsigned int pascal_timeout_ms;

static void pascal_work_disable(struct work_struct *data)
{
	PR_DEBUG_IF(dumpEnable, "work queue callback\n");
	pascal_disable();
}

static enum hrtimer_restart pascal_timer_func(struct hrtimer *timer)
{
	schedule_work(&pascal_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int pascal_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;
	unsigned int s;
	unsigned int ns;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		/* wenhui.Chen@Cam.Drv,20200523,modify for led bug */
		pascal_timeout_ms = 0;//fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		pascal_set_level(fl_arg->arg);
		break;

#ifdef ODM_WT_EDIT
//Sidong.Zhao@WT.CHG,2020/6/29,add for disable highvolt charger when flahlights on
	case FLASH_IOC_SET_SCENARIO:
		pr_debug("FLASH_IOC_SET_SCENARIO(%d): %d\n",
				channel, (int)fl_arg->arg);
		pascal_set_scenario(fl_arg->arg);
		break;
#endif /*ODM_WT_EDIT*/

	case FLASH_IOC_SET_ONOFF:
		pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (pascal_timeout_ms) {
				s = pascal_timeout_ms / 1000;
				ns = pascal_timeout_ms % 1000 * 1000000;
				ktime = ktime_set(s, ns);
				hrtimer_start(&pascal_timer, ktime,
						HRTIMER_MODE_REL);
			}
			pascal_enable();
		} else {
			pascal_disable();
			hrtimer_cancel(&pascal_timer);
		}
		break;
/*wenhui.chen@Camera.Driver, 20200423, add for flashlight calibration*/
	case FLASH_IOC_GET_DUTY_NUMBER:
		PR_INFO_IF(dumpEnable, "FLASH_IOC_GET_DUTY_NUMBER(%d)\n", channel);
		fl_arg->arg = PASCAL_LEVEL_NUM;
		break;

	case FLASH_IOC_GET_MAX_TORCH_DUTY:
		PR_INFO_IF(dumpEnable, "FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = PASCAL_LEVEL_TORCH - 1;
		break;

	case FLASH_IOC_GET_DUTY_CURRENT:
		fl_arg->arg = pascal_verify_level(fl_arg->arg);
		PR_INFO_IF(dumpEnable, "FLASH_IOC_GET_DUTY_CURRENT(%d): %d\n",
				channel, (int)fl_arg->arg);
		fl_arg->arg = pascal_current[fl_arg->arg];
		break;

	default:
		PR_INFO_IF(dumpEnable, "No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int pascal_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int pascal_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int pascal_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&pascal_mutex);
	if (set) {
		if (!use_count)
			ret = pascal_init();
		use_count++;
		PR_DEBUG_IF(dumpEnable, "Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = pascal_uninit();
		if (use_count < 0)
			use_count = 0;
		PR_DEBUG_IF(dumpEnable, "Unset driver: %d\n", use_count);
	}
	mutex_unlock(&pascal_mutex);

	return ret;
}

static ssize_t pascal_strobe_store(struct flashlight_arg arg)
{
	pascal_set_driver(1);
	pascal_set_level(arg.level);
	pascal_timeout_ms = 0;
	pascal_enable();
	msleep(arg.dur);
	pascal_disable();
	pascal_set_driver(0);

	return 0;
}

static struct flashlight_operations pascal_ops = {
	pascal_open,
	pascal_release,
	pascal_ioctl,
	pascal_strobe_store,
	pascal_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int pascal_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" for power saving.
	 * pascal_init();
	 */

	return 0;
}

static int pascal_parse_dt(struct device *dev,
		struct pascal_platform_data *pdata)
{
	struct device_node *np, *cnp;
	u32 decouple = 0;
	int i = 0;

	if (!dev || !dev->of_node || !pdata)
		return -ENODEV;

	np = dev->of_node;

	pdata->channel_num = of_get_child_count(np);
	if (!pdata->channel_num) {
		PR_INFO_IF(dumpEnable, "Parse no dt, node.\n");
		return 0;
	}
	PR_INFO_IF(dumpEnable, "Channel number(%d).\n", pdata->channel_num);

	if (of_property_read_u32(np, "decouple", &decouple))
		PR_INFO_IF(dumpEnable, "Parse no dt, decouple.\n");

	pdata->dev_id = devm_kzalloc(dev,
			pdata->channel_num *
			sizeof(struct flashlight_device_id),
			GFP_KERNEL);
	if (!pdata->dev_id)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		if (of_property_read_u32(cnp, "type", &pdata->dev_id[i].type))
			goto err_node_put;
		if (of_property_read_u32(cnp, "ct", &pdata->dev_id[i].ct))
			goto err_node_put;
		if (of_property_read_u32(cnp, "part", &pdata->dev_id[i].part))
			goto err_node_put;
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE,
				PASCAL_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		PR_INFO_IF(dumpEnable, "Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel,
				pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}

static int pascal_probe(struct platform_device *pdev)
{
	struct pascal_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int err;
	int i;

	PR_DEBUG_IF(dumpEnable, "Probe start.\n");

	/* init pinctrl */
	if (pascal_pinctrl_init(pdev)) {
		PR_DEBUG_IF(dumpEnable, "Failed to init pinctrl.\n");
		err = -EFAULT;
		goto err;
	}

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err;
		}
		pdev->dev.platform_data = pdata;
		err = pascal_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err;
	}

	/* init work queue */
	INIT_WORK(&pascal_work, pascal_work_disable);

	/* init timer */
	hrtimer_init(&pascal_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pascal_timer.function = pascal_timer_func;
	pascal_timeout_ms = 100;

	/* init chip hw */
	pascal_chip_init();

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&pascal_ops)) {
				err = -EFAULT;
				goto err;
			}
	} else {
		if (flashlight_dev_register(PASCAL_NAME, &pascal_ops)) {
			err = -EFAULT;
			goto err;
		}
	}
/*wenhui.chen@Camera.Driver, 20200423, add for flashlight calibration*/
	pascal_current = sgm3785_current;

#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&flashlight_wake_lock, "flashlight_lock_wakelock");
#endif

	PR_DEBUG_IF(dumpEnable, "Probe done.\n");

	return 0;
err:
	return err;
}

static int pascal_remove(struct platform_device *pdev)
{
	struct pascal_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	PR_DEBUG_IF(dumpEnable, "Remove start.\n");

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(PASCAL_NAME);

	/* flush work queue */
	flush_work(&pascal_work);

	PR_DEBUG_IF(dumpEnable, "Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id pascal_pwm_of_match[] = {
	{.compatible = PASCAL_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, pascal_pwm_of_match);
#else
static struct platform_device pascal_pwm_platform_device[] = {
	{
		.name = PASCAL_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, pascal_pwm_platform_device);
#endif

static struct platform_driver pascal_platform_driver = {
	.probe = pascal_probe,
	.remove = pascal_remove,
	.driver = {
		.name = PASCAL_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = pascal_pwm_of_match,
#endif
	},
};

static int __init flashlight_pascal_init(void)
{
	int ret;

	PR_DEBUG_IF(dumpEnable, "Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&pascal_pwm_platform_device);
	if (ret) {
		PR_ERR_IF(dumpEnable, "Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&pascal_platform_driver);
	if (ret) {
		PR_ERR_IF(dumpEnable, "Failed to register platform driver\n");
		return ret;
	}

	PR_DEBUG_IF(dumpEnable, "Init done.\n");

	return 0;
}

#ifdef ODM_WT_EDIT
#ifdef OPPO_FLASHLIGHT_TEST
/* Yang.guo@Camera.Driver, 2020/03/25, add for [wingtech ATO factory app camera] */
static void __exit flashlight_pascal_exit(void)
{
	PR_DEBUG_IF(dumpEnable, "Exit start.\n");

	platform_driver_unregister(&pascal_platform_driver);

	PR_DEBUG_IF(dumpEnable, "Exit done.\n");
}

int high_cct_led_strobe_enable_part1(void)
{
    PR_INFO_IF(dumpEnable, "!!!\n");
    return 0;
}

int high_cct_led_strobe_setduty_part1(int duty)
{

    pascal_set_level(27);
    return 0;

}

int high_cct_led_strobe_on_part1(int onoff)
{
    if(onoff)
        pascal_enable();
    else
        pascal_disable();
    return 0;
}

int low_cct_led_strobe_enable_part1(void)
{
    PR_INFO_IF(dumpEnable, "!!!\n");
    return 0;
}

int low_cct_led_strobe_setduty_part1(int duty)
{

  pascal_set_level(27);
  return 0;

}

int low_cct_led_strobe_on_part1(int onoff)
{
    if(onoff)
        pascal_enable();
    else
        pascal_disable();
    return 0;
}
#endif
#endif

module_init(flashlight_pascal_init);
module_exit(flashlight_pascal_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight PASCAL PWM Driver");

