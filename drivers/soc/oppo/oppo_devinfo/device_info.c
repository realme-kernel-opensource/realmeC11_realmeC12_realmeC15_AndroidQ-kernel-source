/**
 * Copyright 2008-2013 OPPO Mobile Comm Corp., Ltd, All rights reserved.
 * VENDOR_EDIT:
 * FileName:devinfo.c
 * ModuleName:devinfo
 * Author: wangjc
 * Create Date: 2013-10-23
 * Description:add interface to get device information.
 * History:
   <version >  <time>  <author>  <desc>
   1.0		2013-10-23	wangjc	init
   2.0      2015-04-13  hantong modify as platform device  to support diffrent configure in dts
*/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <soc/oppo/device_info.h>
#include <soc/oppo/oppo_project.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include "../../../../fs/proc/internal.h"
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#ifdef ODM_WT_EDIT
/*xu.chen@ODM_WT.BSP.Kernel.Boot, 2020/06/12, Add for S98639 subID match*/
#include <linux/hardware_info.h>
#endif /* ODM_WT_EDIT */

#define DEVINFO_NAME "devinfo"
#define INFO_BUF_LEN 64
/**for definfo log**/
#define log_fmt(fmt) "[line:%d][module:%s][%s] " fmt

#define DEVINFO_ERR(a,arg...) \
 do { \
		printk(KERN_NOTICE log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg); \
  } while (0)

#define DEVINFO_MSG(a, arg...) \
 do { \
		printk(KERN_INFO log_fmt(a),__LINE__,DEVINFO_NAME,__func__,##arg); \
  } while (0)

/**definfo log end**/
extern pid_t fork_pid_child;
extern pid_t fork_pid_father;
extern int happend_times;

static struct of_device_id devinfo_id[] = {
	{.compatible = "oppo, device_info",},
	{},
};

struct devinfo_data {
	struct platform_device *devinfo;
	struct pinctrl *pinctrl;
	int hw_id1_gpio;
	int hw_id2_gpio;
	int hw_id3_gpio;
	int sub_hw_id1;
	int sub_hw_id2;
	int main_hw_id5;
	int main_hw_id6;
	struct pinctrl_state *hw_sub_id_sleep;
	struct pinctrl_state *hw_sub_id_active;
	struct pinctrl_state *hw_main_id5_active;
	struct pinctrl_state *hw_main_id6_active;
	int ant_select_gpio;
	struct manufacture_info sub_mainboard_info;
};

static struct devinfo_data *dev_info;
static struct proc_dir_entry *parent = NULL;

static void *device_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter = 0;
	if ( *pos == 0 ) {
		return &counter;
	}else{
		*pos = 0;
		return NULL;
	}
}

static void *device_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}

static void device_seq_stop(struct seq_file *s, void *v)
{
	return;
}

static int device_seq_show(struct seq_file *s, void *v)
{
        struct proc_dir_entry *pde = s->private;
        struct manufacture_info *info = pde->data;
        if (info) {
                seq_printf(s, "Device version:\t\t%s\nDevice manufacture:\t\t%s\n",
                         info->version,        info->manufacture);
                if(info->fw_path)
                        seq_printf(s, "Device fw_path:\t\t%s\n",
                                info->fw_path);
        }
        return 0;
}

static struct seq_operations device_seq_ops = {
	.start = device_seq_start,
	.next = device_seq_next,
	.stop = device_seq_stop,
	.show = device_seq_show
};

static int device_proc_open(struct inode *inode,struct file *file)
{
	int ret = seq_open(file,&device_seq_ops);
	pr_err("%s is called\n",__func__);

	if(!ret){
		struct seq_file *sf = file->private_data;
		sf->private = PDE(inode);
	}

	return ret;
}
static const struct file_operations device_node_fops = {
	.read =  seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.open = device_proc_open,
	.owner = THIS_MODULE,
};

int register_device_proc(char *name, char *version, char *manufacture)
{
	struct proc_dir_entry *d_entry;
	struct manufacture_info *info;

	if(!parent) {
		parent =  proc_mkdir ("devinfo", NULL);
		if(!parent) {
			pr_err("can't create devinfo proc\n");
			return -ENOENT;
		}
	}

	info = kzalloc(sizeof *info, GFP_KERNEL);
	info->version = version;
	info->manufacture = manufacture;
	d_entry = proc_create_data (name, S_IRUGO, parent, &device_node_fops, info);
	if(!d_entry) {
		DEVINFO_ERR("create %s proc failed.\n", name);
		kfree(info);
		return -ENOENT;
	}
	return 0;
}

int register_devinfo(char *name, struct manufacture_info *info)
{
	struct proc_dir_entry *d_entry;
	if(!parent) {
	parent =  proc_mkdir ("devinfo", NULL);
	if(!parent) {
			pr_err("can't create devinfo proc\n");
			return -ENOENT;
		}
	}

	d_entry = proc_create_data (name, S_IRUGO, parent, &device_node_fops, info);
	if(!d_entry) {
		pr_err("create %s proc failed.\n", name);
		return -ENOENT;
	}
	return 0;
}

static int get_hw_opreator_version(struct devinfo_data *const devinfo_data)
{
	DEVINFO_MSG("%d\n", get_Operator_Version());
	return get_Operator_Version();
}

static int mainboard_verify(struct devinfo_data *const devinfo_data)
{
	struct manufacture_info mainboard_info;
	int hw_opreator_version = 0;
	static char temp[INFO_BUF_LEN] = {0}; //just alloc a memory

	mainboard_info.manufacture = temp;

	if (!devinfo_data) {
		DEVINFO_ERR("devinfo_data is NULL\n");
		return -1;
	}

	hw_opreator_version = get_hw_opreator_version(devinfo_data);
	switch(get_PCB_Version()) {
		case HW_VERSION__10:
			mainboard_info.version ="10";
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SA", hw_opreator_version);
			break;
		case HW_VERSION__11:
			mainboard_info.version = "11";
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SB", hw_opreator_version);
			break;
		case HW_VERSION__12:
			mainboard_info.version = "12";
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SC", hw_opreator_version);
			break;
		case HW_VERSION__13:
			mainboard_info.version = "13";
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SD", hw_opreator_version);
			break;
		case HW_VERSION__14:
			mainboard_info.version = "14";
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-SE", hw_opreator_version);
			break;
		case HW_VERSION__15:
			mainboard_info.version = "15";
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-(T3-T4)", hw_opreator_version);
			break;
		default:
			mainboard_info.version = "UNKOWN";
			snprintf(mainboard_info.manufacture, INFO_BUF_LEN, "%d-UNKOWN", hw_opreator_version);
	}

	return register_device_proc("mainboard", mainboard_info.version, mainboard_info.manufacture);
}


static int subboard_init(struct devinfo_data *const devinfo_data) {
	int ret = 0;
	struct device_node *np = NULL;
	
	np = devinfo_data->devinfo->dev.of_node;

	devinfo_data->sub_hw_id1 = of_get_named_gpio(np, "Hw,sub_hwid_1", 0);
	if(devinfo_data->sub_hw_id1 < 0 ) {
		DEVINFO_ERR("devinfo_data->sub_hw_id1 not specified\n");
		return -1;
	}

	devinfo_data->sub_hw_id2 = of_get_named_gpio(np, "Hw,sub_hwid_2", 0);
	if(devinfo_data->sub_hw_id2 < 0 ) {
		DEVINFO_ERR("devinfo_data->sub_hw_id2 not specified\n");
		return -1;
	}

	devinfo_data->pinctrl = devm_pinctrl_get(&devinfo_data->devinfo->dev);
	if (IS_ERR_OR_NULL(devinfo_data->pinctrl)) {
		 DEVINFO_ERR("Getting pinctrl handle failed\n");
		 return -1;
	}

	devinfo_data->hw_sub_id_sleep = pinctrl_lookup_state(devinfo_data->pinctrl, "hw_sub_id_sleep");
	if (IS_ERR_OR_NULL(devinfo_data->hw_sub_id_sleep)) {
		 DEVINFO_ERR("Failed to get the suspend state pinctrl handle\n");
		 return -1;
	}

	devinfo_data->hw_sub_id_active = pinctrl_lookup_state(devinfo_data->pinctrl, "hw_sub_id_active");
	if (IS_ERR_OR_NULL(devinfo_data->hw_sub_id_active)) {
		DEVINFO_ERR("Failed to get the suspend state pinctrl handle\n");
		return -1;
	}

	devinfo_data->sub_mainboard_info.version = kzalloc(INFO_BUF_LEN, GFP_KERNEL);
	if (devinfo_data->sub_mainboard_info.version == NULL) {
		return -1;
	}

	devinfo_data->sub_mainboard_info.manufacture = kzalloc(INFO_BUF_LEN, GFP_KERNEL);
	if (devinfo_data->sub_mainboard_info.manufacture == NULL) {
		kfree(devinfo_data->sub_mainboard_info.version);
		return -1;
	}

	snprintf(devinfo_data->sub_mainboard_info.version, INFO_BUF_LEN, "MTK");
#ifndef ODM_WT_EDIT
/*Haibo.Dong@ODM_WT.BSP.Kernel.Boot, 2020/04/02, Add for devinfo*/
	ret = register_device_proc("audio_mainboard",
#else
	ret = register_device_proc("sub_mainboard",
#endif
		devinfo_data->sub_mainboard_info.version,
		devinfo_data->sub_mainboard_info.manufacture);

	return ret;
}

/**
* @ sub_mainboard_verify
* normally we use one or two pins to distinguish sub mainboard(small board)
* after get the state of the pins, set to pin state to pull down.
*/
#ifndef ODM_WT_EDIT
/*Haibo.Dong@ODM_WT.BSP.Kernel.Boot, 2020/04/02, Add for devinfo*/
static int subboard_verify(struct devinfo_data *const devinfo_data)
{
	int ret = 0;
	int id1 = -1;
  	int id2 = -1;
	int a = -1;
	int b = -1;
	static int operator = OPERATOR_UNKOWN ;

	DEVINFO_MSG("Enter\n");
	if(IS_ERR_OR_NULL(devinfo_data)){
		DEVINFO_ERR("devinfo_data is NULL\n");
		return -1;
	}

	//ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_active);

	if(devinfo_data->sub_hw_id1 >= 0 ) {
		ret = gpio_request(devinfo_data->sub_hw_id1, "SUB_HW_ID1");
		if(ret){
			DEVINFO_ERR("unable to request gpio [%d]\n",devinfo_data->sub_hw_id1);
			return ret;
		}else{
			ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_sleep);
			msleep(2);
			a = gpio_get_value(devinfo_data->sub_hw_id1);
			ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_active);
			msleep(2);
			b = gpio_get_value(devinfo_data->sub_hw_id1);

			if ((a == 0) && (b == 0)){
				id1 = 0;
			} else if ((a == 1) && (b == 1)){
				id1 = 2;
			} else if ((a == 0) && (b == 1)){
				id1 = 1;
			}
			gpio_free(devinfo_data->sub_hw_id1);
		}
 	}

 	if(devinfo_data->sub_hw_id2 >= 0 ) {
		ret = gpio_request(devinfo_data->sub_hw_id2, "SUB_HW_ID2");
		if(ret){
			DEVINFO_ERR("unable to request gpio [%d]\n",devinfo_data->sub_hw_id2);
			return ret;
		}else{
			ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_sleep);
			msleep(2);
			a = gpio_get_value(devinfo_data->sub_hw_id2);
			msleep(2);
			ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_active);
			msleep(2);
			b = gpio_get_value(devinfo_data->sub_hw_id2);

			if ((a == 0) && (b == 0)){
				id2 = 0;
			} else if ((a == 1) && (b == 1)){
				id2 = 2;
			} else if ((a == 0) && (b == 1)){
				id2 = 1;
			}
			gpio_free(devinfo_data->sub_hw_id2);
		}
 	}

	operator = get_Operator_Version();

	DEVINFO_ERR("id1(%d) = %d id2(%d) = %d Operator = %d", devinfo_data->sub_hw_id1, id1,
		devinfo_data->sub_hw_id2, id2, operator);

	switch(get_project()) {
		case OPPO_18151:
			if ((id1 == 0) && (id2 == 1) && (operator == OPERATOR_18151_MOBILE || operator == OPERATOR_18151_All_BAND)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			} else if ((id1 == 0) && (id2 == 0) && (operator == OPERATOR_18151_CARRIER)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			} else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		case OPPO_18073:
		case OPPO_18593:
			if ((id1 == 1) && (id2 == 1) && (operator == OPERATOR_18073_MOBILE || operator == OPERATOR_18073_All_BAND)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			} else if ((id1 == 1) && (id2 == 0) && (operator == OPERATOR_18593_CARRIER)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		case OPPO_19011:
		case OPPO_19301:
			if ((id1 == 1) && (id2 == 1) && (operator == OPERATOR_19011_All_BAND || operator == OPERATOR_19011_MOBILE)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			} else if ((id1 == 0) && (id2 == 1) && (operator == OPERATOR_19301_CARRIER || operator == OPERATOR_19305_CARRIER)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		case OPPO_19551:
			if ((id1 == 1) && (id2 == 1)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		case OPPO_19550:
		case OPPO_19553:
		case OPPO_19556:
			if ((id1 == 0) && (id2 == 1)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		/*case OPPO_19550:
			if ((id1 == 1) && (id2 == 0)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;*/
		case OPPO_19597:
			if ((id1 == 1) && (id2 == 1)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			} else if ((id1 == 1) && (id2 == 0)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			} else if ((id1 == 1) && (id2 == 2)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		case OPPO_19358:
		case OPPO_19357:
			if ((id1 == 1) && (id2 == 1)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		case OPPO_19354:
		case OPPO_19359:
			if ((id1 == 1) && (id2 == 0)) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-match");
			}else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "rf-unmatch");
			}
			break;
		default:
			DEVINFO_ERR("illegal project\n");
			break;
	}

	ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_sleep);
	if (ret < 0) {
		DEVINFO_ERR("set sub id1 failed\n");
		return -1;
	}

	return ret;
}
#else

#ifdef ODM_WT_EDIT
/*xu.chen@ODM_WT.BSP.Kernel.Boot, 2020/07/7, Add for 98639 subID match*/
int ispascal_e()
{
	int operator=0;
	operator = get_Operator_Version();
	if( operator==136 || operator==137 || operator==138 || operator==139 || operator==140)
		return 1;
	else
		return 0;
}
#endif /* ODM_WT_EDIT */

static int subboard_verify(struct devinfo_data *const devinfo_data)
{
	int ret = -1;
	int sub_gpio = -1;
	int main_gpio = -1;
	DEVINFO_MSG("Enter\n");
	if(IS_ERR_OR_NULL(devinfo_data)){
		DEVINFO_ERR("devinfo_data is NULL\n");
		return -1;
	}

	if((devinfo_data->sub_hw_id1 >= 0) && (devinfo_data->sub_hw_id2 >= 0)) {
		ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_active);
		if (ret < 0) {
			DEVINFO_ERR("set sub id1 active failed\n");
			return -1;
		}
		msleep(2);
		sub_gpio = gpio_get_value(devinfo_data->sub_hw_id1);
		main_gpio = gpio_get_value(devinfo_data->sub_hw_id2);
		DEVINFO_ERR("sub_gpio(%d) = %d main_gpio(%d) = %d\n", devinfo_data->sub_hw_id1, sub_gpio, devinfo_data->sub_hw_id2, main_gpio);
#ifdef ODM_WT_EDIT
/*xu.chen@ODM_WT.BSP.Kernel.Boot, 2020/06/12, Add for 98639 subID match*/
		if(ispascal_e()){
			if (sub_gpio==1 && main_gpio==0) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "-match");
			} else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "-unmatch");
				}
		}else{
			if (sub_gpio == main_gpio) {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "-match");
			} else {
				snprintf(devinfo_data->sub_mainboard_info.manufacture, INFO_BUF_LEN, "-unmatch");
			}
		}
#endif /* ODM_WT_EDIT */
		ret = pinctrl_select_state(devinfo_data->pinctrl,devinfo_data->hw_sub_id_sleep);
		if (ret < 0) {
			DEVINFO_ERR("set sub id1 sleep failed\n");
		}
	} else {
		DEVINFO_ERR("get sub main id failed\n");
	}

	return ret;
}
#endif /* ODM_WT_EDIT*/

static ssize_t devinfo_modify_write(struct file *file, const char __user *buff, size_t count, loff_t *ppos){
	char proc_devinfo_modify_data[28];
	DEVINFO_ERR("call\n");
	if (copy_from_user(&proc_devinfo_modify_data, buff, count)){
		DEVINFO_ERR("error.\n");
		return count;
	}
	if (strncmp(proc_devinfo_modify_data, "1", 1) == 0){
		DEVINFO_ERR("subboard need to check again.\n");
		subboard_verify(dev_info);
	} else {
		DEVINFO_ERR("not support \n");
		return count;
	}

	return count;
}

static const struct file_operations devinfo_modify_fops = {
	.write = devinfo_modify_write,
	.llseek = noop_llseek,
};

static ssize_t fork_para_monitor_read_proc(struct file *file, char __user *buf,
                size_t count, loff_t *off)
{
        char page[256] = {0};
        int ret = 0;
        ret = snprintf(page, 255, " times:%d\n father pid:%d\n child pid:%d\n", happend_times, fork_pid_father, fork_pid_child);

        ret = simple_read_from_buffer(buf, count, off, page, strlen(page));
        return ret;
}

struct file_operations fork_para_monitor_proc_fops = {
        .read = fork_para_monitor_read_proc,
        .write = NULL,
};

static void recursive_fork_para_monitor(void)
{
		struct proc_dir_entry *pentry;

		pentry = proc_create("fork_monitor", S_IRUGO, parent, &fork_para_monitor_proc_fops);
        if (!pentry) {
                pr_err("create /devinfo/fork_monitor proc failed.\n");
        }
}

static void init_proc_devinfo_modify(void){
	struct proc_dir_entry *p = NULL;
	p = proc_create("devinfo_modify", S_IWUSR | S_IWGRP | S_IWOTH, NULL, &devinfo_modify_fops);
	if(!p)
		DEVINFO_ERR("proc_create devinfo_modify_fops fail!\n");
}

#ifdef ODM_WT_EDIT
/*Haibo.Dong@ODM_WT.BSP.Kernel.Boot, 2020/04/02, Add for devinfo*/
extern int proc_devinfo_init( struct proc_dir_entry *devinfo_dir);
#endif /*ODM_WT_EDIT*/

static int devinfo_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct devinfo_data * const devinfo_data = devm_kzalloc(&pdev->dev,sizeof(struct devinfo_data), GFP_KERNEL);

	if( IS_ERR_OR_NULL(devinfo_data) ) {
		DEVINFO_ERR("devinfo_data kzalloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	devinfo_data->devinfo = pdev;
	dev_info = devinfo_data;

	if(!parent) {
		parent =  proc_mkdir ("devinfo", NULL);
		if(!parent) {
			DEVINFO_ERR("can't create devinfo proc\n");
			ret = -ENOENT;
		}
	}
#ifdef ODM_WT_EDIT
/*#Haibo.Dong@ODM_WT.BSP.Kernel.Boot, 2020/04/02, Add for devinfo*/
	proc_devinfo_init(parent);
#endif /*ODM_WT_EDIT*/
	ret = subboard_init(devinfo_data);
	if (ret < 0) {
		DEVINFO_ERR("register audio mainboard failed\n");
	} else {
		subboard_verify(devinfo_data);
	}

	ret = mainboard_verify(devinfo_data);
	if (ret < 0) {
		DEVINFO_ERR("register mainboard failed\n");
	}
	recursive_fork_para_monitor();
	init_proc_devinfo_modify();
	return ret;
}

static int devinfo_remove(struct platform_device *dev)
{
	remove_proc_entry(DEVINFO_NAME, NULL);
	return 0;
}

static struct platform_driver devinfo_platform_driver = {
	.probe = devinfo_probe,
	.remove = devinfo_remove,
	.driver = {
		.name = DEVINFO_NAME,
		.of_match_table = devinfo_id,
	},
};

module_platform_driver(devinfo_platform_driver);

MODULE_DESCRIPTION("OPPO device info");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Wangjc <wjc@oppo.com>");
