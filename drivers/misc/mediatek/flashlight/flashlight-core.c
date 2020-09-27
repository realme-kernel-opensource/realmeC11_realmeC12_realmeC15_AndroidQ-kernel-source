/*
 * Flashlight Core
 *
 * Copyright (C) 2015 MediaTek Inc.
 *
 * Author: Simon Wang <Simon-TCH.Wang@mediatek.com>
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
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/string.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "flashlight-core.h"
#include "mach/upmu_sw.h" /* PT */

#ifdef CONFIG_MTK_FLASHLIGHT_DLPT
#include "mtk_pbm.h" /* DLPT */
#endif
#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Driver 20181218 add to control flashlight via proc file*/
#include<linux/proc_fs.h>
#endif

#ifdef ODM_WT_EDIT
#ifdef OPPO_FLASHLIGHT_TEST
/* Yang.guo@Camera.Driver, 2020/03/25, add for [wingtech ATO factory app camera] */
#include <linux/delay.h>
#endif
#endif

/* Shipei.Chen@Cam.Drv, 20200606, modify the way for log print!*/
#define PR_INFO_IF(cond, ...)      do { if ( (cond) ) { pr_info(__VA_ARGS__); } }while(0)
#define PR_DEBUG_IF(cond, ...)     do { if ( (cond) ) { pr_debug(__VA_ARGS__); } }while(0)
#define PR_ERR_IF(cond, ...)       do { if ( (cond) ) { pr_err(__VA_ARGS__); } }while(0)
static bool dumpEnable = 0;

/******************************************************************************
 * Definition
 *****************************************************************************/
static DEFINE_MUTEX(fl_mutex);
LIST_HEAD(flashlight_list);

#ifndef FLASHLIGHT_BRIGHTNESS_ADD
#define FLASHLIGHT_BRIGHTNESS_ADD
#endif

#ifdef FLASHLIGHT_BRIGHTNESS_ADD
#define F_BACK_NAME_SYSFS  "back"
#define F_BACK_BRIGHTNESS_ATTR   "pascalbrightness"

int flashlight_back_brightness=0;
extern int pascal_enable_flash(int level);

/* boot object */
static struct kobject flashlight_back_brightness_kobj;

static ssize_t flashlight_back_brightness_show(struct kobject *kobj, struct attribute *a, char *buf)
{
	return sprintf(buf, "%d\n", flashlight_back_brightness);
}

static ssize_t flashlight_back_brightness_store(struct kobject *kobj, struct attribute *a, const char *buf, size_t count);

static struct sysfs_ops flashlight_back_brightness_ops = {
	.show = flashlight_back_brightness_show,
	.store = flashlight_back_brightness_store
};

/* boot attribute */
struct attribute f_back_brightness_attr = {F_BACK_BRIGHTNESS_ATTR, 0644};
static struct attribute *f_back_brightness_attrs[] = {
	&f_back_brightness_attr,
	NULL
};

/*flashlight type */
static struct kobj_type flashlight_back_brightness_ktype = {
	.sysfs_ops = &flashlight_back_brightness_ops,
	.default_attrs = f_back_brightness_attrs
};

static void flashlight_back_brightness_set(int brightness){
	int i4RetValue = 0;

	i4RetValue=pascal_enable_flash(brightness);
	if(!i4RetValue)
	PR_INFO_IF(dumpEnable, "prepare to flashing\n");
}

static ssize_t flashlight_back_brightness_store(struct kobject *kobj, struct attribute *a, const char *buf, size_t count)
{
	unsigned long state;
	ssize_t ret = -EINVAL;

	ret = kstrtoul(buf, 10, &state);
	if (ret)
	return ret;

	flashlight_back_brightness = (int)state;
	PR_INFO_IF(dumpEnable, "flashlight_back_brightness_store %d\r\n",flashlight_back_brightness);
	flashlight_back_brightness_set(flashlight_back_brightness);
	return count;
}
#endif

/* duty current */
static struct flashlight_arg duty_current_arg;

/* power variables */
#ifdef CONFIG_MTK_FLASHLIGHT_PT
static int pt_low_vol = LOW_BATTERY_LEVEL_0;
static int pt_low_bat = BATTERY_PERCENT_LEVEL_0;
static int pt_over_cur = BATTERY_OC_LEVEL_0;

#ifdef CONFIG_MTK_FLASHLIGHT_PT_STRICT
static int pt_strict = 1;
#else
static int pt_strict; /* always be zero in C standard */
#endif

static int pt_is_low(int pt_low_vol, int pt_low_bat, int pt_over_cur);
#endif


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int fl_set_level(struct flashlight_dev *fdev, int level)
{
	struct flashlight_dev_arg fl_dev_arg;

	if (!fdev || !fdev->ops) {
		PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
		return -EINVAL;
	}

	/* if pt is low */
#ifdef CONFIG_MTK_FLASHLIGHT_PT
	if (pt_is_low(pt_low_vol, pt_low_bat, pt_over_cur))
		if (fdev->low_pt_level >= 0 && level > fdev->low_pt_level) {
			level = fdev->low_pt_level;
			PR_INFO_IF(dumpEnable, "Set level to (%d) since pt(%d,%d,%d), pt strict(%d)\n",
					level, pt_low_vol, pt_low_bat,
					pt_over_cur, pt_strict);
		}
#endif

	/* ioctl */
	fl_dev_arg.channel = fdev->dev_id.channel;
	fl_dev_arg.arg = level;
	if (fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY,
				(unsigned long)&fl_dev_arg)) {
		PR_ERR_IF(dumpEnable, "Failed to set level\n");
		return -EFAULT;
	}

	/* update device status */
	fdev->level = level;

	return 0;
}

static int fl_enable(struct flashlight_dev *fdev, int enable)
{
	struct flashlight_dev_arg fl_dev_arg;

	if (!fdev || !fdev->ops) {
		PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
		return -EINVAL;
	}

	/* consider pt status */
#ifdef CONFIG_MTK_FLASHLIGHT_DLPT
	kicker_pbm_by_flash(enable);
#endif
#ifdef CONFIG_MTK_FLASHLIGHT_PT
	if (pt_is_low(pt_low_vol, pt_low_bat, pt_over_cur) == 2)
		if (enable) {
			enable = 0;
			PR_INFO_IF(dumpEnable, "Failed to enable since pt(%d,%d,%d), pt strict(%d)\n",
					pt_low_vol, pt_low_bat,
					pt_over_cur, pt_strict);
		}
#endif

	if (fdev->sw_disable_status == FLASHLIGHT_SW_DISABLE_ON) {
		PR_INFO_IF(dumpEnable, "Sw disable on\n");
		return 0;
	}

	/* ioctl */
	fl_dev_arg.channel = fdev->dev_id.channel;
	fl_dev_arg.arg = enable;
	if (fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF,
				(unsigned long)&fl_dev_arg)) {
		PR_ERR_IF(dumpEnable, "Failed to set on/off\n");
		return -EFAULT;
	}

	/* update device status */
	fdev->enable = enable;

	return 0;
}

/* verify function */
int flashlight_verify_type_index(int type_index)
{
	if (type_index < 0 || type_index >= FLASHLIGHT_TYPE_MAX) {
		PR_ERR_IF(dumpEnable, "type index (%d) is not valid\n", type_index);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(flashlight_verify_type_index);

int flashlight_verify_ct_index(int ct_index)
{
	if (ct_index < 0 || ct_index >= FLASHLIGHT_CT_MAX) {
		PR_ERR_IF(dumpEnable, "ct index (%d) is not valid\n", ct_index);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(flashlight_verify_ct_index);

int flashlight_verify_part_index(int part_index)
{
	if (part_index < 0 || part_index >= FLASHLIGHT_PART_MAX) {
		PR_ERR_IF(dumpEnable, "part index (%d) is not valid\n", part_index);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(flashlight_verify_part_index);

int flashlight_verify_index(int type_index, int ct_index, int part_index)
{
	if (flashlight_verify_type_index(type_index) ||
			flashlight_verify_ct_index(ct_index) ||
			flashlight_verify_part_index(part_index))
		return -1;
	return 0;
}
EXPORT_SYMBOL(flashlight_verify_index);

static int flashlight_verify_arg(struct flashlight_arg fl_arg)
{
	if (flashlight_verify_index(fl_arg.type, fl_arg.ct, fl_arg.part))
		return -1;
	if (fl_arg.level < -1 || fl_arg.level > FLASHLIGHT_ARG_LEVEL_MAX) {
		PR_ERR_IF(dumpEnable, "level (%d) is not valid\n", fl_arg.level);
		return -1;
	}
	if (fl_arg.dur < 0 || fl_arg.dur > FLASHLIGHT_ARG_DUR_MAX) {
		PR_ERR_IF(dumpEnable, "duration (%d) is not valid\n", fl_arg.dur);
		return -1;
	}

	return 0;
}

/* get id */
int flashlight_get_type_id(int type_index)
{
	if (flashlight_verify_type_index(type_index)) {
		PR_ERR_IF(dumpEnable, "type index (%d) is not valid\n", type_index);
		return -1;
	}

	return type_index + 1;
}
EXPORT_SYMBOL(flashlight_get_type_id);

int flashlight_get_ct_id(int ct_index)
{
	if (flashlight_verify_ct_index(ct_index)) {
		PR_ERR_IF(dumpEnable, "color temperature index (%d) is not valid\n", ct_index);
		return -1;
	}

	return ct_index + 1;
}
EXPORT_SYMBOL(flashlight_get_ct_id);

int flashlight_get_part_id(int part_index)
{
	if (flashlight_verify_part_index(part_index)) {
		PR_ERR_IF(dumpEnable, "part (%d) is not valid\n", part_index);
		return -1;
	}

	return part_index + 1;
}
EXPORT_SYMBOL(flashlight_get_part_id);

/* get index */
int flashlight_get_type_index(int type_id)
{
	if (type_id < 1 || type_id > FLASHLIGHT_TYPE_MAX) {
		PR_ERR_IF(dumpEnable, "type id (%d) is not valid\n", type_id);
		return -1;
	}

	return type_id - 1;
}
EXPORT_SYMBOL(flashlight_get_type_index);

int flashlight_get_ct_index(int ct_id)
{
	if (ct_id < 1 || ct_id > FLASHLIGHT_CT_MAX) {
		PR_ERR_IF(dumpEnable, "color temperature id (%d) is not valid\n", ct_id);
		return -1;
	}

	return ct_id - 1;
}
EXPORT_SYMBOL(flashlight_get_ct_index);

int flashlight_get_part_index(int part_id)
{
	if (part_id < 1 || part_id > FLASHLIGHT_PART_MAX) {
		PR_ERR_IF(dumpEnable, "part id (%d) is not valid\n", part_id);
		return -1;
	}

	return part_id - 1;
}
EXPORT_SYMBOL(flashlight_get_part_index);


/******************************************************************************
 * Flashlight devices
 *****************************************************************************/
/* find device */
static struct flashlight_dev *flashlight_find_dev_by_full_index(
		const int type, const int ct, const int part)
{
	struct flashlight_dev *fdev;

	/* return the first flashlight device */
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (fdev->dev_id.type == type &&
				fdev->dev_id.ct == ct &&
				fdev->dev_id.part == part)
			return fdev;
	}

	return NULL;
}

static struct flashlight_dev *flashlight_find_dev_by_index(
		const int type, const int ct)
{
	struct flashlight_dev *fdev;

	/* return the first flashlight device */
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (fdev->dev_id.type == type && fdev->dev_id.ct == ct)
			return fdev;
	}

	return NULL;
}

static struct flashlight_dev *flashlight_find_dev_by_device_id(
		const struct flashlight_device_id *dev_id)
{
	struct flashlight_dev *fdev;

	if (!dev_id)
		return NULL;

	/* return the first flashlight device */
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (fdev->dev_id.type == dev_id->type &&
				fdev->dev_id.ct == dev_id->ct &&
				fdev->dev_id.part == dev_id->part)
			return fdev;
	}

	return NULL;
}

/*
 * Register devices
 *
 * Please DO NOT register flashlight device driver,
 * until success to probe hardware.
 */
int flashlight_dev_register(
		const char *name, struct flashlight_operations *dev_ops)
{
	struct flashlight_dev *fdev;
	int type_index, ct_index, part_index;
	int i;

	for (i = 0; i < flashlight_device_num; i++) {
		if (!strncmp(name, flashlight_id[i].name,
					FLASHLIGHT_NAME_SIZE)) {
			type_index = flashlight_id[i].type;
			ct_index = flashlight_id[i].ct;
			part_index = flashlight_id[i].part;

			if (flashlight_verify_index(
						type_index,
						ct_index,
						part_index)) {
				PR_ERR_IF(dumpEnable, "Failed to register device (%s)\n",
						flashlight_id[i].name);
				continue;
			}

			PR_INFO_IF(dumpEnable, "%s %d %d %d\n", flashlight_id[i].name,
					type_index, ct_index, part_index);

			mutex_lock(&fl_mutex);
			fdev = kzalloc(sizeof(*fdev), GFP_KERNEL);
			if (!fdev) {
				mutex_unlock(&fl_mutex);
				return -ENOMEM;
			}
			fdev->ops = dev_ops;
			fdev->dev_id = flashlight_id[i];
			fdev->low_pt_level = -1;
			fdev->charger_status = FLASHLIGHT_CHARGER_READY;
			list_add_tail(&fdev->node, &flashlight_list);
			mutex_unlock(&fl_mutex);
		}
	}

	return 0;
}
EXPORT_SYMBOL(flashlight_dev_register);

int flashlight_dev_unregister(const char *name)
{
	struct flashlight_dev *fdev;
	int type_index, ct_index, part_index;
	int i;

	for (i = 0; i < flashlight_device_num; i++) {
		if (!strncmp(name, flashlight_id[i].name,
					FLASHLIGHT_NAME_SIZE)) {
			type_index = flashlight_id[i].type;
			ct_index = flashlight_id[i].ct;
			part_index = flashlight_id[i].part;

			if (flashlight_verify_index(
						type_index,
						ct_index,
						part_index)) {
				PR_ERR_IF(dumpEnable, "Failed to unregister device (%s)\n",
						flashlight_id[i].name);
				continue;
			}

			PR_INFO_IF(dumpEnable, "%s %d %d %d\n", flashlight_id[i].name,
					type_index, ct_index, part_index);

			mutex_lock(&fl_mutex);
			fdev = flashlight_find_dev_by_device_id(
					&flashlight_id[i]);
			if (fdev) {
				list_del(&fdev->node);
				kfree(fdev);
			}
			mutex_unlock(&fl_mutex);
		}
	}

	return 0;
}
EXPORT_SYMBOL(flashlight_dev_unregister);

/*
 * Register devices
 *
 * Please DO NOT register flashlight device driver,
 * until success to probe hardware.
 */
int flashlight_dev_register_by_device_id(
		struct flashlight_device_id *dev_id,
		struct flashlight_operations *dev_ops)
{
	struct flashlight_dev *fdev;

	if (!dev_id || !dev_ops)
		return -EINVAL;

	if (flashlight_verify_index(dev_id->type, dev_id->ct, dev_id->part)) {
		PR_ERR_IF(dumpEnable, "Failed to register device (%d,%d,%d)\n",
				dev_id->type, dev_id->ct, dev_id->part);
		return -EINVAL;
	}

	PR_INFO_IF(dumpEnable, "Register device (%d,%d,%d)\n",
			dev_id->type, dev_id->ct, dev_id->part);

	mutex_lock(&fl_mutex);
	fdev = kzalloc(sizeof(*fdev), GFP_KERNEL);
	if (!fdev) {
		mutex_unlock(&fl_mutex);
		return -ENOMEM;
	}
	fdev->ops = dev_ops;
	fdev->dev_id = *dev_id;
	fdev->low_pt_level = -1;
	fdev->charger_status = FLASHLIGHT_CHARGER_READY;
	list_add_tail(&fdev->node, &flashlight_list);
	mutex_unlock(&fl_mutex);

	return 0;
}
EXPORT_SYMBOL(flashlight_dev_register_by_device_id);

int flashlight_dev_unregister_by_device_id(struct flashlight_device_id *dev_id)
{
	struct flashlight_dev *fdev;

	if (!dev_id)
		return -EINVAL;

	if (flashlight_verify_index(dev_id->type, dev_id->ct, dev_id->part)) {
		PR_ERR_IF(dumpEnable, "Failed to unregister device (%d,%d,%d)\n",
				dev_id->type, dev_id->ct, dev_id->part);
		return -EINVAL;
	}

	PR_INFO_IF(dumpEnable, "Unregister device (%d,%d,%d)\n",
			dev_id->type, dev_id->ct, dev_id->part);

	mutex_lock(&fl_mutex);
	fdev = flashlight_find_dev_by_device_id(dev_id);
	if (fdev) {
		list_del(&fdev->node);
		kfree(fdev);
	}
	mutex_unlock(&fl_mutex);

	return 0;
}
EXPORT_SYMBOL(flashlight_dev_unregister_by_device_id);


/******************************************************************************
 * Vsync IRQ
 *****************************************************************************/
/*
 * LED flash control for high current capture mode
 * which is used by "imgsensor/src/[PLAT]/kd_sensorlist.c"
 *
 * Already be removed from kernel-4.4.
 */
ssize_t strobe_VDIrq(void)
{
	return 0;
}
EXPORT_SYMBOL(strobe_VDIrq);
#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Driver 20181218 add to control flashlight via proc file*/
static int flashlight_state = 0;
static ssize_t FL_HW_WRITE( struct file *file, const char __user *buffer, size_t count,
                                                                     loff_t *data)
{
	char regBuf[64] = {'\0'};
	struct flashlight_dev_arg fl_dev_arg;
	struct flashlight_dev *fdev;

	u32 u4CopyBufSize = (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

	if (copy_from_user(regBuf, buffer, u4CopyBufSize))
		return -EFAULT;

	PR_ERR_IF(dumpEnable, "new_state = %d, old_flashlight_state:%d\n",regBuf[0] - '0',flashlight_state);
	if(regBuf[0] == '6') {
		flashlight_state = regBuf[0] - '0';
		return count;
	}
	if (regBuf[0] == '5' && flashlight_state != 1) {
		flashlight_state = regBuf[0] - '0';
		return count;
	}

	if (flashlight_state == regBuf[0] - '0') {
		PR_ERR_IF(dumpEnable, "flash state is same, do not need set flash \n");
		return count;
	}

	fdev = flashlight_find_dev_by_full_index(0, 0, 0);
	if (!fdev) {
		PR_INFO_IF(dumpEnable, "Find no flashlight device\n");
		return -EFAULT;
	}

	if (regBuf[0] == '5' && flashlight_state == 1) {
		flashlight_state = regBuf[0] - '0';

		fl_dev_arg.channel = fdev->dev_id.channel;
		fl_dev_arg.arg = 0;
		if (fdev->ops) {
			/* Henry.Chang@Camera.Driver add for Dual channel flashlight 20190223 */
			fdev->ops->flashlight_set_driver(1);
			PR_INFO_IF(dumpEnable, "regBuf[0] == '5' set driver:1");
			fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
			fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		} else {
			PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
		}
		PR_ERR_IF(dumpEnable, "sensor is poweron ,need to set flash off\n");
		return count;
	}

	if (flashlight_state == 5 && regBuf[0] == '0') {
		PR_ERR_IF(dumpEnable, "camera is open ,not to set flash\n");
		return count;
	}
	if (flashlight_state == 6 && regBuf[0] == '0') {
		return count;
	}

	if(regBuf[0] == '0') {
		flashlight_state = regBuf[0] - '0';

		fl_dev_arg.channel = fdev->dev_id.channel;
		fl_dev_arg.arg = 0;
		if (fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg)) {
			PR_ERR_IF(dumpEnable, "Failed to set on/off.\n");
		} else {
			PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
		}
		/* Henry.Chang@Camera.Driver add for Dual channel flashlight 20190223 */
		if (fdev->ops) {
			fdev->ops->flashlight_set_driver(0);
			PR_INFO_IF(dumpEnable, "set driver 0");
		}
	} else if (regBuf[0] == '1') {
		fl_dev_arg.channel = fdev->dev_id.channel;
		fl_dev_arg.arg = 0;
		if (fdev->ops) {
			/* Henry.Chang@Camera.Driver add for Dual channel flashlight 20190223 */
			fdev->ops->flashlight_set_driver(1);
			PR_INFO_IF(dumpEnable, "regBuf[0] == '1'set driver 1");
			fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
			fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		} else {
			PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
			return -EFAULT;
		}
		fl_dev_arg.arg = 1;
		if (fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg)) {
			PR_ERR_IF(dumpEnable, "Failed to set on/off.\n");
		}
	}
	flashlight_state = regBuf[0] - '0';

	PR_ERR_IF(dumpEnable, "flashlight_state=%d\n",flashlight_state);

	return count;
}

static ssize_t FL_HW_READ(struct file *filp, char __user *buff,
						size_t len, loff_t *data)
{
	char value[2] = {0};

	snprintf(value, sizeof(value), "%d", flashlight_state);
	return simple_read_from_buffer(buff, len, data, value,1);
}

static const struct file_operations flashlight_proc_fops = {
	.owner = THIS_MODULE,
	.read = FL_HW_READ,
	.write = FL_HW_WRITE,
};

static int flash_proc_init(void)
{
	int ret=0;
	struct proc_dir_entry *proc_entry = proc_create_data( "qcom_flash", 0666, NULL,&flashlight_proc_fops, NULL);
	if (proc_entry == NULL) {
		ret = -ENOMEM;
		PR_ERR_IF(dumpEnable, "Error! Couldn't create qcom_flash proc entry\n");
	}
	return ret;
}
#endif /*VENDOR_EDIT*/


/******************************************************************************
 * Charger Status
 *****************************************************************************/
static int flashlight_update_charger_status(struct flashlight_dev *fdev)
{
	struct flashlight_dev_arg fl_dev_arg;

	if (!fdev || !fdev->ops) {
		PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
		return -EINVAL;
	}

	/* ioctl */
	fl_dev_arg.channel = fdev->dev_id.channel;
	if (fdev->ops->flashlight_ioctl(FLASH_IOC_IS_CHARGER_READY,
				(unsigned long)&fl_dev_arg))
		PR_INFO_IF(dumpEnable, "Failed to get charger status\n");
	else
		fdev->charger_status = fl_dev_arg.arg;

	return 0;
}


/******************************************************************************
 * Power throttling
 *****************************************************************************/
#ifdef CONFIG_MTK_FLASHLIGHT_PT
static int pt_arg_verify(int pt_low_vol, int pt_low_bat, int pt_over_cur)
{
	if (pt_low_vol < LOW_BATTERY_LEVEL_0 ||
			pt_low_vol > LOW_BATTERY_LEVEL_2) {
		PR_ERR_IF(dumpEnable, "PT low voltage (%d) is not valid\n", pt_low_vol);
		return -1;
	}
	if (pt_low_bat < BATTERY_PERCENT_LEVEL_0 ||
			pt_low_bat > BATTERY_PERCENT_LEVEL_1) {
		PR_ERR_IF(dumpEnable, "PT low battery (%d) is not valid\n", pt_low_bat);
		return -1;
	}
	if (pt_over_cur < BATTERY_OC_LEVEL_0 ||
			pt_over_cur > BATTERY_OC_LEVEL_1) {
		PR_ERR_IF(dumpEnable, "PT over current (%d) is not valid\n", pt_over_cur);
		return -1;
	}

	return 0;
}

static int pt_is_low(int pt_low_vol, int pt_low_bat, int pt_over_cur)
{
	int is_low = 0;

	if (pt_low_bat != BATTERY_PERCENT_LEVEL_0
			|| pt_low_vol != LOW_BATTERY_LEVEL_0
			|| pt_over_cur != BATTERY_OC_LEVEL_0) {
		is_low = 1;
		if (pt_strict)
			is_low = 2;
	}

	return is_low;
}

static int pt_trigger(void)
{
	struct flashlight_dev *fdev;
	int is_flash_enable = 0;

	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (fdev->enable)
			is_flash_enable = 1;
	}
	if (is_flash_enable) {
		list_for_each_entry(fdev, &flashlight_list, node) {
			if (!fdev->ops)
				continue;

			fdev->ops->flashlight_open();
			fdev->ops->flashlight_set_driver(1);
			if (pt_strict) {
				PR_INFO_IF(dumpEnable, "PT trigger(%d,%d,%d) disable flashlight\n",
					pt_low_vol, pt_low_bat, pt_over_cur);
				fl_enable(fdev, 0);
			} else {
				PR_INFO_IF(dumpEnable, "PT trigger(%d,%d,%d) decrease duty: %d\n",
					pt_low_vol, pt_low_bat,
					pt_over_cur, fdev->low_pt_level);
				fl_set_level(fdev, fdev->low_pt_level);
			}
			fdev->ops->flashlight_set_driver(0);
			fdev->ops->flashlight_release();
		}
	}
	mutex_unlock(&fl_mutex);

	return 0;
}
#ifndef VENDOR_EDIT
/* Qiao.Hu@BSP.BaseDrv.CHG.Basic, 2018/05/25, Add for pmic invalid at low battery voltage */
static void pt_low_vol_callback(LOW_BATTERY_LEVEL level)
{
	if (level == LOW_BATTERY_LEVEL_0) {
		pt_low_vol = LOW_BATTERY_LEVEL_0;
	} else if (level == LOW_BATTERY_LEVEL_1) {
		pt_low_vol = LOW_BATTERY_LEVEL_1;
		pt_trigger();
	} else if (level == LOW_BATTERY_LEVEL_2) {
		pt_low_vol = LOW_BATTERY_LEVEL_2;
		pt_trigger();
	} else {
		/* unlimited cpu and gpu */
	}
}

static void pt_low_bat_callback(BATTERY_PERCENT_LEVEL level)
{
	if (level == BATTERY_PERCENT_LEVEL_0) {
		pt_low_bat = BATTERY_PERCENT_LEVEL_0;
	} else if (level == BATTERY_PERCENT_LEVEL_1) {
		pt_low_bat = BATTERY_PERCENT_LEVEL_1;
		pt_trigger();
	} else {
		/* unlimited cpu and gpu*/
	}
}
#endif
static void pt_oc_callback(BATTERY_OC_LEVEL level)
{
	if (level == BATTERY_OC_LEVEL_0) {
		pt_over_cur = BATTERY_OC_LEVEL_0;
	} else if (level == BATTERY_OC_LEVEL_1) {
		pt_over_cur = BATTERY_OC_LEVEL_1;
		pt_trigger();
	} else {
		/* unlimited cpu and gpu*/
	}
}
#endif


/******************************************************************************
 * File operations
 *****************************************************************************/
static long _flashlight_ioctl(
		struct file *file, unsigned int cmd, unsigned long arg)
{
	struct flashlight_user_arg fl_arg;
	struct flashlight_dev_arg fl_dev_arg;
	struct flashlight_dev *fdev;
	int type, ct, part;
	int ret = 0;

	memset(&fl_arg, 0, sizeof(struct flashlight_user_arg));
	if (copy_from_user(&fl_arg, (void __user *)arg,
				sizeof(struct flashlight_user_arg))) {
		PR_ERR_IF(dumpEnable, "Failed copy arguments from user\n");
		return -EFAULT;
	}

	/* find flashlight device */
	mutex_lock(&fl_mutex);
	fdev = flashlight_find_dev_by_index(
			flashlight_get_type_index(fl_arg.type_id),
			flashlight_get_ct_index(fl_arg.ct_id));
	mutex_unlock(&fl_mutex);
	if (!fdev) {
		pr_info_ratelimited("Find no flashlight device\n");
		return -EINVAL;
	}

	/* setup flash dev arguments */
	fl_dev_arg.arg = fl_arg.arg;
	fl_dev_arg.channel = fdev->dev_id.channel;
	type = fdev->dev_id.type;
	ct = fdev->dev_id.ct;
	part = fdev->dev_id.part;

	if (flashlight_verify_index(type, ct, part)) {
		PR_ERR_IF(dumpEnable, "Failed with error index\n");
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_GET_PROTOCOL_VERSION:
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_GET_PROTOCOL_VERSION(%d,%d,%d): %d\n",
				type, ct, part, FLASHLIGHT_PROTOCOL_VERSION);
		ret = FLASHLIGHT_PROTOCOL_VERSION;
		break;

	case FLASH_IOC_IS_LOW_POWER:
		fl_arg.arg = 0;
#ifdef CONFIG_MTK_FLASHLIGHT_PT
		fl_arg.arg = pt_is_low(pt_low_vol, pt_low_bat, pt_over_cur);
		if (fl_arg.arg)
			PR_DEBUG_IF(dumpEnable, "Pt status: (%d,%d,%d)\n",
					pt_low_vol, pt_low_bat, pt_over_cur);
#endif
		if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
			PR_ERR_IF(dumpEnable, "Failed to copy power status to user\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_LOW_POWER_DETECT_START:
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_LOW_POWER_DETECT_START(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		mutex_lock(&fl_mutex);
		fdev->low_pt_level = fl_arg.arg;
		mutex_unlock(&fl_mutex);
		break;

	case FLASH_IOC_LOW_POWER_DETECT_END:
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_LOW_POWER_DETECT_END(%d,%d,%d)\n",
				type, ct, part);
		mutex_lock(&fl_mutex);
		fdev->low_pt_level = -1;
		mutex_unlock(&fl_mutex);
		break;

	case FLASH_IOC_IS_CHARGER_READY:
		mutex_lock(&fl_mutex);
		flashlight_update_charger_status(fdev);
		mutex_unlock(&fl_mutex);
		fl_arg.arg = fdev->charger_status;
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_IS_CHARGER_READY(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
			PR_ERR_IF(dumpEnable, "Failed to copy charger status to user\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_IS_HARDWARE_READY:
		if (fdev->ops)
			fl_arg.arg = 1;
		else
			fl_arg.arg = 0;
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_IS_HARDWARE_READY(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
			PR_ERR_IF(dumpEnable, "Failed to copy hardware status to user\n");
			return -EFAULT;
		}
		break;

	case FLASHLIGHTIOC_X_SET_DRIVER:
		PR_DEBUG_IF(dumpEnable, "FLASHLIGHTIOC_X_SET_DRIVER(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		if (fdev->ops) {
			ret = fdev->ops->flashlight_set_driver(fl_arg.arg);
			if (fdev->dev_id.decouple) {
				fl_dev_arg.arg = FLASHLIGHT_SCENARIO_DECOUPLE;
				fdev->ops->flashlight_ioctl(
					FLASH_IOC_SET_SCENARIO,
					(unsigned long)&fl_dev_arg);
			}
		} else {
			PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_SET_SCENARIO:
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_SET_SCENARIO(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		if (fdev->ops) {
			if (fdev->dev_id.decouple)
				fl_dev_arg.arg |= FLASHLIGHT_SCENARIO_DECOUPLE;
			ret = fdev->ops->flashlight_ioctl(
					cmd, (unsigned long)&fl_dev_arg);
		} else {
			PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_GET_PART_ID:
	case FLASH_IOC_GET_MAIN_PART_ID:
	case FLASH_IOC_GET_SUB_PART_ID:
	case FLASH_IOC_GET_MAIN2_PART_ID:
		fl_arg.arg = flashlight_get_part_id(part);
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_GET_PART_ID(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
			PR_ERR_IF(dumpEnable, "Failed to copy part id to user\n");
			return -EFAULT;
		}
		break;

	case FLASH_IOC_SET_DUTY:
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_SET_DUTY(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		mutex_lock(&fl_mutex);
		ret = fl_set_level(fdev, fl_arg.arg);
		mutex_unlock(&fl_mutex);
		break;

	case FLASH_IOC_SET_ONOFF:
		PR_DEBUG_IF(dumpEnable, "FLASH_IOC_SET_ONOFF(%d,%d,%d): %d\n",
				type, ct, part, fl_arg.arg);
		mutex_lock(&fl_mutex);
		ret = fl_enable(fdev, fl_arg.arg);
		mutex_unlock(&fl_mutex);
		break;

	case FLASH_IOC_GET_DUTY_NUMBER:
	case FLASH_IOC_GET_DUTY_CURRENT:
	case FLASH_IOC_GET_HW_FAULT:
	case FLASH_IOC_GET_HW_FAULT2:
		if (fdev->ops) {
			ret = fdev->ops->flashlight_ioctl(
					cmd, (unsigned long)&fl_dev_arg);
			fl_arg.arg = fl_dev_arg.arg;
			if (copy_to_user((void __user *)arg, (void *)&fl_arg,
					sizeof(struct flashlight_user_arg))) {
				PR_INFO_IF(dumpEnable, "Failed to copy arg to user cmd:%d\n",
					_IOC_NR(cmd));
				return -EFAULT;
			}
		} else {
			PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
			return -ENOTTY;
		}
		break;

	default:
		if (fdev->ops)
			ret = fdev->ops->flashlight_ioctl(
					cmd, (unsigned long)&fl_dev_arg);
		else {
			PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
			return -ENOTTY;
		}
		break;
	}

	return ret;
}

static long flashlight_ioctl(
		struct file *file, unsigned int cmd, unsigned long arg)
{
	return _flashlight_ioctl(file, cmd, arg);
}

#ifdef CONFIG_COMPAT
static long flashlight_compat_ioctl(
		struct file *filep, unsigned int cmd, unsigned long arg)
{
	return _flashlight_ioctl(filep, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int flashlight_open(struct inode *inode, struct file *file)
{
	struct flashlight_dev *fdev;

	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)
			continue;

		PR_DEBUG_IF(dumpEnable, "Open(%d,%d,%d)\n", fdev->dev_id.type,
				fdev->dev_id.ct, fdev->dev_id.part);
		fdev->ops->flashlight_open();
	}
	mutex_unlock(&fl_mutex);

	return 0;
}

static int flashlight_release(struct inode *inode, struct file *file)
{
	struct flashlight_dev *fdev;

	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)
			continue;

		PR_DEBUG_IF(dumpEnable, "Release(%d,%d,%d)\n", fdev->dev_id.type,
				fdev->dev_id.ct, fdev->dev_id.part);
		fl_enable(fdev, 0);
		fdev->ops->flashlight_release();
	}
	mutex_unlock(&fl_mutex);

	return 0;
}

static const struct file_operations flashlight_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = flashlight_ioctl,
	.open = flashlight_open,
	.release = flashlight_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = flashlight_compat_ioctl,
#endif
};


/******************************************************************************
 * SYSFS
 *****************************************************************************/
/* flashlight strobe sysfs */
static ssize_t flashlight_strobe_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	PR_DEBUG_IF(dumpEnable, "Strobe show\n");

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [CT] [PART] [LEVEL] [DURATION(ms)]\n");
}

static ssize_t flashlight_strobe_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct flashlight_dev *fdev;
	struct flashlight_arg fl_arg;
	s32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	PR_DEBUG_IF(dumpEnable, "Strobe store\n");

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtos32(token, 10, &num);
		if (ret) {
			PR_ERR_IF(dumpEnable, "Error arguments\n");
			goto unlock;
		}

		if (count == FLASHLIGHT_ARG_TYPE)
			fl_arg.type = (int)num;
		else if (count == FLASHLIGHT_ARG_CT)
			fl_arg.ct = (int)num;
		else if (count == FLASHLIGHT_ARG_PART)
			fl_arg.part = (int)num;
		else if (count == FLASHLIGHT_ARG_LEVEL)
			fl_arg.level = (int)num;
		else if (count == FLASHLIGHT_ARG_DUR)
			fl_arg.dur = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != FLASHLIGHT_ARG_NUM) {
		PR_ERR_IF(dumpEnable, "Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}
	if (flashlight_verify_arg(fl_arg)) {
		PR_ERR_IF(dumpEnable, "Error arguments\n");
		ret = -1;
		goto unlock;
	}

	PR_DEBUG_IF(dumpEnable, "(%d, %d, %d), (%d, %d)\n",
			fl_arg.type, fl_arg.ct, fl_arg.part,
			fl_arg.level, fl_arg.dur);

	/* call callback function */
	mutex_lock(&fl_mutex);
	fdev = flashlight_find_dev_by_full_index(
			fl_arg.type, fl_arg.ct, fl_arg.part);
	mutex_unlock(&fl_mutex);
	if (!fdev) {
		PR_INFO_IF(dumpEnable, "Find no flashlight device\n");
		ret = -1;
		goto unlock;
	}

	fl_arg.channel = fdev->dev_id.channel;
	fl_arg.decouple = fdev->dev_id.decouple;

	PR_INFO_IF(dumpEnable, "channel:%d decouple:%d\n",
			fl_arg.channel, fl_arg.decouple);

	if (fdev->ops) {
		fdev->ops->flashlight_strobe_store(fl_arg);
		ret = size;
	} else {
		PR_INFO_IF(dumpEnable, "Failed with no flashlight ops\n");
		ret = -1;
	}

unlock:
	return ret;
}
static DEVICE_ATTR_RW(flashlight_strobe);

/* pt status sysfs */
static ssize_t flashlight_pt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	PR_DEBUG_IF(dumpEnable, "Power throttling show\n");

#ifdef CONFIG_MTK_FLASHLIGHT_PT
	return scnprintf(buf, PAGE_SIZE,
			"[LOW_VOLTAGE] [LOW_BATTERY] [OVER_CURRENT] [PT_STRICT]\n%d %d %d %d\n",
			pt_low_vol, pt_low_bat, pt_over_cur, pt_strict);
#else
	return scnprintf(buf, PAGE_SIZE,
			"No support power throttling\n");
#endif
}

static ssize_t flashlight_pt_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int low_vol = LOW_BATTERY_LEVEL_0;
	int low_bat = BATTERY_PERCENT_LEVEL_0;
	int over_cur = BATTERY_OC_LEVEL_0;
	int strict = 1;
	u32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	PR_DEBUG_IF(dumpEnable, "Power throttling store\n");

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtou32(token, 10, &num);
		if (ret) {
			PR_ERR_IF(dumpEnable, "Error arguments\n");
			goto unlock;
		}

		if (count == PT_NOTIFY_LOW_VOL)
			low_vol = (int)num;
		else if (count == PT_NOTIFY_LOW_BAT)
			low_bat = (int)num;
		else if (count == PT_NOTIFY_OVER_CUR)
			over_cur = (int)num;
		else if (count == PT_NOTIFY_STRICT)
			strict = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != PT_NOTIFY_NUM) {
		PR_ERR_IF(dumpEnable, "Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}

#ifdef CONFIG_MTK_FLASHLIGHT_PT
	if (pt_arg_verify(low_vol, low_bat, over_cur) ||
			strict < 0 || strict > 1) {
		PR_ERR_IF(dumpEnable, "Error arguments\n");
		ret = -1;
		goto unlock;
	}
	PR_DEBUG_IF(dumpEnable, "PT status (%d, %d, %d) with strict(%d)\n",
			low_vol, low_bat, over_cur, strict);

	/* call callback function */
	pt_strict = strict;
	#ifndef VENDOR_EDIT
	/* Qiao.Hu@BSP.BaseDrv.CHG.Basic, 2018/05/25, Add for pmic invalid at low battery voltage */
	pt_low_vol_callback(low_vol);
	pt_low_bat_callback(low_bat);
	#endif
	pt_oc_callback(over_cur);
#endif

	ret = size;
unlock:
	return ret;
}
static DEVICE_ATTR_RW(flashlight_pt);

/* charger status sysfs */
static ssize_t flashlight_charger_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	struct flashlight_dev *fdev;
	char status[FLASHLIGHT_CHARGER_STATUS_BUF_SIZE];
	char status_tmp[FLASHLIGHT_CHARGER_STATUS_TMPBUF_SIZE];

	PR_DEBUG_IF(dumpEnable, "Charger status show\n");

	memset(status, '\0', FLASHLIGHT_CHARGER_STATUS_BUF_SIZE);

	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)
			continue;

		flashlight_update_charger_status(fdev);
		snprintf(status_tmp,
				FLASHLIGHT_CHARGER_STATUS_TMPBUF_SIZE,
				"%d %d %d %d\n", fdev->dev_id.type,
				fdev->dev_id.ct, fdev->dev_id.part,
				fdev->charger_status);
		strncat(status, status_tmp,
				FLASHLIGHT_CHARGER_STATUS_TMPBUF_SIZE);
	}
	mutex_unlock(&fl_mutex);

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [CT] [PART] [CHARGER_STATUS]\n%s\n", status);
}

static ssize_t flashlight_charger_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct flashlight_dev *fdev;
	struct flashlight_arg fl_arg;
	int charger_status_tmp = 0;
	s32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	PR_DEBUG_IF(dumpEnable, "Charger status store\n");

	memset(&fl_arg, 0, sizeof(struct flashlight_arg));

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtos32(token, 10, &num);
		if (ret) {
			PR_ERR_IF(dumpEnable, "Error arguments\n");
			goto unlock;
		}

		if (count == FLASHLIGHT_CHARGER_TYPE)
			fl_arg.type = (int)num;
		else if (count == FLASHLIGHT_CHARGER_CT)
			fl_arg.ct = (int)num;
		else if (count == FLASHLIGHT_CHARGER_PART)
			fl_arg.part = (int)num;
		else if (count == FLASHLIGHT_CHARGER_STATUS)
			charger_status_tmp = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != FLASHLIGHT_CHARGER_NUM) {
		PR_ERR_IF(dumpEnable, "Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}
	if (flashlight_verify_index(fl_arg.type, fl_arg.ct, fl_arg.part)) {
		PR_ERR_IF(dumpEnable, "Error arguments\n");
		ret = -1;
		goto unlock;
	}
	if (charger_status_tmp < FLASHLIGHT_CHARGER_NOT_READY ||
			charger_status_tmp > FLASHLIGHT_CHARGER_READY) {
		PR_ERR_IF(dumpEnable, "Error arguments charger status(%d)\n",
				charger_status_tmp);
		ret = -1;
		goto unlock;
	}

	PR_DEBUG_IF(dumpEnable, "(%d, %d, %d), (%d)\n", fl_arg.type, fl_arg.ct, fl_arg.part,
			charger_status_tmp);

	/* store charger status */
	mutex_lock(&fl_mutex);
	fdev = flashlight_find_dev_by_full_index(
			fl_arg.type, fl_arg.ct, fl_arg.part);
	mutex_unlock(&fl_mutex);
	if (!fdev) {
		PR_INFO_IF(dumpEnable, "Find no flashlight device\n");
		ret = -1;
		goto unlock;
	}

	fdev->charger_status = charger_status_tmp;

	ret = size;
unlock:
	return ret;
}
static DEVICE_ATTR_RW(flashlight_charger);

/* flashlight capability sysfs */
static ssize_t flashlight_capability_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	struct flashlight_dev *fdev;
	struct flashlight_dev_arg fl_dev_arg;

	/* flashlight capability */
	int hw_timeout;
	int max_duty;
	int max_torch_duty;
	char capability[FLASHLIGHT_CAPABILITY_BUF_SIZE];
	char capability_tmp[FLASHLIGHT_CAPABILITY_TMPBUF_SIZE];

	PR_DEBUG_IF(dumpEnable, "Capability show\n");

	memset(capability, '\0', FLASHLIGHT_CAPABILITY_BUF_SIZE);

	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)
			continue;

		fl_dev_arg.channel = fdev->dev_id.channel;

		fl_dev_arg.arg = -1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_GET_HW_TIMEOUT,
				(unsigned long)&fl_dev_arg);
		hw_timeout = fl_dev_arg.arg;

		fl_dev_arg.arg = -1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_GET_DUTY_NUMBER,
				(unsigned long)&fl_dev_arg);
		max_duty = fl_dev_arg.arg - 1;

		fl_dev_arg.arg = -1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_GET_MAX_TORCH_DUTY,
				(unsigned long)&fl_dev_arg);
		max_torch_duty = fl_dev_arg.arg;

		snprintf(capability_tmp, FLASHLIGHT_CAPABILITY_TMPBUF_SIZE,
				"%d %d %d %s %d %d %d %d %d\n",
				fdev->dev_id.type, fdev->dev_id.ct,
				fdev->dev_id.part, fdev->dev_id.name,
				fdev->dev_id.channel, fdev->dev_id.decouple,
				hw_timeout, max_duty, max_torch_duty);
		strncat(capability, capability_tmp,
				FLASHLIGHT_CAPABILITY_TMPBUF_SIZE);
	}
	mutex_unlock(&fl_mutex);

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [CT] [PART] [DEVICE] [CHANNEL] [DECOUPLE] [HW TIMEOUT] [MAX DUTY] [MAX TORCH DUTY]\n%s\n",
			capability);
}
static DEVICE_ATTR_RO(flashlight_capability);


#ifdef ODM_WT_EDIT
#ifdef OPPO_FLASHLIGHT_TEST
/* Yang.guo@Camera.Driver, 2020/03/25, add for [wingtech ATO factory app camera] */
/******************************************************************************/
/********************************flashlight test*******************************/
/******************************************************************************/
#define TAG_NAME "[flashligh-core]"
#define PK_DBG_FUNC(fmt, arg...)    PR_DEBUG_IF(dumpEnable, TAG_NAME "%s: " fmt, __func__, ##arg)
//===============================================================
extern int high_cct_led_strobe_enable_part1(void);
extern int high_cct_led_strobe_setduty_part1(int duty);
extern int high_cct_led_strobe_on_part1(int onoff);
extern int low_cct_led_strobe_enable_part1(void);
extern int low_cct_led_strobe_setduty_part1(int duty);
extern int low_cct_led_strobe_on_part1(int onoff);
//-------------------------------------------------
static int flashduty1 = 0;
static int flashduty2 = 0;
//25mA,50mA,75mA,100mA,125mA,150mA
//(num*2.91 + 2.55)
int flashduty_part1[] = {1, 2, 3, 4, 5, 6};

int high_cct_led_strobe_enable(void){
	high_cct_led_strobe_enable_part1();

	return 0;
}

int high_cct_led_strobe_setduty(int duty){
	high_cct_led_strobe_setduty_part1(flashduty_part1[duty]);

	return 0;
}

int high_cct_led_strobe_on(int onoff){
	high_cct_led_strobe_on_part1(onoff);

	return 0;
}

int low_cct_led_strobe_enable(void){
	low_cct_led_strobe_enable_part1();

	return 0;
}

int low_cct_led_strobe_setduty(int duty){
	low_cct_led_strobe_setduty_part1(flashduty_part1[duty]);

	return 0;
}

int low_cct_led_strobe_on(int onoff){
	low_cct_led_strobe_on_part1(onoff);

	return 0;
}

static ssize_t show_flashduty1(struct device *dev, struct device_attribute *attr, char *buf)
{
	PK_DBG_FUNC("[LED]get backlight duty value is:%d\n", flashduty1);
	return sprintf(buf, "%d\n", flashduty1);
}

static ssize_t store_flashduty1(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	char *pvalue=NULL;

	PK_DBG_FUNC("Enter!\n");
	flashduty1 = simple_strtol(buf, &pvalue, 0);
	PK_DBG_FUNC("flashduty1= %d\n", flashduty1);

	if(flashduty1 > 5)
		flashduty1 = 5;

	//open leds resource
	high_cct_led_strobe_enable();

	if(flashduty1 >= 0){ //on
		high_cct_led_strobe_setduty(flashduty1);
		high_cct_led_strobe_on(1);
	} else { // < 0 //off
		high_cct_led_strobe_on(0);
	}

	PK_DBG_FUNC("Exit!\n");
	return count;
}
static DEVICE_ATTR(flash1, 0664, show_flashduty1, store_flashduty1);

static ssize_t show_flashduty2(struct device *dev, struct device_attribute *attr, char *buf)
{
	PK_DBG_FUNC("[LED]get backlight duty value is %d\n", flashduty2);
	return sprintf(buf, "%d\n", flashduty2);
}

static ssize_t store_flashduty2(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	char *pvalue=NULL;

	PK_DBG_FUNC("Enter!\n");
	flashduty2= simple_strtol(buf, &pvalue, 0);
	PK_DBG_FUNC("flashduty2= %d\n", flashduty2);

	if(flashduty2>5)
		flashduty2=5;

	//open leds resource
	low_cct_led_strobe_enable();

	if(flashduty2>=0){ //on
		low_cct_led_strobe_setduty(flashduty2);
		low_cct_led_strobe_on(1);
	} else { // < 0 //off
		low_cct_led_strobe_on(0);
	}

	PK_DBG_FUNC("Exit!\n");
	return count;
}

static DEVICE_ATTR(flash2, 0664, show_flashduty2, store_flashduty2);

//-------------flash duty test-------------
static int flashduty1_test=-1;
static int flashduty2_test=-1;
static int openFlash_flag_test=0;
static ssize_t show_flashduty_test(struct device *dev, struct device_attribute *attr, char *buf)
{
	PK_DBG_FUNC("[LED]get backlight duty value is %d\n", flashduty1_test);
	return sprintf(buf, "%d\n", flashduty1_test);
}

static ssize_t store_flashduty_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct flashlight_dev *fdev;
	struct flashlight_dev_arg fl_dev_arg;
	char *pvalue=NULL;

	PK_DBG_FUNC("Enter!\n");

	if (buf != NULL && count != 0) {
		flashduty1_test = simple_strtol(buf, &pvalue, 0);
		PK_DBG_FUNC("flashduty1_test= %d\n", flashduty1_test);

		if (*pvalue ) {
			flashduty2_test = simple_strtol((pvalue + 1), NULL, 0);
			PK_DBG_FUNC("flashduty2_test= %d\n", flashduty2_test);
		}
	}

	if((flashduty1_test >= 0)||(flashduty2_test >= 0))
		PK_DBG_FUNC("OnOff= %d\n", 1);
	else
		PK_DBG_FUNC("OnOff= %d\n", 0);

	if(flashduty1_test >= 26)
		flashduty1_test = 26;
	if(flashduty2_test >= 26)
		flashduty2_test = 26;

	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)return -EBUSY;
		fdev->ops->flashlight_open();
		fdev->ops->flashlight_set_driver(1);

		PK_DBG_FUNC("opened flashlight_open !!!\n");
		if((flashduty1_test < 0)&&(flashduty2_test < 0))
			goto out;
		else
			goto work3;
	}

work3:
	if((flashduty1_test < 0) && (flashduty2_test < 0)){
		PK_DBG_FUNC("flashduty1_test=%d,flashduty2_test=%d, %d\n", flashduty1_test,flashduty2_test,__LINE__);
		//close flash pF1 & close flash pF2
	}else if((flashduty1_test >= 0) && (flashduty2_test < 0)){
		PK_DBG_FUNC("flashduty1_test=%d,flashduty2_test=%d, %d\n", flashduty1_test,flashduty2_test,__LINE__);
		fl_dev_arg.channel = 0;
		fl_dev_arg.arg = flashduty1_test;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1000;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);

		fl_dev_arg.channel = 1;
		fl_dev_arg.arg = flashduty2_test;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1000;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 0;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);

	}else if((flashduty2_test >= 0) && (flashduty1_test < 0)){
		PK_DBG_FUNC("flashduty1_test=%d,flashduty2_test=%d, %d\n", flashduty1_test,flashduty2_test,__LINE__);
		fl_dev_arg.channel = 0;
		fl_dev_arg.arg = flashduty1_test;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1000;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 0;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);

		fl_dev_arg.channel = 1;
		fl_dev_arg.arg = flashduty2_test;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1000;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);
	}else{ //(flashduty1_test >= 0) && (flashduty2_test >= 0)
		PK_DBG_FUNC("flashduty1_test=%d,flashduty2_test=%d, %d\n", flashduty1_test,flashduty2_test,__LINE__);
		fl_dev_arg.channel = 0;
		fl_dev_arg.arg = flashduty1_test;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1000;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);

		fl_dev_arg.channel = 1;
		fl_dev_arg.arg = flashduty2_test;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1000;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_TIME_OUT_TIME_MS, (unsigned long)&fl_dev_arg);
		fl_dev_arg.arg = 1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);
	}

	msleep(500);

	openFlash_flag_test = 0;
	fl_dev_arg.channel = 0;
	fl_dev_arg.arg = -1;
	fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
	fl_dev_arg.arg = 0;
	fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);

	fl_dev_arg.channel = 1;
	fl_dev_arg.arg = -1;
	fdev->ops->flashlight_ioctl(FLASH_IOC_SET_DUTY, (unsigned long)&fl_dev_arg);
	fl_dev_arg.arg = 0;
	fdev->ops->flashlight_ioctl(FLASH_IOC_SET_ONOFF, (unsigned long)&fl_dev_arg);

	fdev->ops->flashlight_set_driver(0);
	fdev->ops->flashlight_release();
	PK_DBG_FUNC("Exit1!\n");
	return count;

out:
	openFlash_flag_test = 0;
	fdev->ops->flashlight_set_driver(0);
	fdev->ops->flashlight_release();
	PK_DBG_FUNC("Exit2!\n");
	return -1;
}

static DEVICE_ATTR(flash3, 0664, show_flashduty_test, store_flashduty_test);

/******************************************************************************/
#endif
#endif

/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static struct class *flashlight_class;
static struct device *flashlight_device;
static dev_t flashlight_devno;
static struct cdev *flashlight_cdev;

#ifdef ODM_WT_EDIT
#ifdef OPPO_FLASHLIGHT_TEST
/* Yang.guo@Camera.Driver, 2020/03/25, add for [wingtech ATO factory app camera] */
int probe_flag = 0;
static struct class *oppo_flashlight_class;
static struct device *oppo_flashlight_device;
static dev_t oppo_flashlight_devno;
#define ALLOC_DEVNO
#define OPPO_FLASHLIGHT_DEVNAME   "kd_camera_flashlight"
static struct cdev *oppo_flashlight_cdev;

static const struct file_operations oppo_flashlight_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = NULL,
	.open = NULL,
	.release = NULL,
#ifdef CONFIG_COMPAT
	.compat_ioctl = NULL,
#endif
};
#endif
#endif



/* flashlight current sysfs */
static ssize_t flashlight_current_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	struct flashlight_dev *fdev;
	struct flashlight_dev_arg fl_dev_arg;
	int i;

	/* flashlight current */
	int duty_num = 0;
	char duty_current_tmp[FLASHLIGHT_DUTY_CURRENT_TMPBUF_SIZE];
	char duty_current[FLASHLIGHT_DUTY_CURRENT_BUF_SIZE];

	PR_DEBUG_IF(dumpEnable, "Current show\n");

	memset(duty_current, '\0', FLASHLIGHT_DUTY_CURRENT_BUF_SIZE);

	mutex_lock(&fl_mutex);
	fdev = flashlight_find_dev_by_full_index(duty_current_arg.type,
			duty_current_arg.ct, duty_current_arg.part);
	mutex_unlock(&fl_mutex);

	if (fdev && fdev->ops) {
		fl_dev_arg.channel = fdev->dev_id.channel;

		fl_dev_arg.arg = -1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_GET_DUTY_NUMBER,
				(unsigned long)&fl_dev_arg);
		duty_num = fl_dev_arg.arg;

		snprintf(duty_current, FLASHLIGHT_DUTY_CURRENT_BUF_SIZE,
				"%d %d %d %d ", fdev->dev_id.type,
				fdev->dev_id.ct, fdev->dev_id.part, duty_num);

		for (i = 0; i < duty_num; i++) {
			fl_dev_arg.arg = i;
			if (fdev->ops->flashlight_ioctl(
						FLASH_IOC_GET_DUTY_CURRENT,
						(unsigned long)&fl_dev_arg))
				break;
			snprintf(duty_current_tmp,
					FLASHLIGHT_DUTY_CURRENT_TMPBUF_SIZE,
					"%d,", fl_dev_arg.arg);
			strncat(duty_current, duty_current_tmp,
					FLASHLIGHT_DUTY_CURRENT_TMPBUF_SIZE);
		}
		duty_current[strlen(duty_current) - 1] = '\0';
	} else {
		snprintf(duty_current, FLASHLIGHT_DUTY_CURRENT_BUF_SIZE,
				"%d %d %d %d ", duty_current_arg.type,
				duty_current_arg.ct, duty_current_arg.part,
				duty_num);
	}

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [CT] [PART] [DUTY NUM] [DUTY CURRENT]\n%s\n",
			duty_current);
}

static ssize_t flashlight_current_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct flashlight_arg fl_arg;
	s32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	PR_DEBUG_IF(dumpEnable, "Current store\n");

	memset(&fl_arg, 0, sizeof(struct flashlight_arg));

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtos32(token, 10, &num);
		if (ret) {
			PR_ERR_IF(dumpEnable, "Error arguments\n");
			goto unlock;
		}

		if (count == FLASHLIGHT_CURRENT_TYPE)
			fl_arg.type = (int)num;
		else if (count == FLASHLIGHT_CURRENT_CT)
			fl_arg.ct = (int)num;
		else if (count == FLASHLIGHT_CURRENT_PART)
			fl_arg.part = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != FLASHLIGHT_CURRENT_NUM) {
		PR_ERR_IF(dumpEnable, "Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}
	if (flashlight_verify_index(fl_arg.type, fl_arg.ct, fl_arg.part)) {
		PR_ERR_IF(dumpEnable, "Error arguments\n");
		ret = -1;
		goto unlock;
	}

	PR_DEBUG_IF(dumpEnable, "(%d, %d, %d)\n",
			fl_arg.type, fl_arg.ct, fl_arg.part);

	/* store duty current */
	duty_current_arg = fl_arg;

	ret = size;
unlock:
	return ret;
}
static DEVICE_ATTR_RW(flashlight_current);

#ifndef VENDOR_EDIT
/* Qiao.Hu@BSP.BaseDrv.CHG.Basic, 2018/05/25, Add for pmic invalid at low battery voltage */
/* flashlight fault sysfs */
static ssize_t flashlight_fault_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	struct flashlight_dev *fdev;
	struct flashlight_dev_arg fl_dev_arg;

	/* flashlight capability */
	int fault_flag1;
	int fault_flag2;
	char fault[FLASHLIGHT_FAULT_BUF_SIZE];
	char fault_tmp[FLASHLIGHT_FAULT_TMPBUF_SIZE];

	PR_DEBUG_IF(dumpEnable, "Fault show\n");

	memset(fault, '\0', FLASHLIGHT_FAULT_BUF_SIZE);

	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)
			continue;

		fl_dev_arg.channel = fdev->dev_id.channel;

		fl_dev_arg.arg = -1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_GET_HW_FAULT,
				(unsigned long)&fl_dev_arg);
		fault_flag1 = fl_dev_arg.arg;

		fl_dev_arg.arg = -1;
		fdev->ops->flashlight_ioctl(FLASH_IOC_GET_HW_FAULT2,
				(unsigned long)&fl_dev_arg);
		fault_flag2 = fl_dev_arg.arg;

		snprintf(fault_tmp, FLASHLIGHT_FAULT_TMPBUF_SIZE,
				"%d %d %d %s %d %d %d %d\n",
				fdev->dev_id.type, fdev->dev_id.ct,
				fdev->dev_id.part, fdev->dev_id.name,
				fdev->dev_id.channel, fdev->dev_id.decouple,
				fault_flag1, fault_flag2);
		strncat(fault, fault_tmp,
				FLASHLIGHT_FAULT_TMPBUF_SIZE);
	}
	mutex_unlock(&fl_mutex);

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [CT] [PART] [DEVICE] [CHANNEL] [DECOUPLE] [FAULT FLAG1] [FAULT FLAG2]\n%s\n",
			fault);
}
static DEVICE_ATTR_RO(flashlight_fault);

/* sw disable sysfs */
static ssize_t flashlight_sw_disable_show(
		struct device *dev, struct device_attribute *attr, char *buf)
{
	struct flashlight_dev *fdev;
	char status[FLASHLIGHT_SW_DISABLE_STATUS_BUF_SIZE];
	char status_tmp[FLASHLIGHT_SW_DISABLE_STATUS_TMPBUF_SIZE];

	PR_DEBUG_IF(dumpEnable, "Charger status show\n");

	memset(status, '\0', FLASHLIGHT_SW_DISABLE_STATUS_BUF_SIZE);

	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)
			continue;

		snprintf(status_tmp,
				FLASHLIGHT_SW_DISABLE_STATUS_TMPBUF_SIZE,
				"%d %d\n", fdev->dev_id.type,
				fdev->sw_disable_status);
		strncat(status, status_tmp,
				FLASHLIGHT_SW_DISABLE_STATUS_TMPBUF_SIZE);
	}
	mutex_unlock(&fl_mutex);

	return scnprintf(buf, PAGE_SIZE,
			"[TYPE] [SW_DISABLE_STATUS]\n%s\n", status);
}

static ssize_t flashlight_sw_disable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct flashlight_dev *fdev;
	struct flashlight_arg fl_arg;
	int sw_disable_status_tmp = 0;
	s32 num;
	int count = 0;
	char delim[] = " ";
	char *token, *cur = (char *)buf;
	int ret;

	PR_DEBUG_IF(dumpEnable, "Sw disable store\n");

	memset(&fl_arg, 0, sizeof(struct flashlight_arg));

	while (cur) {
		token = strsep(&cur, delim);
		ret = kstrtos32(token, 10, &num);
		if (ret) {
			PR_INFO_IF(dumpEnable, "Error arguments\n");
			goto unlock;
		}

		if (count == FLASHLIGHT_SW_DISABLE_TYPE)
			fl_arg.type = (int)num;
		else if (count == FLASHLIGHT_SW_DISABLE_STATUS)
			sw_disable_status_tmp = (int)num;
		else {
			count++;
			break;
		}

		count++;
	}

	/* verify data */
	if (count != FLASHLIGHT_SW_DISABLE_NUM) {
		PR_INFO_IF(dumpEnable, "Error argument number: (%d)\n", count);
		ret = -1;
		goto unlock;
	}
	if (sw_disable_status_tmp < FLASHLIGHT_SW_DISABLE_OFF ||
			sw_disable_status_tmp > FLASHLIGHT_SW_DISABLE_ON) {
		PR_INFO_IF(dumpEnable, "Error arguments sw disable status(%d)\n",
				sw_disable_status_tmp);
		ret = -1;
		goto unlock;
	}

	PR_DEBUG_IF(dumpEnable, "(%d), (%d)\n", fl_arg.type, sw_disable_status_tmp);

	/* store sw_disable status */
	mutex_lock(&fl_mutex);
	list_for_each_entry(fdev, &flashlight_list, node) {
		if (!fdev->ops)
			continue;
		if (fl_arg.type == fdev->dev_id.type) {
			if (sw_disable_status_tmp == FLASHLIGHT_SW_DISABLE_ON)
				fl_enable(fdev, 0);

			fdev->sw_disable_status = sw_disable_status_tmp;
		}
	}
	mutex_unlock(&fl_mutex);
	ret = size;
unlock:
	return ret;
}
static DEVICE_ATTR_RW(flashlight_sw_disable);
#endif
/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static struct class *flashlight_class;
static struct device *flashlight_device;
static dev_t flashlight_devno;
static struct cdev *flashlight_cdev;

static int fl_init(void)
{
	return 0;
}

static int fl_uninit(void)
{
	struct flashlight_dev *fdev, *n;

	mutex_lock(&fl_mutex);
	list_for_each_entry_safe(fdev, n, &flashlight_list, node) {
		/* uninit device */
		if (fdev->ops) {
			fdev->ops->flashlight_open();
			fdev->ops->flashlight_set_driver(1);
			fl_enable(fdev, 0);
			fdev->ops->flashlight_set_driver(0);
			fdev->ops->flashlight_release();
		}

		/* clear node and free memory */
		list_del(&fdev->node);
		kfree(fdev);
	}
	mutex_unlock(&fl_mutex);

	return 0;
}

static int flashlight_probe(struct platform_device *dev)
{
#ifdef FLASHLIGHT_BRIGHTNESS_ADD
	int flashlight_value= 0;
#endif
	PR_DEBUG_IF(dumpEnable, "Probe start\n");

	/* allocate char device number */
	if (alloc_chrdev_region(&flashlight_devno, 0, 1, FLASHLIGHT_DEVNAME)) {
		PR_ERR_IF(dumpEnable, "Failed to allocate char device region\n");
		goto err_allocate_chrdev;
	}
	PR_DEBUG_IF(dumpEnable, "Allocate major number and minor number: (%d, %d)\n",
			MAJOR(flashlight_devno),
			MINOR(flashlight_devno));

	/* allocate char device */
	flashlight_cdev = cdev_alloc();
	if (!flashlight_cdev) {
		PR_ERR_IF(dumpEnable, "Failed to allcoate cdev\n");
		goto err_allocate_cdev;
	}
	flashlight_cdev->ops = &flashlight_fops;
	flashlight_cdev->owner = THIS_MODULE;

	/* add char device to the system */
	if (cdev_add(flashlight_cdev, flashlight_devno, 1)) {
		PR_ERR_IF(dumpEnable, "Failed to add cdev\n");
		goto err_add_cdev;
	}

	/* create class */
	flashlight_class = class_create(THIS_MODULE, FLASHLIGHT_CORE);
	if (IS_ERR(flashlight_class)) {
		PR_ERR_IF(dumpEnable, "Failed to create class (%d)\n",
				(int)PTR_ERR(flashlight_class));
		goto err_create_class;
	}

	/* create device */
	flashlight_device =
	    device_create(flashlight_class, NULL, flashlight_devno,
				NULL, FLASHLIGHT_DEVNAME);
	if (!flashlight_device) {
		PR_ERR_IF(dumpEnable, "Failed to create device\n");
		goto err_create_device;
	}

	/* create device file */
	if (device_create_file(flashlight_device,
				&dev_attr_flashlight_strobe)) {
		PR_ERR_IF(dumpEnable, "Failed to create device file(strobe)\n");
		goto err_create_strobe_device_file;
	}
	if (device_create_file(flashlight_device,
				&dev_attr_flashlight_pt)) {
		PR_ERR_IF(dumpEnable, "Failed to create device file(pt)\n");
		goto err_create_pt_device_file;
	}
	if (device_create_file(flashlight_device,
				&dev_attr_flashlight_charger)) {
		PR_ERR_IF(dumpEnable, "Failed to create device file(charger)\n");
		goto err_create_charger_device_file;
	}
	if (device_create_file(flashlight_device,
				&dev_attr_flashlight_capability)) {
		PR_ERR_IF(dumpEnable, "Failed to create device file(capability)\n");
		goto err_create_capability_device_file;
	}
	if (device_create_file(flashlight_device,
				&dev_attr_flashlight_current)) {
		PR_ERR_IF(dumpEnable, "Failed to create device file(current)\n");
		goto err_create_current_device_file;
	}
	#ifndef VENDOR_EDIT
	/* Qiao.Hu@BSP.BaseDrv.CHG.Basic, 2018/05/25, Add for pmic invalid at low battery voltage */
	if (device_create_file(flashlight_device, &dev_attr_flashlight_fault)) {
		PR_INFO_IF(dumpEnable, "Failed to create device file(fault)\n");
		goto err_create_fault_device_file;
	}
	if (device_create_file(flashlight_device,
				&dev_attr_flashlight_sw_disable)) {
		PR_INFO_IF(dumpEnable, "Failed to create device file(sw_disable)\n");
		goto err_create_sw_disable_device_file;
	}
       #endif
       #ifdef VENDOR_EDIT
       /*Henry.Chang@Camera.driver, 2018.12.18  add for flashlight */
       flash_proc_init();
       #endif /*VENDOR_EDIT*/

#ifdef ODM_WT_EDIT
#ifdef OPPO_FLASHLIGHT_TEST
/* Yang.guo@Camera.Driver, 2020/03/25, add for [wingtech ATO factory app camera] */
    /******************************************************/
    // echo 04 > /sys/class/flashlightdrv/kd_camera_flashlight/flash1   //open flash led1
    // echo -1 > /sys/class/flashlightdrv/kd_camera_flashlight/flash1   //close flash led1
    // echo 04 > /sys/class/flashlightdrv/kd_camera_flashlight/flash2   //open flash  led2
    // echo -1 > /sys/class/flashlightdrv/kd_camera_flashlight/flash1   //close flash led2
    //echo 26 26 > /sys/class/flashlightdrv/kd_camera_flashlight/flash3 //open flash led1 led2
    //echo -1 -1 > /sys/class/flashlightdrv/kd_camera_flashlight/flash3 //close flash led1 led2
    if (alloc_chrdev_region(&oppo_flashlight_devno, 0, 1, OPPO_FLASHLIGHT_DEVNAME)) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe] alloc_chrdev_region fail~");
    } else {
        PR_ERR_IF(dumpEnable, "[flashlight_probe] major: %d, minor: %d ~", MAJOR(oppo_flashlight_devno),
             MINOR(oppo_flashlight_devno));
    }

    oppo_flashlight_cdev = cdev_alloc();
    if (!oppo_flashlight_cdev) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe] Failed to allcoate cdev\n");
    }

    oppo_flashlight_cdev->ops = &oppo_flashlight_fops;
    oppo_flashlight_cdev->owner = THIS_MODULE;
    if (cdev_add(oppo_flashlight_cdev, oppo_flashlight_devno, 1)) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe] cdev_add fail ~" );
    }

    oppo_flashlight_class = class_create(THIS_MODULE, "flashlightdrv");
    if (IS_ERR(oppo_flashlight_class)) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe] Unable to create class, err = %d ~",
             (int)PTR_ERR(oppo_flashlight_class));
        return  -1 ;
    }

    oppo_flashlight_device =
        device_create(oppo_flashlight_class, NULL, oppo_flashlight_devno, NULL, OPPO_FLASHLIGHT_DEVNAME);
    if (NULL == oppo_flashlight_device) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe] device_create fail ~");
       }

    if (device_create_file(oppo_flashlight_device, &dev_attr_flash1)) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe]device_create_file flash1 fail!\n");
    }

    if (device_create_file(oppo_flashlight_device, &dev_attr_flash2)) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe]device_create_file flash2 fail!\n");
    }

    if (device_create_file(oppo_flashlight_device, &dev_attr_flash3)) {
        PR_ERR_IF(dumpEnable, "[flashlight_probe]device_create_file flash3 fail!\n");
    }
    /*****************************************************************************/
#endif
#endif

#ifdef FLASHLIGHT_BRIGHTNESS_ADD
	/*add kobject */
	flashlight_value = kobject_init_and_add(&flashlight_back_brightness_kobj, &flashlight_back_brightness_ktype, &(flashlight_device->kobj), F_BACK_NAME_SYSFS);

	if (flashlight_value < 0) {
		PR_INFO_IF(dumpEnable, "fail to add flashlight brightness kobject\n");
	return flashlight_value;
	}
#endif

	/* init flashlight */
	fl_init();

	PR_DEBUG_IF(dumpEnable, "Probe done\n");

	return 0;
#ifndef VENDOR_EDIT
/* Qiao.Hu@BSP.BaseDrv.CHG.Basic, 2018/05/25, Add for pmic invalid at low battery voltage */
err_create_sw_disable_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_sw_disable);
err_create_fault_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_fault);
#endif
err_create_current_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_capability);
err_create_capability_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_charger);
err_create_charger_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_pt);
err_create_pt_device_file:
	device_remove_file(flashlight_device, &dev_attr_flashlight_strobe);
err_create_strobe_device_file:
	device_destroy(flashlight_class, flashlight_devno);
err_create_device:
	class_destroy(flashlight_class);
err_create_class:
err_add_cdev:
	cdev_del(flashlight_cdev);
err_allocate_cdev:
	unregister_chrdev_region(flashlight_devno, 1);
err_allocate_chrdev:
	return -1;
}

static int flashlight_remove(struct platform_device *dev)
{
	fl_uninit();

	/* remove device file */
	#ifndef VENDOR_EDIT
	/* Qiao.Hu@BSP.BaseDrv.CHG.Basic, 2018/05/25, Add for pmic invalid at low battery voltage */
	device_remove_file(flashlight_device, &dev_attr_flashlight_sw_disable);
	device_remove_file(flashlight_device, &dev_attr_flashlight_fault);
	#endif
	device_remove_file(flashlight_device, &dev_attr_flashlight_current);
	device_remove_file(flashlight_device, &dev_attr_flashlight_capability);
	device_remove_file(flashlight_device, &dev_attr_flashlight_charger);
	device_remove_file(flashlight_device, &dev_attr_flashlight_pt);
	device_remove_file(flashlight_device, &dev_attr_flashlight_strobe);
	/* remove device */
	device_destroy(flashlight_class, flashlight_devno);
	/* remove class */
	class_destroy(flashlight_class);
	/* remove char device */
	cdev_del(flashlight_cdev);
	/* unregister char device number */
	unregister_chrdev_region(flashlight_devno, 1);

	return 0;
}

static void flashlight_shutdown(struct platform_device *dev)
{
	fl_uninit();
}

#ifdef CONFIG_OF
static const struct of_device_id flashlight_of_match[] = {
	{.compatible = "mediatek,flashlight_core"},
	{},
};
MODULE_DEVICE_TABLE(of, flashlight_of_match);
#else
static struct platform_device flashlight_platform_device[] = {
	{
		.name = FLASHLIGHT_DEVNAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, flashlight_platform_device);
#endif

static struct platform_driver flashlight_platform_driver = {
	.probe = flashlight_probe,
	.remove = flashlight_remove,
	.shutdown = flashlight_shutdown,
	.driver = {
		   .name = FLASHLIGHT_DEVNAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = flashlight_of_match,
#endif
	},
};

static int __init flashlight_init(void)
{
	int ret;

	PR_DEBUG_IF(dumpEnable, "Init start\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&flashlight_platform_device);
	if (ret) {
		PR_ERR_IF(dumpEnable, "Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&flashlight_platform_driver);
	if (ret) {
		PR_ERR_IF(dumpEnable, "Failed to register platform driver\n");
		return ret;
	}

#ifdef CONFIG_MTK_FLASHLIGHT_PT
	#ifndef VENDOR_EDIT
	/* Qiao.Hu@BSP.BaseDrv.CHG.Basic, 2018/05/25, Add for pmic invalid at low battery voltage */
	register_low_battery_notify(
			&pt_low_vol_callback, LOW_BATTERY_PRIO_FLASHLIGHT);
	register_battery_percent_notify(
			&pt_low_bat_callback, BATTERY_PERCENT_PRIO_FLASHLIGHT);
	#endif
	register_battery_oc_notify(
			&pt_oc_callback, BATTERY_OC_PRIO_FLASHLIGHT);
#endif

	PR_DEBUG_IF(dumpEnable, "Init done\n");

	return 0;
}

static void __exit flashlight_exit(void)
{
	PR_DEBUG_IF(dumpEnable, "Exit start\n");

	platform_driver_unregister(&flashlight_platform_driver);

	PR_DEBUG_IF(dumpEnable, "Exit done\n");
}

module_init(flashlight_init);
module_exit(flashlight_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight Core Driver");

