/*
 * kernel/power/wakeup_reason.c
 *
 * Logs the reasons which caused the kernel to resume from
 * the suspend mode.
 *
 * Copyright (C) 2014 Google, Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/wakeup_reason.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/suspend.h>

#ifdef VENDOR_EDIT
/* ChaoYing.Chen@BSP.Power.Basic.1056413, 2017/12/9, Add for print wakeup source */
#include <linux/notifier.h>
#include <linux/fb.h>



#define LOG_BUF_SIZE	256
extern	char wakeup_source_buf[LOG_BUF_SIZE];

 
#if defined (CONFIG_MACH_MT6771) || defined (CONFIG_MACH_MT6765)|| defined (CONFIG_MACH_MT6779)
/* ChaoYing.Chen@BSP.Power.Basic.1056413, 2018/04/23, Add for print wakeup source */
#define PMIC_INT_REG_WIDTH  	16
#define PMIC_INT_REG_NUMBER  	16
#else /* CONFIG_MACH_MT6771 */
#define PMIC_INT_REG_WIDTH  	16
#define PMIC_INT_REG_NUMBER  	18
#endif /* CONFIG_MACH_MT6771 */

extern u64 pmic_wakesrc_x_count[PMIC_INT_REG_NUMBER][PMIC_INT_REG_WIDTH];
extern const char *pmic_interrupt_status_name[PMIC_INT_REG_NUMBER][PMIC_INT_REG_WIDTH];

#define EINT_WIDTH  			32
#define EINT_REG_NUMBER  		18
extern u64 eint_wakesrc_x_count[EINT_REG_NUMBER][EINT_WIDTH];
extern  u64  wakesrc_count[32];
int wakeup_reason_stastics_flag = 0;
extern const char * mt_eint_get_name(int index);
extern void mt_clear_wakesrc_count(void);
extern void mt_pmic_clear_wakesrc_count(void);
extern void mt_eint_clear_wakesrc_count(void);

extern const char *wakesrc_str[32];

#endif /* VENDOR_EDIT */

#define MAX_WAKEUP_REASON_IRQS 32
static int irq_list[MAX_WAKEUP_REASON_IRQS];
static int irqcount;
static bool suspend_abort;
static char abort_reason[MAX_SUSPEND_ABORT_LEN];
static struct kobject *wakeup_reason;
static DEFINE_SPINLOCK(resume_reason_lock);

static ktime_t last_monotime; /* monotonic time before last suspend */
static ktime_t curr_monotime; /* monotonic time after last suspend */
static ktime_t last_stime; /* monotonic boottime offset before last suspend */
static ktime_t curr_stime; /* monotonic boottime offset after last suspend */


#ifdef VENDOR_EDIT
//Wenxian.zhen@Prd.BaseDrv, 2016/07/19, add for analysis power consumption
void wakeup_src_clean(void);
#endif /* VENDOR_EDIT */
#ifdef VENDOR_EDIT
/* ChaoYing.Chen@BSP.Power.Basic.1056413, 2017/12/11, Add for print wakeup source */
static ssize_t new_resume_reason_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%s\n", wakeup_source_buf);
}

static struct kobj_attribute new_resume_reason = __ATTR_RO(new_resume_reason);

static ssize_t ap_resume_reason_stastics_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	int i = 0;
	int j = 0;
	int buf_offset = 0;
	const char *name = NULL;

	for (i = 0; i < MAX_WAKEUP_REASON_IRQS; i++) {
		if (wakesrc_count[i]) {
			buf_offset += sprintf(buf + buf_offset, wakesrc_str[i]);
			buf_offset += sprintf(buf + buf_offset,  "%s",":");
			buf_offset += sprintf(buf + buf_offset,  "%lld \n",wakesrc_count[i]);
			printk(KERN_WARNING "%s wakeup %lld times\n",wakesrc_str[i],wakesrc_count[i]);
		}
	}
	for (i = 0; i < EINT_REG_NUMBER; i++) {
		for (j = 0; j < EINT_WIDTH; j++) {
			if (eint_wakesrc_x_count[i][j] != 0)	{
				name = mt_eint_get_name(i*32 +j);
				buf_offset += sprintf(buf + buf_offset, name);
				buf_offset += sprintf(buf + buf_offset, "%s", ":");
				buf_offset += sprintf(buf + buf_offset, "%lld \n", eint_wakesrc_x_count[i][j]);
				printk(KERN_WARNING "%s wakeup %lld times\n", name, eint_wakesrc_x_count[i][j]);
			}
		}
	}
	for (i = 0; i < PMIC_INT_REG_NUMBER; i++) {
		for (j = 0; j < PMIC_INT_REG_WIDTH; j++) {
			if (pmic_wakesrc_x_count[i][j] != 0)	{
				buf_offset += sprintf(buf + buf_offset, pmic_interrupt_status_name[i][j]);
				buf_offset += sprintf(buf + buf_offset, "%s", ":");
				buf_offset += sprintf(buf + buf_offset, "%lld \n", pmic_wakesrc_x_count[i][j]);
				printk(KERN_WARNING "%s wakeup %lld times\n", pmic_interrupt_status_name[i][j], pmic_wakesrc_x_count[i][j]);
			}
		}
	}
	return buf_offset;
}


static struct kobj_attribute ap_resume_reason_stastics = __ATTR_RO(ap_resume_reason_stastics);


#endif /* VENDOR_EDIT */

static ssize_t last_resume_reason_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
/* Zhengding.Chen@BSP.Power.Basic, 2019/06/14, Add for batterystats count last resume wakeup source correctly*/
#ifdef VENDOR_EDIT
	int buf_offset = 0;
#else
	int irq_no, buf_offset = 0;
	struct irq_desc *desc;
#endif /* VENDOR_EDIT */
	spin_lock(&resume_reason_lock);
	if (suspend_abort) {
		buf_offset = sprintf(buf, "Abort: %s", abort_reason);
	} else {
/* Zhengding.Chen@BSP.Power.Basic, 2019/06/14, Add for batterystats count last resume wakeup source correctly*/
#ifdef VENDOR_EDIT
		buf_offset = sprintf(buf, "0 %s\n", wakeup_source_buf);
#else
		for (irq_no = 0; irq_no < irqcount; irq_no++) {
			desc = irq_to_desc(irq_list[irq_no]);
			if (desc && desc->action && desc->action->name)
				buf_offset += sprintf(buf + buf_offset, "%d %s\n",
						irq_list[irq_no], desc->action->name);
			else
				buf_offset += sprintf(buf + buf_offset, "%d\n",
						irq_list[irq_no]);
		}
#endif /* VENDOR_EDIT */
	}
	spin_unlock(&resume_reason_lock);
	return buf_offset;
}

static ssize_t last_suspend_time_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	struct timespec sleep_time;
	struct timespec total_time;
	struct timespec suspend_resume_time;

	/*
	 * total_time is calculated from monotonic bootoffsets because
	 * unlike CLOCK_MONOTONIC it include the time spent in suspend state.
	 */
	total_time = ktime_to_timespec(ktime_sub(curr_stime, last_stime));

	/*
	 * suspend_resume_time is calculated as monotonic (CLOCK_MONOTONIC)
	 * time interval before entering suspend and post suspend.
	 */
	suspend_resume_time = ktime_to_timespec(ktime_sub(curr_monotime, last_monotime));

	/* sleep_time = total_time - suspend_resume_time */
	sleep_time = timespec_sub(total_time, suspend_resume_time);

	/* Export suspend_resume_time and sleep_time in pair here. */
	return sprintf(buf, "%lu.%09lu %lu.%09lu\n",
				suspend_resume_time.tv_sec, suspend_resume_time.tv_nsec,
				sleep_time.tv_sec, sleep_time.tv_nsec);
}

#ifdef VENDOR_EDIT
//Wenxian.Zhen@BSP.Power.Basic, 2018/11/17, Add for  clean wake up source  according to echo reset >   /sys/kernel/wakeup_reasons/wakeup_stastisc_reset
static ssize_t  wakeup_stastisc_reset_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	char reset_string[]="reset";
#ifdef VENDOR_EDIT
/* Ji.Xu@SW.BSP.CHG, 2018-11-29 modify framework ioctl fail */
	if(!((count == strlen(reset_string)) || ((count == strlen(reset_string) + 1) && (buf[count-1] == '\n'))))
#endif /*VENDOR_EDIT*/
		return count;

	if (strncmp(buf, reset_string, strlen(reset_string)) != 0)
		return count;

	wakeup_src_clean();
	return count;
}


#endif /* VENDOR_EDIT */

static struct kobj_attribute resume_reason = __ATTR_RO(last_resume_reason);
static struct kobj_attribute suspend_time = __ATTR_RO(last_suspend_time);

#ifdef VENDOR_EDIT
//Wenxian.Zhen@BSP.Power.Basic, 2018/11/17, Add for  clean wake up source  according to echo reset >   /sys/kernel/wakeup_reasons/wakeup_stastisc_reset
static struct kobj_attribute wakeup_stastisc_reset_sys =
	__ATTR(wakeup_stastisc_reset, S_IWUSR|S_IRUGO, NULL, wakeup_stastisc_reset_store);
#endif /* VENDOR_EDIT */
static struct attribute *attrs[] = {
	&resume_reason.attr,

	#ifdef VENDOR_EDIT
	/* ChaoYing.Chen@BSP.Power.Basic.1056413, 2017/12/11, Add for print wakeup source */
	&new_resume_reason.attr,
	&ap_resume_reason_stastics.attr,


	#endif /* VENDOR_EDIT */
	#ifdef VENDOR_EDIT
	//Wenxian.Zhen@BSP.Power.Basic, 2018/11/17, Add for  clean wake up source  according to echo reset >   /sys/kernel/wakeup_reasons/wakeup_stastisc_reset
    &wakeup_stastisc_reset_sys.attr,
	#endif /* VENDOR_EDIT */
	&suspend_time.attr,
	NULL,
};
static struct attribute_group attr_group = {
	.attrs = attrs,
};

/*
 * logs all the wake up reasons to the kernel
 * stores the irqs to expose them to the userspace via sysfs
 */
void log_wakeup_reason(int irq)
{
	struct irq_desc *desc;
	desc = irq_to_desc(irq);
	if (desc && desc->action && desc->action->name)
		printk(KERN_INFO "Resume caused by IRQ %d, %s\n", irq,
				desc->action->name);
	else
		printk(KERN_INFO "Resume caused by IRQ %d\n", irq);

	spin_lock(&resume_reason_lock);
	if (irqcount == MAX_WAKEUP_REASON_IRQS) {
		spin_unlock(&resume_reason_lock);
		printk(KERN_WARNING "Resume caused by more than %d IRQs\n",
				MAX_WAKEUP_REASON_IRQS);
		return;
	}

	irq_list[irqcount++] = irq;
	spin_unlock(&resume_reason_lock);
}

int check_wakeup_reason(int irq)
{
	int irq_no;
	int ret = false;

	spin_lock(&resume_reason_lock);
	for (irq_no = 0; irq_no < irqcount; irq_no++)
		if (irq_list[irq_no] == irq) {
			ret = true;
			break;
	}
	spin_unlock(&resume_reason_lock);
	return ret;
}

void log_suspend_abort_reason(const char *fmt, ...)
{
	va_list args;

	spin_lock(&resume_reason_lock);

	//Suspend abort reason has already been logged.
	if (suspend_abort) {
		spin_unlock(&resume_reason_lock);
		return;
	}

	suspend_abort = true;
	va_start(args, fmt);
	vsnprintf(abort_reason, MAX_SUSPEND_ABORT_LEN, fmt, args);
	va_end(args);
	spin_unlock(&resume_reason_lock);
}

/* Detects a suspend and clears all the previous wake up reasons*/
static int wakeup_reason_pm_event(struct notifier_block *notifier,
		unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		spin_lock(&resume_reason_lock);
		irqcount = 0;
		suspend_abort = false;
		spin_unlock(&resume_reason_lock);
		/* monotonic time since boot */
		last_monotime = ktime_get();
		/* monotonic time since boot including the time spent in suspend */
		last_stime = ktime_get_boottime();
		break;
	case PM_POST_SUSPEND:
		/* monotonic time since boot */
		curr_monotime = ktime_get();
		/* monotonic time since boot including the time spent in suspend */
		curr_stime = ktime_get_boottime();
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block wakeup_reason_pm_notifier_block = {
	.notifier_call = wakeup_reason_pm_event,
};

#ifdef VENDOR_EDIT
/* ChaoYing.Chen@BSP.Power.Basic.1056413, 2017/12/11, Add for print wakeup source */
void wakeup_src_clean(void)
{
	mt_clear_wakesrc_count();
	mt_pmic_clear_wakesrc_count();
	mt_eint_clear_wakesrc_count();
}
EXPORT_SYMBOL(wakeup_src_clean);
static int wakeup_src_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank = NULL;
    const char *name = NULL;
    int i = 0;
    int j = 0;

    if (evdata && evdata->data && event == FB_EVENT_BLANK) {
        blank = evdata->data;

        if (*blank == FB_BLANK_UNBLANK) {
            for (i = 0; i < MAX_WAKEUP_REASON_IRQS; i++) {
                if (wakesrc_count[i]) {
                    printk(KERN_WARNING "%s wakeup %lld times\n", wakesrc_str[i], wakesrc_count[i]);
                }
            }
            for (i = 0; i < EINT_REG_NUMBER; i++) {
                for (j = 0; j < EINT_WIDTH; j++) {
                    if (eint_wakesrc_x_count[i][j] != 0) {
                        name = mt_eint_get_name(i*32 +j);
                        printk(KERN_WARNING "%s wakeup %lld times\n", name, eint_wakesrc_x_count[i][j]);
                    }
                }
            }
            for (i = 0; i < PMIC_INT_REG_NUMBER; i++) {
                for (j = 0; j < PMIC_INT_REG_WIDTH; j++) {
                    if (pmic_wakesrc_x_count[i][j] != 0) {
                        printk(KERN_WARNING "%s wakeup %lld times\n", pmic_interrupt_status_name[i][j], pmic_wakesrc_x_count[i][j]);
                    }
                }
            }
        }
    } else if (evdata && evdata->data && event == FB_EARLY_EVENT_BLANK) {
			blank = evdata->data;
			if (*blank == FB_BLANK_POWERDOWN) {
		//wenxian.Zhen@PSW.BSP.POWER, 2019/01/15, removing for analysis power consumption,clear wakeup source stastatics action according to framework			
//				wakeup_src_clean();
//				pr_err("[wakeup_src_fb_notifier_callback] wakeup_src_clean all wakeup\n");
			}
    }
    return 0;
}

static struct notifier_block wakeup_src_fb_notif = {
	.notifier_call = wakeup_src_fb_notifier_callback,
};
#endif /* VENDOR_EDIT */

/* Initializes the sysfs parameter
 * registers the pm_event notifier
 */
int __init wakeup_reason_init(void)
{
	int retval;

	retval = register_pm_notifier(&wakeup_reason_pm_notifier_block);
	if (retval)
		printk(KERN_WARNING "[%s] failed to register PM notifier %d\n",
				__func__, retval);

	wakeup_reason = kobject_create_and_add("wakeup_reasons", kernel_kobj);
	if (!wakeup_reason) {
		printk(KERN_WARNING "[%s] failed to create a sysfs kobject\n",
				__func__);
		return 1;
	}
	retval = sysfs_create_group(wakeup_reason, &attr_group);
	if (retval) {
		kobject_put(wakeup_reason);
		printk(KERN_WARNING "[%s] failed to create a sysfs group %d\n",
				__func__, retval);
	}
	#ifdef VENDOR_EDIT
	/* ChaoYing.Chen@BSP.Power.Basic.1056413, 2017/12/11, Add for print wakeup source */
	fb_register_client(&wakeup_src_fb_notif);
	#endif /* VENDOR_EDIT */
	return 0;
}

late_initcall(wakeup_reason_init);
