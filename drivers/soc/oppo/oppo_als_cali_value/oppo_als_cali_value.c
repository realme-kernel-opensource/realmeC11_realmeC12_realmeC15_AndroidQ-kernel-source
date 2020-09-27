/************************************************************************************
 ** VENDOR_EDIT
 ** Copyright (C), 2008-2018, OPPO Mobile Comm Corp., Ltd
 **
 ** Description:
 **      oppo_als_cali_value.c (sw23)
 **
 ** Version: 1.0
 ** Date created: 18:03:11,11/21/2018
 ** Author: Chao.Zeng@PSW.BSP.Sensor
 ** --------------------------- Revision History: --------------------------------
 **  <author>      <data>            <desc>
 **  Chao.Zeng       11/21/2018      create the file
 ************************************************************************************/

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <soc/oppo/oppo_project.h>

struct oppo_als_cali_data {
    int red_max_lux;
    int green_max_lux;
    int blue_max_lux;
    int white_max_lux;
    int cali_coe;
    int row_coe;
    struct proc_dir_entry         *proc_oppo_als;
};
static struct oppo_als_cali_data *gdata = NULL;

static ssize_t red_max_lux_read_proc(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    if (!gdata){
        return -ENOMEM;
    }

    len = sprintf(page,"%d",gdata->red_max_lux);

    if(len > *off)
        len -= *off;
    else
        len = 0;

    if(copy_to_user(buf,page,(len < count ? len : count))){
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}
static ssize_t red_max_lux_write_proc(struct file *file, const char __user *buf,
        size_t count, loff_t *off)

{
    char page[256] = {0};
    unsigned int input = 0;

    if(!gdata){
        return -ENOMEM;
    }


    if (count > 256)
        count = 256;
    if(count > *off)
        count -= *off;
    else
        count = 0;

    if (copy_from_user(page, buf, count))
        return -EFAULT;
    *off += count;

    if (sscanf(page, "%u", &input) != 1) {
        count = -EINVAL;
        return count;
    }

    if(input != gdata->red_max_lux){
        gdata->red_max_lux= input;
    }

    return count;
}
static struct file_operations red_max_lux_fops = {
    .read = red_max_lux_read_proc,
    .write = red_max_lux_write_proc,
};
static ssize_t white_max_lux_read_proc(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    if (!gdata){
        return -ENOMEM;
    }

    len = sprintf(page,"%d",gdata->white_max_lux);

    if(len > *off)
        len -= *off;
    else
        len = 0;

    if(copy_to_user(buf,page,(len < count ? len : count))){
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}
static ssize_t white_max_lux_write_proc(struct file *file, const char __user *buf,
        size_t count, loff_t *off)

{
    char page[256] = {0};
    unsigned int input = 0;

    if(!gdata){
        return -ENOMEM;
    }


    if (count > 256)
        count = 256;
    if(count > *off)
        count -= *off;
    else
        count = 0;

    if (copy_from_user(page, buf, count))
        return -EFAULT;
    *off += count;

    if (sscanf(page, "%u", &input) != 1) {
        count = -EINVAL;
        return count;
    }

    if(input != gdata->white_max_lux){
        gdata->white_max_lux= input;
    }

    return count;
}
static struct file_operations white_max_lux_fops = {
    .read = white_max_lux_read_proc,
    .write = white_max_lux_write_proc,
};
static ssize_t blue_max_lux_read_proc(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    if (!gdata){
        return -ENOMEM;
    }

    len = sprintf(page,"%d",gdata->blue_max_lux);

    if(len > *off)
        len -= *off;
    else
        len = 0;

    if(copy_to_user(buf,page,(len < count ? len : count))){
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}
static ssize_t blue_max_lux_write_proc(struct file *file, const char __user *buf,
        size_t count, loff_t *off)

{
    char page[256] = {0};
    unsigned int input = 0;

    if(!gdata){
        return -ENOMEM;
    }


    if (count > 256)
        count = 256;
    if(count > *off)
        count -= *off;
    else
        count = 0;

    if (copy_from_user(page, buf, count))
        return -EFAULT;
    *off += count;

    if (sscanf(page, "%u", &input) != 1) {
        count = -EINVAL;
        return count;
    }

    if(input != gdata->blue_max_lux){
        gdata->blue_max_lux= input;
    }

    return count;
}
static struct file_operations blue_max_lux_fops = {
    .read = blue_max_lux_read_proc,
    .write = blue_max_lux_write_proc,
};
static ssize_t green_max_lux_read_proc(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    if (!gdata){
        return -ENOMEM;
    }

    len = sprintf(page,"%d",gdata->green_max_lux);

    if(len > *off)
        len -= *off;
    else
        len = 0;

    if(copy_to_user(buf,page,(len < count ? len : count))){
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}
static ssize_t green_max_lux_write_proc(struct file *file, const char __user *buf,
        size_t count, loff_t *off)

{
    char page[256] = {0};
    unsigned int input = 0;

    if(!gdata){
        return -ENOMEM;
    }


    if (count > 256)
        count = 256;
    if(count > *off)
        count -= *off;
    else
        count = 0;

    if (copy_from_user(page, buf, count))
        return -EFAULT;
    *off += count;

    if (sscanf(page, "%u", &input) != 1) {
        count = -EINVAL;
        return count;
    }

    if(input != gdata->green_max_lux){
        gdata->green_max_lux= input;
    }

    return count;
}
static struct file_operations green_max_lux_fops = {
    .read = green_max_lux_read_proc,
    .write = green_max_lux_write_proc,
};
static ssize_t cali_coe_read_proc(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    if (!gdata){
        return -ENOMEM;
    }

    len = sprintf(page,"%d",gdata->cali_coe);

    if(len > *off)
        len -= *off;
    else
        len = 0;

    if(copy_to_user(buf,page,(len < count ? len : count))){
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}
static ssize_t cali_coe_write_proc(struct file *file, const char __user *buf,
        size_t count, loff_t *off)

{
    char page[256] = {0};
    unsigned int input = 0;

    if(!gdata){
        return -ENOMEM;
    }


    if (count > 256)
        count = 256;
    if(count > *off)
        count -= *off;
    else
        count = 0;

    if (copy_from_user(page, buf, count))
        return -EFAULT;
    *off += count;

    if (sscanf(page, "%u", &input) != 1) {
        count = -EINVAL;
        return count;
    }

    if(input != gdata->cali_coe){
        gdata->cali_coe= input;
    }

    return count;
}
static struct file_operations cali_coe_fops = {
    .read = cali_coe_read_proc,
    .write = cali_coe_write_proc,
};
static ssize_t row_coe_read_proc(struct file *file, char __user *buf,
        size_t count,loff_t *off)
{
    char page[256] = {0};
    int len = 0;

    if (!gdata){
        return -ENOMEM;
    }

    len = sprintf(page,"%d",gdata->row_coe);

    if(len > *off)
        len -= *off;
    else
        len = 0;

    if(copy_to_user(buf,page,(len < count ? len : count))){
        return -EFAULT;
    }
    *off += len < count ? len : count;
    return (len < count ? len : count);
}
static ssize_t row_coe_write_proc(struct file *file, const char __user *buf,
        size_t count, loff_t *off)

{
    char page[256] = {0};
    unsigned int input = 0;

    if(!gdata){
        return -ENOMEM;
    }


    if (count > 256)
        count = 256;
    if(count > *off)
        count -= *off;
    else
        count = 0;

    if (copy_from_user(page, buf, count))
        return -EFAULT;
    *off += count;

    if (sscanf(page, "%u", &input) != 1) {
        count = -EINVAL;
        return count;
    }

    if(input != gdata->row_coe){
        gdata->row_coe= input;
    }

    return count;
}
static struct file_operations row_coe_fops = {
    .read = row_coe_read_proc,
    .write = row_coe_write_proc,
};
static int __init oppo_als_cali_data_init(void)
{
    int rc = 0;
    struct proc_dir_entry *pentry;
    unsigned int prj = 0;

    struct oppo_als_cali_data *data = NULL;
    if(gdata){
        printk("%s:just can be call one time\n",__func__);
        return 0;
    }
    data = kzalloc(sizeof(struct oppo_als_cali_data),GFP_KERNEL);
    if(data == NULL){
        rc = -ENOMEM;
        printk("%s:kzalloc fail %d\n",__func__,rc);
        return rc;
    }
    gdata = data;
    gdata->row_coe = 540;
    prj = get_project();
    if ( (prj == 19550) || (prj == 19551) || (prj == 19553) || (prj == 19556)) {
        gdata->row_coe = 140;
    }
    else if((prj == 19354) || (prj == 19357) || (prj == 19358) || (prj == 19359)){
        gdata->row_coe = 110;
    }
    if (gdata->proc_oppo_als) {
        printk("proc_oppo_als has alread inited\n");
        return 0;
    }

    gdata->proc_oppo_als =  proc_mkdir("oppoAls", NULL);
    if(!gdata->proc_oppo_als) {
        pr_err("can't create proc_oppo_als proc\n");
        rc = -EFAULT;
        return rc;
    }

    pentry = proc_create("red_max_lux",0666, gdata->proc_oppo_als,
        &red_max_lux_fops);
    if(!pentry) {
        pr_err("create red_max_lux proc failed.\n");
        rc = -EFAULT;
        return rc;
    }

    pentry = proc_create("green_max_lux",0666, gdata->proc_oppo_als,
        &green_max_lux_fops);
    if(!pentry) {
        pr_err("create green_max_lux proc failed.\n");
        rc = -EFAULT;
        return rc;
    }

    pentry = proc_create("blue_max_lux",0666, gdata->proc_oppo_als,
        &blue_max_lux_fops);
    if(!pentry) {
        pr_err("create blue_max_lux proc failed.\n");
        rc = -EFAULT;
        return rc;
    }

    pentry = proc_create("white_max_lux",0666, gdata->proc_oppo_als,
        &white_max_lux_fops);
    if(!pentry) {
        pr_err("create white_max_lux proc failed.\n");
        rc = -EFAULT;
        return rc;
    }

    pentry = proc_create("cali_coe",0666, gdata->proc_oppo_als,
        &cali_coe_fops);
    if(!pentry) {
        pr_err("create cali_coe proc failed.\n");
        rc = -EFAULT;
        return rc;
    }

    pentry = proc_create("row_coe",0666, gdata->proc_oppo_als,
        &row_coe_fops);
    if(!pentry) {
        pr_err("create row_coe proc failed.\n");
        rc = -EFAULT;
        return rc;
    }

    return 0;
}

void oppo_als_cali_data_clean(void)
{
    if (gdata){
        kfree(gdata);
        gdata = NULL;
    }
}
module_init(oppo_als_cali_data_init);
module_exit(oppo_als_cali_data_clean);
MODULE_DESCRIPTION("OPPO custom version");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chao.Zeng <zengchao@oppo.com>");
