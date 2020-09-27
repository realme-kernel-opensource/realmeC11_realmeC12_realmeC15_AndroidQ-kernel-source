/* SPDX-License-Identifier: GPL-2.0 */
/*  Himax Android Driver Sample Code for common functions
 *
 *  Copyright (C) 2019 Himax Corporation.
 *
 *  This software is licensed under the terms of the GNU General Public
 *  License version 2,  as published by the Free Software Foundation,  and
 *  may be copied,  distributed,  and modified under those terms.
 *
 *  This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

/*#include "himax_common.h"*/
/*#include "himax_ic_core.h"*/
#include "himax_inspection.h"
#include "himax_modular_table.h"
#if defined(HX_TP_USB_NOTIFIER)
#include <linux/tp_usb_notifier.h>
#endif
#ifdef HX_UPDATE_FW_NOTIFIER
#include <linux/update_tpfw_notifier.h>
#endif
#ifdef HX_TP_HEADSET_NOTIFIER
#include <linux/headset_notifier.h>
#endif
#if defined(__HIMAX_MOD__)
int (*hx_msm_drm_register_client)(struct notifier_block *nb);
int (*hx_msm_drm_unregister_client)(struct notifier_block *nb);
#endif
#ifdef OPPO_PROC_NODE
static void tp_fw_update_work(struct work_struct *work);
uint32_t g_oppo_debug_level = 3;
#endif

#if defined(HX_EXCP_RECOVERY)
static void himax_excp_hw_reset(void);
#endif

#if defined(HX_SMART_WAKEUP)
#define GEST_SUP_NUM 26
/* Setting cust key define (DF = double finger) */
/* {Double Tap, Up, Down, Left, Right, C, Z, M,
 *	O, S, V, W, e, m, @, (reserve),
 *	Finger gesture, ^, >, <, f(R), f(L), Up(DF), Down(DF),
 *	Left(DF), Right(DF)}
 */
uint8_t gest_event[GEST_SUP_NUM] = {
	0x80, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x81, 0x1D, 0x2D, 0x3D, 0x1F, 0x2F, 0x51, 0x52,
	0x53, 0x54};

/*gest_event mapping to gest_key_def*/
uint16_t gest_key_def[GEST_SUP_NUM] = {
	HX_KEY_DOUBLE_CLICK, HX_KEY_UP, HX_KEY_DOWN, HX_KEY_LEFT,
	HX_KEY_RIGHT,	HX_KEY_C, HX_KEY_Z, HX_KEY_M,
	HX_KEY_O, HX_KEY_S, HX_KEY_V, HX_KEY_W,
	HX_KEY_E, HX_KEY_LC_M, HX_KEY_AT, HX_KEY_RESERVE,
	HX_KEY_FINGER_GEST,	HX_KEY_V_DOWN, HX_KEY_V_LEFT, HX_KEY_V_RIGHT,
	HX_KEY_F_RIGHT,	HX_KEY_F_LEFT, HX_KEY_DF_UP, HX_KEY_DF_DOWN,
	HX_KEY_DF_LEFT,	HX_KEY_DF_RIGHT};

uint8_t *wake_event_buffer;
#endif

#if !defined(HX_USE_KSYM)
struct himax_chip_entry himax_ksym_lookup;
EXPORT_SYMBOL(himax_ksym_lookup);
#endif

#define SUPPORT_FINGER_DATA_CHECKSUM 0x0F
#define TS_WAKE_LOCK_TIMEOUT		(5000)
#define FRAME_COUNT 5

#if defined(HX_TP_PROC_GUEST_INFO)
struct hx_guest_info *g_guest_info_data;
EXPORT_SYMBOL(g_guest_info_data);

char *g_guest_info_item[] = {
	"projectID",
	"CGColor",
	"BarCode",
	"Reserve1",
	"Reserve2",
	"Reserve3",
	"Reserve4",
	"Reserve5",
	"VCOM",
	"Vcom-3Gar",
	NULL
};
#endif

uint32_t g_hx_chip_inited;

#if defined(__EMBEDDED_FW__)
struct firmware g_embedded_fw = {
	.data = _binary___Himax_firmware_bin_start,
};
#endif

#if defined(HX_BOOT_UPGRADE) || defined(HX_ZERO_FLASH)
bool g_boot_upgrade_flag;
#endif
const struct firmware *hxfw;
int g_i_FW_VER;
int g_i_CFG_VER;
int g_i_CID_MAJ; /*GUEST ID*/
int g_i_CID_MIN; /*VER for GUEST*/
#if defined(HX_ZERO_FLASH)
int g_f_0f_updat = 0;

#if defined(OPPO_PROC_NODE)
	#define HIMAX_PROC_TOUCHSCREEN_FOLDER "touchscreen"
	struct proc_dir_entry *himax_proc_touchscreen_dir;
	#define HIMAX_PROC_OPPO_CTP_OPENSHORT_TEST	"ctp_openshort_test"
	struct proc_dir_entry *himax_proc_oppo_ctp_openshort_test = NULL;

	#define HIMAX_PROC_TOUCHPANEL_FOLDER "touchpanel"
	struct proc_dir_entry *himax_proc_touchpanel_dir;
	#define HIMAX_PROC_OPPO_REG_INFO_FILE	"oppo_register_info"
	struct proc_dir_entry *himax_proc_oppo_reg_info_file = NULL;
	#define HIMAX_PROC_TP_FW_UPDATE_FILE	"tp_fw_update"
	struct proc_dir_entry *himax_proc_tp_fw_update_file = NULL;

#ifdef HX_SMART_WAKEUP
	#define HIMAX_PROC_DOUBLE_TAP_EN_FILE	"double_tap_enable"
	struct proc_dir_entry *himax_proc_double_tap_en_file = NULL;
#ifdef HX_GESTURE_TRACK
	#define HIMAX_PROC_COORDINATE_FILE	"coordinate"
	struct proc_dir_entry *himax_proc_coordinate_file = NULL;
#endif
#endif
	#define HIMAX_PROC_LIMIT_ENABLE_FILE	"oppo_tp_limit_enable"
	struct proc_dir_entry *himax_proc_limit_enable_file;
	#define HIMAX_PROC_LIMIT_AREA_FILE	"oppo_tp_limit_area"
	struct proc_dir_entry *himax_proc_limit_area_file;
	#define PAGESIZE 256
	#define HIMAX_PROC_GAME_SWITCH_FILE	"game_switch_enable"
	struct proc_dir_entry *himax_proc_game_switch_en_file;

	#define HIMAX_PROC_OPPO_TP_DIRECTION_FILE	"oppo_tp_direction"
	struct proc_dir_entry *himax_proc_oppo_tp_direction_file = NULL;

	#define HIMAX_PROC_INCELL_PANEL_FILE	"incell_panel"
	struct proc_dir_entry *himax_proc_incell_panel_file = NULL;

	#define HIMAX_PROC_OPPO_TX_HOP_FILE	  "oppo_tp_hop_test"
	struct proc_dir_entry *himax_proc_oppo_tx_hop_file = NULL;

	#define HIMAX_PROC_BASELINE_TEST_FILE	"baseline_test"
	struct proc_dir_entry *himax_proc_baseline_test_file;

	#define HIMAX_PROC_BACKSCREEN_BASELINE_FILE	"black_screen_test"
	struct proc_dir_entry *himax_proc_backscreen_baseline_file;

	#define HIMAX_PROC_IRQ_DEPTH_FILE	"irq_depth"
	struct proc_dir_entry *himax_proc_irq_depth_file = NULL;

	#define HIMAX_PROC_OPPO_DEBUG_LEVEL_FILE	"debug_level"
	struct proc_dir_entry *himax_proc_oppo_debug_level_file = NULL;

	//proc/touchpanel/debug_infor node begin
	struct proc_dir_entry *himax_proc_debug_infor_dir = NULL;
	#define HIMAX_PROC_DEBUG_INFO_FOLDER	"debug_info"

	#define HIMAX_PROC_DELTA_O_FILE	"delta"
	struct proc_dir_entry *himax_proc_delta_o_file = NULL;
	#define HIMAX_PROC_BASELINE_O_FILE	"baseline"
	struct proc_dir_entry *himax_proc_baseline_O_file = NULL;
	#define HIMAX_PROC_MAIN_REG_FILE	"main_register"
	struct proc_dir_entry *himax_proc_main_reg_file = NULL;
	#define HIMAX_PROC_DATA_LIMIT_FILE	"data_limit"
	struct proc_dir_entry *himax_proc_data_limit_file = NULL;
	//proc/touchpanel/debug_infor node end
	extern uint8_t cmd_set[8];
	extern uint8_t cfg_flag;
	extern uint8_t byte_length;
	extern uint8_t reg_cmd[4];
	extern int oppo_baseline_read( struct seq_file *s );
	extern int oppo_delta_read_sub( struct seq_file *s );
	extern int himax_self_test_data_init(void);
	extern void himax_self_test_data_deinit(void);
	extern int **g_inspection_criteria;
	extern char *g_hx_inspt_crtra_name[];
	extern int HX_CRITERIA_SIZE;
	#if defined(HX_SMART_WAKEUP)
	extern int himax_black_chip_self_test(struct seq_file *s, void *v);
	extern int tp_gesture;
	#endif
	extern struct fw_operation *pfw_op;
	struct corner_info corner_info[4];
	struct point_info points_info[10];
#ifdef HX_TP_HEADSET_NOTIFIER
	static bool himax_tp_headset_state = 0;
#endif
	extern int get_boot_mode(void);
#endif
#endif

struct himax_ts_data *private_ts;
EXPORT_SYMBOL(private_ts);

struct himax_ic_data *ic_data;
EXPORT_SYMBOL(ic_data);

struct himax_report_data *hx_touch_data;
EXPORT_SYMBOL(hx_touch_data);

struct himax_core_fp g_core_fp;
EXPORT_SYMBOL(g_core_fp);

struct himax_debug *debug_data;
EXPORT_SYMBOL(debug_data);

struct proc_dir_entry *himax_touch_proc_dir;
EXPORT_SYMBOL(himax_touch_proc_dir);

int g_mmi_refcnt;
EXPORT_SYMBOL(g_mmi_refcnt);

#if defined(OPPO_PROC_NODE)
#define HIMAX_PROC_TOUCH_FOLDER "himax"
#else
#define HIMAX_PROC_TOUCH_FOLDER "touchpanel"
#endif
/*ts_work about start*/
struct himax_target_report_data *g_target_report_data;
EXPORT_SYMBOL(g_target_report_data);

static void himax_report_all_leave_event(struct himax_ts_data *ts);
/*ts_work about end*/
static int		HX_TOUCH_INFO_POINT_CNT;

struct filename* (*kp_getname_kernel)(const char *filename);
struct file* (*kp_file_open_name)(struct filename *name,
		int flags, umode_t mode);

unsigned long FW_VER_MAJ_FLASH_ADDR;
EXPORT_SYMBOL(FW_VER_MAJ_FLASH_ADDR);

unsigned long FW_VER_MIN_FLASH_ADDR;
EXPORT_SYMBOL(FW_VER_MIN_FLASH_ADDR);

unsigned long CFG_VER_MAJ_FLASH_ADDR;
EXPORT_SYMBOL(CFG_VER_MAJ_FLASH_ADDR);

unsigned long CFG_VER_MIN_FLASH_ADDR;
EXPORT_SYMBOL(CFG_VER_MIN_FLASH_ADDR);

unsigned long CID_VER_MAJ_FLASH_ADDR;
EXPORT_SYMBOL(CID_VER_MAJ_FLASH_ADDR);

unsigned long CID_VER_MIN_FLASH_ADDR;
EXPORT_SYMBOL(CID_VER_MIN_FLASH_ADDR);
/*unsigned long	PANEL_VERSION_ADDR;*/
uint32_t CFG_TABLE_FLASH_ADDR;
EXPORT_SYMBOL(CFG_TABLE_FLASH_ADDR);

unsigned char IC_CHECKSUM;
EXPORT_SYMBOL(IC_CHECKSUM);

#if defined(HX_EXCP_RECOVERY)
u8 HX_EXCP_RESET_ACTIVATE;
EXPORT_SYMBOL(HX_EXCP_RESET_ACTIVATE);

int hx_EB_event_flag;
EXPORT_SYMBOL(hx_EB_event_flag);

int hx_EC_event_flag;
EXPORT_SYMBOL(hx_EC_event_flag);

int hx_ED_event_flag;
EXPORT_SYMBOL(hx_ED_event_flag);

int g_zero_event_count;

#endif

static bool chip_test_r_flag;
u8 HX_HW_RESET_ACTIVATE;

static uint8_t AA_press;
static uint8_t EN_NoiseFilter;
static uint8_t Last_EN_NoiseFilter;

static int p_point_num = 0xFFFF;
static int probe_fail_flag;
#if defined(HX_USB_DETECT_GLOBAL)|| defined(HX_TP_USB_NOTIFIER)
bool USB_detect_flag = 0;
#endif

#if defined(HX_GESTURE_TRACK)
static int gest_pt_cnt;
static int gest_pt_x[GEST_PT_MAX_NUM];
static int gest_pt_y[GEST_PT_MAX_NUM];
#if !defined(OPPO_PROC_NODE)
static int gest_start_x, gest_start_y, gest_end_x, gest_end_y;
static int gest_width, gest_height, gest_mid_x, gest_mid_y;
#endif
static int hx_gesture_coor[16];
#endif

int g_ts_dbg;
EXPORT_SYMBOL(g_ts_dbg);

/* File node for Selftest, SMWP and HSEN - Start*/
#define HIMAX_PROC_SELF_TEST_FILE	"self_test"
struct proc_dir_entry *himax_proc_self_test_file;

uint8_t HX_PROC_SEND_FLAG;
EXPORT_SYMBOL(HX_PROC_SEND_FLAG);

#if defined(HX_SMART_WAKEUP)
#define HIMAX_PROC_SMWP_FILE "SMWP"
struct proc_dir_entry *himax_proc_SMWP_file;
#define HIMAX_PROC_GESTURE_FILE "GESTURE"
struct proc_dir_entry *himax_proc_GESTURE_file;
uint8_t HX_SMWP_EN;
#if defined(HX_ULTRA_LOW_POWER)
#define HIMAX_PROC_PSENSOR_FILE "Psensor"
struct proc_dir_entry *himax_proc_psensor_file;
#endif
#endif

#if defined(HX_HIGH_SENSE)
#define HIMAX_PROC_HSEN_FILE "HSEN"
struct proc_dir_entry *himax_proc_HSEN_file;
#endif

#if defined(HX_PALM_REPORT)
static int himax_palm_detect(uint8_t *buf)
{
	struct himax_ts_data *ts = private_ts;
	int32_t loop_i;
	int base = 0;
	int x = 0, y = 0, w = 0;

	loop_i = 0;
	base = loop_i * 4;
	x = buf[base] << 8 | buf[base + 1];
	y = (buf[base + 2] << 8 | buf[base + 3]);
	w = buf[(ts->nFinger_support * 4) + loop_i];
	I(" %s HX_PALM_REPORT_loopi=%d,base=%x,X=%x,Y=%x,W=%x\n",
		__func__, loop_i, base, x, y, w);
	if ((!atomic_read(&ts->suspend_mode))
	&& (x == 0xFA5A)
	&& (y == 0xFA5A)
	&& (w == 0x00))
		return PALM_REPORT;
	else
		return NOT_REPORT;
}
#endif

static ssize_t himax_self_test(struct seq_file *s, void *v)
{
	int val = 0x00;
	size_t ret = 0;

	I("%s: enter, %d\n", __func__, __LINE__);

	if (private_ts->suspended == 1) {
		E("%s: please do self test in normal active mode\n", __func__);
		return HX_INIT_FAIL;
	}

	himax_int_enable(0);/* disable irq */

	private_ts->in_self_test = 1;

	val = g_core_fp.fp_chip_self_test(s, v);
/*
 *#if defined(HX_EXCP_RECOVERY)
 *	HX_EXCP_RESET_ACTIVATE = 1;
 *#endif
 *	himax_int_enable(1); //enable irq
 */

	private_ts->in_self_test = 0;

#if defined(HX_EXCP_RECOVERY)
	HX_EXCP_RESET_ACTIVATE = 1;
#endif
	himax_int_enable(1);

	return ret;
}

static void *himax_self_test_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= 1)
		return NULL;


	return (void *)((unsigned long) *pos + 1);
}

static void *himax_self_test_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}

static void himax_self_test_seq_stop(struct seq_file *s, void *v)
{
}

static int himax_self_test_seq_read(struct seq_file *s, void *v)
{
	size_t ret = 0;

	if (chip_test_r_flag) {
#if defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT)
		if (g_rslt_data)
			seq_printf(s, "%s", g_rslt_data);
		else
#endif
			seq_puts(s, "No chip test data.\n");
	} else {
		himax_self_test(s, v);
	}

	return ret;
}

static const struct seq_operations himax_self_test_seq_ops = {
	.start	= himax_self_test_seq_start,
	.next	= himax_self_test_seq_next,
	.stop	= himax_self_test_seq_stop,
	.show	= himax_self_test_seq_read,
};

static int himax_self_test_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &himax_self_test_seq_ops);
};

static ssize_t himax_self_test_write(struct file *filp, const char __user *buff,
			size_t len, loff_t *data)
{
	char buf[80];

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len))
		return -EFAULT;

	if (buf[0] == 'r') {
		chip_test_r_flag = true;
		I("%s: Start to read chip test data.\n", __func__);
	}	else {
		chip_test_r_flag = false;
		I("%s: Back to do self test.\n", __func__);
	}

	return len;
}

static const struct file_operations himax_proc_self_test_ops = {
	.owner = THIS_MODULE,
	.open = himax_self_test_proc_open,
	.read = seq_read,
	.write = himax_self_test_write,
	.release = seq_release,
};

#if defined(HX_HIGH_SENSE)
static ssize_t himax_HSEN_read(struct file *file, char *buf,
		size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	size_t count = 0;
	char *temp_buf = NULL;

	if (!HX_PROC_SEND_FLAG) {
		temp_buf = kcalloc(len, sizeof(char), GFP_KERNEL);
		if (temp_buf != NULL) {
			count = snprintf(temp_buf, PAGE_SIZE, "%d\n",
					ts->HSEN_enable);

			if (copy_to_user(buf, temp_buf, len))
				I("%s, here:%d\n", __func__, __LINE__);

			kfree(temp_buf);
			HX_PROC_SEND_FLAG = 1;
		} else {
			E("%s, Failed to allocate memory\n", __func__);
		}
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return count;
}

static ssize_t himax_HSEN_write(struct file *file, const char *buff,
		size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	char buf[80] = {0};

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len))
		return -EFAULT;

	if (buf[0] == '0')
		ts->HSEN_enable = 0;
	else if (buf[0] == '1')
		ts->HSEN_enable = 1;
	else
		return -EINVAL;

	g_core_fp.fp_set_HSEN_enable(ts->HSEN_enable, ts->suspended);
	I("%s: HSEN_enable = %d.\n", __func__, ts->HSEN_enable);
	return len;
}

static const struct file_operations himax_proc_HSEN_ops = {
	.owner = THIS_MODULE,
	.read = himax_HSEN_read,
	.write = himax_HSEN_write,
};
#endif

#if defined(HX_SMART_WAKEUP)
static ssize_t himax_SMWP_read(struct file *file, char *buf,
		size_t len, loff_t *pos)
{
	size_t count = 0;
	struct himax_ts_data *ts = private_ts;
	char *temp_buf = NULL;

	if (!HX_PROC_SEND_FLAG) {
		temp_buf = kcalloc(len, sizeof(char), GFP_KERNEL);
		if (temp_buf != NULL) {
			count = snprintf(temp_buf, PAGE_SIZE, "%d\n",
					ts->SMWP_enable);

			if (copy_to_user(buf, temp_buf, len))
				I("%s, here:%d\n", __func__, __LINE__);

			kfree(temp_buf);
			HX_PROC_SEND_FLAG = 1;
		} else {
			E("%s, Failed to allocate memory\n", __func__);
		}
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return count;
}

static ssize_t himax_SMWP_write(struct file *file, const char *buff,
		size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	char buf[80] = {0};

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len))
		return -EFAULT;

	if (buf[0] == '0')
		ts->SMWP_enable = 0;
	else if (buf[0] == '1')
		ts->SMWP_enable = 1;
	else
		return -EINVAL;

	g_core_fp.fp_set_SMWP_enable(ts->SMWP_enable, ts->suspended);
	HX_SMWP_EN = ts->SMWP_enable;
	I("%s: SMART_WAKEUP_enable = %d.\n", __func__, HX_SMWP_EN);
	return len;
}

static const struct file_operations himax_proc_SMWP_ops = {
	.owner = THIS_MODULE,
	.read = himax_SMWP_read,
	.write = himax_SMWP_write,
};

static ssize_t himax_GESTURE_read(struct file *file, char *buf,
		size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	int i = 0;
	size_t ret = 0;
	char *temp_buf = NULL;

	if (!HX_PROC_SEND_FLAG) {
		temp_buf = kcalloc(len, sizeof(char), GFP_KERNEL);
		if (temp_buf != NULL) {
			for (i = 0; i < GEST_SUP_NUM; i++)
				ret += snprintf(temp_buf + ret, len - ret,
						"ges_en[%d]=%d\n",
						i, ts->gesture_cust_en[i]);

			if (copy_to_user(buf, temp_buf, len))
				I("%s, here:%d\n", __func__, __LINE__);

			kfree(temp_buf);
			HX_PROC_SEND_FLAG = 1;
		} else {
			E("%s, Failed to allocate memory\n", __func__);
		}
	} else {
		HX_PROC_SEND_FLAG = 0;
		ret = 0;
	}

	return ret;
}

static ssize_t himax_GESTURE_write(struct file *file, const char *buff,
		size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	int i = 0;
	int j = 0;
	char buf[80] = {0};

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len))
		return -EFAULT;

	I("himax_GESTURE_store= %s, len = %d\n", buf, (int)len);

	for (i = 0; i < len; i++) {
		if (buf[i] == '0' && j < GEST_SUP_NUM) {
			ts->gesture_cust_en[j] = 0;
			I("gesture en[%d]=%d\n", j, ts->gesture_cust_en[j]);
			j++;
		} else if (buf[i] == '1' && j < GEST_SUP_NUM) {
			ts->gesture_cust_en[j] = 1;
			I("gesture en[%d]=%d\n", j, ts->gesture_cust_en[j]);
			j++;
		} else
			I("Not 0/1 or >=GEST_SUP_NUM : buf[%d] = %c\n",
				i, buf[i]);
	}

	return len;
}

static const struct file_operations himax_proc_Gesture_ops = {
	.owner = THIS_MODULE,
	.read = himax_GESTURE_read,
	.write = himax_GESTURE_write,
};

#if defined(HX_ULTRA_LOW_POWER)
static ssize_t himax_psensor_read(struct file *file, char *buf,
		size_t len, loff_t *pos)
{
	size_t count = 0;
	struct himax_ts_data *ts = private_ts;
	char *temp_buf = NULL;

	if (!HX_PROC_SEND_FLAG) {
		temp_buf = kcalloc(len, sizeof(char), GFP_KERNEL);
		if (temp_buf != NULL) {
			count = snprintf(temp_buf, PAGE_SIZE,
					"p-sensor flag = %d\n",
					ts->psensor_flag);

			if (copy_to_user(buf, temp_buf, len))
				I("%s, here:%d\n", __func__, __LINE__);

			kfree(temp_buf);
			HX_PROC_SEND_FLAG = 1;
		} else {
			E("%s, Failed to allocate memory\n", __func__);
		}
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return count;
}

static ssize_t himax_psensor_write(struct file *file, const char *buff,
		size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	char buf[80] = {0};

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len))
		return -EFAULT;

	if (buf[0] == '0' && ts->SMWP_enable == 1) {
		ts->psensor_flag = false;
		g_core_fp.fp_black_gest_ctrl(false);
	} else if (buf[0] == '1' && ts->SMWP_enable == 1) {
		ts->psensor_flag = true;
		g_core_fp.fp_black_gest_ctrl(true);
	} else if (ts->SMWP_enable == 0) {
		I("%s: SMWP is disable, not supprot to ctrl p-sensor.\n",
			__func__);
	} else
		return -EINVAL;

	I("%s: psensor_flag = %d.\n", __func__, ts->psensor_flag);
	return len;
}

static const struct file_operations himax_proc_psensor_ops = {
	.owner = THIS_MODULE,
	.read = himax_psensor_read,
	.write = himax_psensor_write,
};
#endif
#endif

int himax_common_proc_init(void)
{
#ifdef OPPO_PROC_NODE
		himax_touch_proc_dir = proc_mkdir(HIMAX_PROC_TOUCH_FOLDER, himax_proc_debug_infor_dir);

		if (himax_touch_proc_dir == NULL) {
			E(" %s: oppo himax_touch_proc_dir file create failed!\n", __func__);
			return -ENOMEM;
		}
#else
		himax_touch_proc_dir = proc_mkdir(HIMAX_PROC_TOUCH_FOLDER, NULL);

		if (himax_touch_proc_dir == NULL) {
			E(" %s: himax_touch_proc_dir file create failed!\n", __func__);
			return -ENOMEM;
		}
#endif

#if defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT)
	if (fp_himax_self_test_init != NULL)
		fp_himax_self_test_init();
#endif

	himax_proc_self_test_file = proc_create(HIMAX_PROC_SELF_TEST_FILE, 0444,
		himax_touch_proc_dir, &himax_proc_self_test_ops);
	if (himax_proc_self_test_file == NULL) {
		E(" %s: proc self_test file create failed!\n", __func__);
		goto fail_1;
	}

#if defined(HX_HIGH_SENSE)
	himax_proc_HSEN_file = proc_create(HIMAX_PROC_HSEN_FILE, 0666,
			himax_touch_proc_dir, &himax_proc_HSEN_ops);

	if (himax_proc_HSEN_file == NULL) {
		E(" %s: proc HSEN file create failed!\n", __func__);
		goto fail_2;
	}

#endif
#if defined(HX_SMART_WAKEUP)
	himax_proc_SMWP_file = proc_create(HIMAX_PROC_SMWP_FILE, 0666,
			himax_touch_proc_dir, &himax_proc_SMWP_ops);

	if (himax_proc_SMWP_file == NULL) {
		E(" %s: proc SMWP file create failed!\n", __func__);
		goto fail_3;
	}

	himax_proc_GESTURE_file = proc_create(HIMAX_PROC_GESTURE_FILE, 0666,
			himax_touch_proc_dir, &himax_proc_Gesture_ops);

	if (himax_proc_GESTURE_file == NULL) {
		E(" %s: proc GESTURE file create failed!\n", __func__);
		goto fail_4;
	}
#if defined(HX_ULTRA_LOW_POWER)
	himax_proc_psensor_file = proc_create(HIMAX_PROC_PSENSOR_FILE, 0666,
			himax_touch_proc_dir, &himax_proc_psensor_ops);

	if (himax_proc_psensor_file == NULL) {
		E(" %s: proc GESTURE file create failed!\n", __func__);
		goto fail_5;
	}
#endif
#endif
	return 0;
#if defined(HX_SMART_WAKEUP)
#if defined(HX_ULTRA_LOW_POWER)
	remove_proc_entry(HIMAX_PROC_PSENSOR_FILE, himax_touch_proc_dir);
fail_5:
#endif
	remove_proc_entry(HIMAX_PROC_GESTURE_FILE, himax_touch_proc_dir);
fail_4:
	remove_proc_entry(HIMAX_PROC_SMWP_FILE, himax_touch_proc_dir);
fail_3:
#endif
#if defined(HX_HIGH_SENSE)
	remove_proc_entry(HIMAX_PROC_HSEN_FILE, himax_touch_proc_dir);
fail_2:
#endif
	remove_proc_entry(HIMAX_PROC_SELF_TEST_FILE, himax_touch_proc_dir);
fail_1:
	return -ENOMEM;
}

void himax_common_proc_deinit(void)
{
	remove_proc_entry(HIMAX_PROC_SELF_TEST_FILE, himax_touch_proc_dir);
#if defined(HX_HIGH_SENSE)
	remove_proc_entry(HIMAX_PROC_HSEN_FILE, himax_touch_proc_dir);
#endif
#if defined(HX_SMART_WAKEUP)
#if defined(HX_ULTRA_LOW_POWER)
	remove_proc_entry(HIMAX_PROC_PSENSOR_FILE, himax_touch_proc_dir);
#endif
	remove_proc_entry(HIMAX_PROC_GESTURE_FILE, himax_touch_proc_dir);
	remove_proc_entry(HIMAX_PROC_SMWP_FILE, himax_touch_proc_dir);
#endif
#ifdef OPPO_PROC_NODE
	remove_proc_entry(HIMAX_PROC_TOUCH_FOLDER, himax_proc_touchpanel_dir);
#else
	remove_proc_entry(HIMAX_PROC_TOUCH_FOLDER, NULL);
#endif
}

/* File node for SMWP and HSEN - End*/

/*  File node for oppo  -- start*/
#if defined(OPPO_PROC_NODE)
static void himax_Freq_hop_test_sub(struct work_struct *work)
{
	I("%s: Entering freq_hop sub! \n", __func__);
	 if(private_ts->Freq_hop_test_stauts == 1){
		private_ts->Freq_hop_test_stauts = 2;
	 	}
	 else{
	 	private_ts->Freq_hop_test_stauts = 1;
	 	}

	 I("%s: tx freq hop test start,MODE= %d \n", __func__, private_ts->Freq_hop_test_stauts);
	g_core_fp.fp_tx_freq_chg_test(private_ts->Freq_hop_test_stauts);

	queue_delayed_work(private_ts->himax_Freq_hop_test_wq, &private_ts->himax_Freq_hop_test_wrok, msecs_to_jiffies(10000));
	private_ts->Freq_hop_delay_work_start =1;

	return;
}
// himax_proc_data_limit_ops start
static ssize_t himax_data_limit_read(struct file *file, char *buf,
									size_t len, loff_t *pos)
{
	ssize_t ret = 0;
	uint8_t loop_i = 0;
	int item_i = 0;
	int item_data_max = -0x80000000;
	int item_data_min = 0x7FFFFFFF;
	int all_mut_len = ic_data->HX_TX_NUM*ic_data->HX_RX_NUM;
	char *temp_buf;

	if (!HX_PROC_SEND_FLAG) {

		ret = himax_self_test_data_init();
		if(ret)
		{
			I("read criteria file fail %s,here:%d\n", __func__, __LINE__);
			return 0;
		}

		temp_buf = kzalloc(len, GFP_KERNEL);
		if(temp_buf == NULL)
		{
			I("loc memory fail %s,here:%d\n", __func__, __LINE__);
			himax_self_test_data_deinit();
			return 0;
		}
		for (loop_i = 0; loop_i < HX_CRITERIA_SIZE; loop_i++)
		{
			item_data_max = -0x80000000;
			item_data_min = 0x7FFFFFFF;
			for(item_i = 0; item_i <all_mut_len; item_i++)
			{
				if(g_inspection_criteria[loop_i][item_i]>item_data_max)
				{
					item_data_max = g_inspection_criteria[loop_i][item_i];
				}

				if(g_inspection_criteria[loop_i][item_i]<item_data_min)
				{
					item_data_min = g_inspection_criteria[loop_i][item_i];
				}
			}
			ret += snprintf(temp_buf + ret, len - ret, "%s: (%d -- %d)\n", g_hx_inspt_crtra_name[loop_i], item_data_min, item_data_max);
		}

		//ret += snprintf(temp_buf + ret, len - ret, "\n");

		if (copy_to_user(buf, temp_buf, len))
			I("%s,here:%d\n", __func__, __LINE__);

		kfree(temp_buf);
		himax_self_test_data_deinit();
		HX_PROC_SEND_FLAG = 1;
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return ret;
}


static struct file_operations himax_proc_data_limit_ops = {
	.owner = THIS_MODULE,
	.read = himax_data_limit_read,
};

// himax_proc_data_limit_ops end
//himax_proc_main_reg_ops start
static ssize_t himax_main_reg_read(struct file *file, char *buf, /*main_register*/
									size_t len, loff_t *pos)
{
	ssize_t ret = 0;
	size_t reg_ary_len;
	uint8_t loop_i = 0;
	uint8_t addr[4] = {0};
	uint8_t data[4] = {0};
	char *temp_buf;

	if (!HX_PROC_SEND_FLAG) {
		temp_buf = kzalloc(len, GFP_KERNEL);
		reg_ary_len = (size_t)(sizeof(dbg_reg_ary)/sizeof(uint32_t));

		for (loop_i = 0; loop_i < reg_ary_len; loop_i++) {
			himax_parse_assign_cmd(dbg_reg_ary[loop_i], addr, 4);
			g_core_fp.fp_register_read(addr, DATA_LEN_4, data, 0);

			ret += snprintf(temp_buf + ret, len - ret,
			"reg[0-3] : 0x%08X = 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
			dbg_reg_ary[loop_i], data[0], data[1], data[2], data[3]);
			I("reg[0-3] : 0x%08X = 0x%02X, 0x%02X, 0x%02X, 0x%02X\n",
			dbg_reg_ary[loop_i], data[0], data[1], data[2], data[3]);
		}
		if (copy_to_user(buf, temp_buf, len))
			I("%s,here:%d\n", __func__, __LINE__);

		kfree(temp_buf);
		HX_PROC_SEND_FLAG = 1;
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return ret;
}

static struct file_operations himax_proc_main_reg_ops = {
	.owner = THIS_MODULE,
	.read = himax_main_reg_read,
};
//himax_proc_main_reg_ops end
// himax_proc_baseline_ops start

static void *himax_oppo_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= 1) {
		return NULL;
	}
	return (void *)((unsigned long) *pos + 1);
}

static void *himax_oppo_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}

static void himax_oppo_seq_stop(struct seq_file *s, void *v)
{
}
static int himax_oppo_baseline_read(struct seq_file *s, void *v) /*baseline*/
{
	size_t ret = 0;

	ret = oppo_baseline_read ( s );
	return ret;
}

static struct seq_operations himax_oppo_baseline_seq_ops = {
	.start	= himax_oppo_seq_start,
	.next	= himax_oppo_seq_next,
	.stop	= himax_oppo_seq_stop,
	.show	= himax_oppo_baseline_read,
};

static int himax_oppo_baseline_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &himax_oppo_baseline_seq_ops);
};

static struct file_operations himax_proc_baseline_ops = {
	.owner = THIS_MODULE,
	.open = himax_oppo_baseline_proc_open,
	.read = seq_read,
};
// himax_proc_baseline_ops end

//himax_proc_delta_ops start

static int himax_oppo_delta_read(struct seq_file *s, void *v) /*delta*/
{
	size_t ret = 0;

	ret = oppo_delta_read_sub( s );

	return ret;
}
static struct seq_operations himax_oppo_delta_seq_ops = {
	.start	= himax_oppo_seq_start,
	.next	= himax_oppo_seq_next,
	.stop	= himax_oppo_seq_stop,
	.show	= himax_oppo_delta_read,
};

static int himax_oppo_delta_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &himax_oppo_delta_seq_ops);
};

static struct file_operations himax_proc_delta_ops = {
	.owner = THIS_MODULE,
	.open = himax_oppo_delta_proc_open,
	.read = seq_read,
};
//himax_proc_delta_ops end

#if defined(HX_SMART_WAKEUP)
// himax_black_screen_test_fops start
static ssize_t himax_black_screen_test(struct seq_file *s, void *v)
{
	int val = 0x00;
	size_t ret = 0;
	I("%s: enter, %d\n", __func__, __LINE__);

	if (private_ts->suspended == 0) {
		E("%s: please do self test in black screen mode\n", __func__);
		return HX_INIT_FAIL;
	}

	himax_int_enable(0);/* disable irq */

	private_ts->in_self_test = 1;

	val = himax_black_chip_self_test(s, v);

	private_ts->in_self_test = 0;

	himax_int_enable(1);

	tp_gesture = private_ts->gesture_bak;
	private_ts->SMWP_enable = private_ts->backup_flag;
	return ret;
}

static void *himax_black_screen_test_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= 1)
		return NULL;


	return (void *)((unsigned long) *pos + 1);
}

static void *himax_black_screen_test_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}

static void himax_black_screen_test_seq_stop(struct seq_file *s, void *v)
{
}

static int himax_black_screen_test_seq_read(struct seq_file *s, void *v)
{
	size_t ret = 0;

	if (chip_test_r_flag) {
#if defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT)
		if (g_rslt_data)
			seq_printf(s, "%s", g_rslt_data);
		else
#endif
			seq_puts(s, "No chip test data.\n");
	} else {
		himax_black_screen_test(s, v);
	}

	return ret;
}

static const struct seq_operations himax_black_screen_test_seq_ops = {
	.start	= himax_black_screen_test_seq_start,
	.next	= himax_black_screen_test_seq_next,
	.stop	= himax_black_screen_test_seq_stop,
	.show	= himax_black_screen_test_seq_read,
};

static int himax_black_screen_test_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &himax_black_screen_test_seq_ops);
};

static ssize_t himax_black_screen_test_write(struct file *filp, const char __user *userbuf,
			size_t len, loff_t *data)
{
	int value = 0;
	char buf[4] = {0};
	struct himax_ts_data *ts = private_ts;

	if ( copy_from_user(buf, userbuf, len) ) {
		E("%s: copy from user error.", __func__);
		return -1;
	}
	sscanf(buf, "%d", &value);

	I("value ============%d\n",value);

	ts->backup_flag = ts->SMWP_enable;
	ts->gesture_bak = tp_gesture;
	ts->SMWP_enable = 1;
	tp_gesture = 1;
	ts->in_self_test = value;
	return len;
}

static const struct file_operations himax_black_screen_test_fops = {
	.owner = THIS_MODULE,
	.open = himax_black_screen_test_open,
	.read = seq_read,
	.write = himax_black_screen_test_write,
	.release = seq_release,
};
// himax_black_screen_test_fops end
#endif
// himax_proc_baseline_test_ops start
static ssize_t himax_baseline_test(struct seq_file *s, void *v)
{
	int val = 0x00;
	size_t ret = 0;

	I("%s: enter, %d\n", __func__, __LINE__);

	if (private_ts->suspended == 1) {
		E("%s: please do self test in normal active mode\n", __func__);
		return HX_INIT_FAIL;
	}

	himax_int_enable(0);/* disable irq */

	private_ts->in_self_test = 1;

	val = g_core_fp.fp_chip_self_test(s, v);

	//private_ts->in_self_test = 0;

	himax_int_enable(1);

	return ret;
}

static void *himax_baseline_test_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= 1)
		return NULL;


	return (void *)((unsigned long) *pos + 1);
}

static void *himax_baseline_test_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}

static void himax_baseline_test_seq_stop(struct seq_file *s, void *v)
{
}

static int himax_baseline_test_seq_read(struct seq_file *s, void *v)
{
	size_t ret = 0;

	if (chip_test_r_flag) {
#if defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT)
		if (g_rslt_data)
			seq_printf(s, "%s", g_rslt_data);
		else
#endif
			seq_puts(s, "No chip test data.\n");
	} else {
		himax_baseline_test(s, v);
	}

	return ret;
}

static const struct seq_operations himax_baseline_test_seq_ops = {
	.start	= himax_baseline_test_seq_start,
	.next	= himax_baseline_test_seq_next,
	.stop	= himax_baseline_test_seq_stop,
	.show	= himax_baseline_test_seq_read,
};

static int himax_proc_baseline_test_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &himax_baseline_test_seq_ops);
};

static ssize_t himax_proc_baseline_write(struct file *filp, const char __user *buff,
			size_t len, loff_t *data)
{
	char buf[80];

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len))
		return -EFAULT;

	if (buf[0] == 'r') {
		chip_test_r_flag = true;
		I("%s: Start to read chip test data.\n", __func__);
	}	else {
		chip_test_r_flag = false;
		I("%s: Back to do self test.\n", __func__);
	}

	return len;
}

static const struct file_operations himax_proc_baseline_test_ops = {
	.owner = THIS_MODULE,
	.open = himax_proc_baseline_test_open,
	.read = seq_read,
	.write = himax_proc_baseline_write,
	.release = seq_release,
};

// himax_proc_baseline_test_ops end

//himax_proc_oppo_tx_hop_op start
static ssize_t oppo_io_ctrl_read(struct file *file, char *buf,
							   size_t len, loff_t *pos)
{
	size_t count = 0;
	struct himax_ts_data *ts = private_ts;
	char *temp_buf;

	if (!HX_PROC_SEND_FLAG) {
		temp_buf = kzalloc(len, GFP_KERNEL);
		count = snprintf(temp_buf, PAGE_SIZE, "%d\n", ts->Freq_hop_test_stauts);

		if (copy_to_user(buf, temp_buf, len))
			I("%s,here:%d\n", __func__, __LINE__);

		kfree(temp_buf);
		HX_PROC_SEND_FLAG = 1;
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return count;
}

static ssize_t himax_oppo_hop_test_write(struct file *file, const char *buff,/*oppo_io_ctrl*/
								size_t len, loff_t *pos)
{
	char buf[80] = {0};

	if ( *pos ) {
		E("is already read the file\n");
		return 0;
	}
	*pos += len;

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len)) {
		return -EFAULT;
	}

	if (buf[0] == '1') {
		if ( private_ts->Freq_hop_delay_work_start == 0) {
		private_ts->Freq_hop_delay_work_start =1;
		private_ts->Freq_hop_test_stauts = 1;
		queue_delayed_work(private_ts->himax_Freq_hop_test_wq, &private_ts->himax_Freq_hop_test_wrok, msecs_to_jiffies(10000));
		I("%s: tx freq hop test start,status= %c \n", __func__, buf[0]);
		}
	} else{
		if ( private_ts->Freq_hop_delay_work_start == 1 ) {
		cancel_delayed_work_sync(&private_ts->himax_Freq_hop_test_wrok);
		flush_workqueue(private_ts->himax_Freq_hop_test_wq);
		g_core_fp.fp_tx_freq_chg_test(0);
		I("%s: tx freq hop test close,status= %c \n", __func__, buf[0]);
		private_ts->Freq_hop_delay_work_start = 0;
		private_ts->Freq_hop_test_stauts = 0;
		}
	}

	return len;
}

static struct file_operations himax_proc_oppo_tx_hop_ops = {
	.owner = THIS_MODULE,
	.read = oppo_io_ctrl_read,
	.write = himax_oppo_hop_test_write,
};

//himax_proc_oppo_tx_hop_op end

/* incell_panel	Start*/

static ssize_t himax_incell_panel_read(struct file	*file, char	__user *user_buf,	size_t count,	loff_t *ppos)
{
	ssize_t	ret	=	0;
	char* temp_buf = NULL;
	unsigned char temp_data =0;

	if(count < 3)
	{
		I("%s,here:%d count=%d < 3\n", __func__, __LINE__,(int)count);
		return count;
	}

	if (!HX_PROC_SEND_FLAG) {
		if(strncmp (private_ts->chip_name,  HX_83102D_SERIES_PWON,  30))
		{
			temp_data=0;
		}
		else
		{
			temp_data=1;//is incell
		}

		temp_buf = kzalloc(count, GFP_KERNEL);
		if(temp_buf ==NULL)
		{
			return -1;
		}
		ret = snprintf(temp_buf, PAGE_SIZE, "%d\n", temp_data);

		if (copy_to_user(user_buf, temp_buf, count))
			I("%s,here:%d\n", __func__, __LINE__);

		kfree(temp_buf);
		HX_PROC_SEND_FLAG = 1;
	} else {
		HX_PROC_SEND_FLAG = 0;
	}
	return ret;
}


static struct file_operations himax_proc_incell_panel_ops = {
	.owner = THIS_MODULE,
	.open	 = simple_open,
	.read = himax_incell_panel_read,
};

/* incell_panel	End*/
/* irq depth	Start*/
static ssize_t himax_irq_depth_read(struct file *file, char *buf,
								  size_t len, loff_t *pos)
{
	size_t ret = 0;
	char *temp_buf;
	//struct irq_desc *desc = irq_to_desc(private_ts->pdata->gpio_irq);//wdd modify
	extern unsigned int himax_touch_irq;
	struct irq_desc *desc = irq_to_desc(himax_touch_irq);
	I("%s: enter, %d \n", __func__, __LINE__);

	if (!HX_PROC_SEND_FLAG) {
		temp_buf = kzalloc(len, GFP_KERNEL);
		ret += snprintf(temp_buf + ret, len - ret, "now depth=%d\n", desc->depth);

		if (copy_to_user(buf, temp_buf, len))
			I("%s,here:%d\n", __func__, __LINE__);

		kfree(temp_buf);
		HX_PROC_SEND_FLAG = 1;
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return ret;
}

static ssize_t himax_irq_depth_write(struct file *file, const char *buff,
								   size_t len, loff_t *pos)
{

	I("%s:Nothing to be done!\n", __func__);
	return len;
}

static struct file_operations himax_proc_irq_depth_ops = {
	.owner = THIS_MODULE,
	.read = himax_irq_depth_read,
	.write = himax_irq_depth_write,
};

/* irq depth	End*/

/*oppo debug level 	Start*/
static ssize_t himax_oppo_debug_level_read(struct file *file, char *buf,
								  size_t len, loff_t *pos)
{
	size_t ret = 0;
	char temp_buf[8];
	I("%s: enter, %d \n", __func__, __LINE__);

	if (!HX_PROC_SEND_FLAG) {
		ret = snprintf(temp_buf, sizeof(temp_buf), "%d\n", g_oppo_debug_level);
		if (copy_to_user(buf, temp_buf, ret))
			I("%s,here:%d\n", __func__, __LINE__);

		HX_PROC_SEND_FLAG = 1;
	} else {
		HX_PROC_SEND_FLAG = 0;
	}

	return ret;
}


static ssize_t himax_oppo_debug_level_write(struct file *file, const char *buff,
								   size_t len, loff_t *pos)
{

    char buf[80] = {0};
    int32_t dbgLevel = 0;
    int32_t ret = 0;

    if (len >= 80)
    {
        I("%s: no command exceeds 80 chars.\n", __func__);
        return -EFAULT;
    }
    if (copy_from_user(buf, buff, len))
    {
        return -EFAULT;
    }

    ret = sscanf(buf, "%d", &dbgLevel);

    if (ret >= 1) {
        g_oppo_debug_level = 0;
        switch(dbgLevel) {
            case 2:
                g_oppo_debug_level	= dbgLevel & BIT(1);
                break;
            case 1:
            case 0:
                g_oppo_debug_level = dbgLevel & BIT(0);
                break;
            default:
            g_oppo_debug_level = 0;
        };
    }
    I("%s: buf = %s, debug_level = %d\n", __func__, buf, g_oppo_debug_level);

    return len;
}

static struct file_operations himax_proc_oppo_debug_level_ops = {
	.owner = THIS_MODULE,
	.read = himax_oppo_debug_level_read,
	.write = himax_oppo_debug_level_write,
};

/* oppo debug level	End*/

/* oppo_tp_direction	Start*/
static ssize_t himax_oppo_tp_direction_write(struct file *file, const char *buff,
									size_t len, loff_t *pos)
{
	char buf_tmp[4] = {0};
	int ret = 0;
	struct himax_ts_data *ts = private_ts;
	if (len >= 4) {
		I("%s: no command exceeds 4 chars.\n", __func__);
		return -EFAULT;
	}
	if (!ts)
		return len;

	if (copy_from_user(buf_tmp, buff, len)) {
		return -EFAULT;
	}

	I("%s: ap send %c.\n", __func__, buf_tmp[0]);
	ret = sscanf(buf_tmp, "%d", &ts->touch_direction);
	if (ret == 0) {
		I("%s: char to int fail.\n", __func__);
	}

	if (buf_tmp[0] == '0') {
		himax_parse_assign_cmd(fw_oppo_tp_direction_0_pwd, buf_tmp, sizeof(buf_tmp));
		g_core_fp.fp_register_write(pfw_op->addr_oppo_tp_direction/*0x10007f3c*/, DATA_LEN_4, buf_tmp, 0);
		I("%s: oppo_tp_direction disable .\n", __func__);
	}
	else if(buf_tmp[0] == '1')
	{
		himax_parse_assign_cmd(fw_oppo_tp_direction_1_pwd, buf_tmp, sizeof(buf_tmp));
		g_core_fp.fp_register_write(pfw_op->addr_oppo_tp_direction, DATA_LEN_4, buf_tmp, 0);
		I("%s: oppo_tp_direction enable .\n", __func__);
	}
	else if(buf_tmp[0] == '2')
	{
		himax_parse_assign_cmd(fw_oppo_tp_direction_2_pwd, buf_tmp, sizeof(buf_tmp));
		g_core_fp.fp_register_write(pfw_op->addr_oppo_tp_direction, DATA_LEN_4, buf_tmp, 0);
		I("%s: oppo_tp_direction enable .\n", __func__);
	}
	else {
		I("%s: oppo_tp_direction wrong parameter .\n", __func__);
		return -EINVAL;
	}

	return len;
}


static ssize_t proc_dir_control_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    ssize_t ret = 0;
    char page[PAGESIZE];
    struct himax_ts_data *ts = private_ts;

    if (!ts)
        return count;

    sprintf(page, "%d\n", ts->touch_direction);
    ret = simple_read_from_buffer(user_buf, count, ppos, page, strlen(page));

    return ret;
}

static struct file_operations himax_proc_oppo_tp_direction_ops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = proc_dir_control_read,
	.write = himax_oppo_tp_direction_write,
};
/* oppo_tp_direction	End*/

/* Game Switch Enable Start*/
static ssize_t hx_game_switch_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char buf[8] = {0};
	int temp;
	bool ret;
	struct himax_ts_data *ts = private_ts;

	if (!ts)
		return count;

	if (count > 3)
		count = 3;
	if (copy_from_user(buf, buffer, count)) {
		E("%s: read proc input error.\n", __func__);
		return count;
	}

	sscanf(buf, "%x", &temp);
	ts->game_switch = temp;

	if (temp != 0) {
		I("%s:Touchpanel game switch enable %d\n", __func__, temp);
		ret = g_core_fp.fp_no_jiter_en(1);
		if (ret == false) {
			I("%s:Touchpanel enable game switch failed\n", __func__);
		}
	} else {
		I("%s:Touchpanel game switch disable %d\n", __func__, temp);
		ret = g_core_fp.fp_no_jiter_en(0);
		if (ret == false) {
			I("%s:Touchpanel disable game switch failed\n", __func__);
		}
	}

	return count;
}

static ssize_t proc_game_switch_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	ssize_t ret = 0;
	char page[PAGESIZE];
	struct himax_ts_data *ts = private_ts;

	if (!ts)
		return count;
	I("game_switch is:%d\n", ts->game_switch);
	ret = snprintf(page, PAGE_SIZE, "%d\n", ts->game_switch);
	ret = simple_read_from_buffer(user_buf,count, ppos, page, strlen(page));

	return ret;
}

static const struct	file_operations	hx_proc_game_switch_ops	= {
	.write = hx_game_switch_write,
	.read	 = proc_game_switch_read,
	.open	 = simple_open,
	.owner = THIS_MODULE,
};
/* Game	Switch Enable	End*/

//proc_limit_area_ops_start
static ssize_t proc_limit_area_read(struct file	*file, char	__user *user_buf,	size_t count,	loff_t *ppos)
{
	ssize_t	ret	=	0;
	char page[PAGESIZE];
	struct himax_ts_data *ts = private_ts;

	if (!ts)
		return count;
	I("limit_area is: %d\n", ts->edge_limit.limit_area);
	ret	=	snprintf(page, PAGE_SIZE,"limit_area=%d left_x1=%d right_x1=%d left_x2=%d right_x2=%d left_y1=%d right_y1=%d left_y2=%d right_y2=%d\n",
					ts->edge_limit.limit_area, ts->edge_limit.left_x1, ts->edge_limit.right_x1,	ts->edge_limit.left_x2,	ts->edge_limit.right_x2,
					ts->edge_limit.left_y1,	ts->edge_limit.right_y1, ts->edge_limit.left_y2, ts->edge_limit.right_y2);
	ret	=	simple_read_from_buffer(user_buf,	count, ppos, page, strlen(page));

	return ret;
}

static ssize_t proc_limit_area_write(struct	file *file,	const	char __user	*buffer, size_t	count, loff_t	*ppos)
{
	char buf[8];
	int	temp;
	struct himax_ts_data *ts = private_ts;

	if (!ts)
			return count;

	if (copy_from_user(buf,	buffer,	count))	{
			I("%s: read	proc input error.\n",	__func__);
			return count;
	}

	sscanf(buf,	"%d",	&temp);
	if (temp < 0 ||	temp > 10)
		return count;

	ts->edge_limit.limit_area	=	temp;
	ts->edge_limit.left_x1		=	(ts->edge_limit.limit_area*1000)/100;
	ts->edge_limit.right_x1		=	ic_data->HX_X_RES	-	ts->edge_limit.left_x1;
	ts->edge_limit.left_x2		=	2	*	ts->edge_limit.left_x1;
	ts->edge_limit.right_x2		=	ic_data->HX_X_RES	-	(2 * ts->edge_limit.left_x1);
	/*ts->edge_limit.right_x1		=	ic_data->HX_X_RES	-	ts->edge_limit.left_x1;
	ts->edge_limit.left_x2		=	2	*	ts->edge_limit.left_x1;
	ts->edge_limit.right_x2		=	ic_data->HX_X_RES	-	(2 * ts->edge_limit.left_x1);*/

	I("limit_area	=	%d;	left_x1	=	%d;	right_x1 = %d; left_x2 = %d; right_x2	=	%d\n",
				 ts->edge_limit.limit_area,	ts->edge_limit.left_x1,	ts->edge_limit.right_x1, ts->edge_limit.left_x2, ts->edge_limit.right_x2);

	ts->edge_limit.left_y1		=	(ts->edge_limit.limit_area*1000)/100;
	ts->edge_limit.right_y1		=	ts->edge_limit.left_y1;
	ts->edge_limit.left_y2		=	ic_data->HX_Y_RES	-	2	*	ts->edge_limit.left_y1;
	ts->edge_limit.right_y2		=	ic_data->HX_Y_RES	-	(2 * ts->edge_limit.left_y1);
	/*ts->edge_limit.right_y1		=	ic_data->HX_Y_RES	-	ts->edge_limit.left_y1;
	ts->edge_limit.left_y2		=	2	*	ts->edge_limit.left_y1;
	ts->edge_limit.right_y2		=	ic_data->HX_Y_RES	-	(2 * ts->edge_limit.left_y1);*/

	I("limit_area =%d; left_y1=%d; right_y1 =%d; left_y2 =%d; right_y2=%d\n",ts->edge_limit.limit_area, ts->edge_limit.left_y1, ts->edge_limit.right_y1,ts->edge_limit.left_y2,ts->edge_limit.right_y2);
	return count;
}

static const struct	file_operations	proc_limit_area_ops	= {
		.read	 = proc_limit_area_read,
		.write = proc_limit_area_write,
		.open	 = simple_open,
		.owner = THIS_MODULE,
};

//proc_limit_area_ops_end


//hx_proc_limit_control_ops_start
static ssize_t hx_limit_ctrl_read(struct file	*file, char	__user *user_buf,	size_t count,	loff_t *ppos)
{
	ssize_t	ret	=	0;
	char page[PAGESIZE];
	struct himax_ts_data *ts = private_ts;

	if (!ts)
			return count;

	I("limit_enable:%d, ts->limit_edge = 0x%x, ts->limit_corner=0x%x\n", ts->limit_enable, ts->limit_edge, ts->limit_corner);
	ret	=	snprintf(page, PAGESIZE,	"%d\n",	ts->limit_enable);

	ret	=	simple_read_from_buffer(user_buf,	count, ppos, page, strlen(page));

	return ret;
}

static ssize_t hx_limit_ctrl_write(struct	file *file,	const	char __user	*buffer, size_t	count, loff_t	*ppos)
{
	char buf[8]	=	{0};
	int	temp;
	bool ret;
	struct himax_ts_data *ts = private_ts;

	if (!ts)
			return count;

	if (count	>	3)
			count	=	3;
	if (copy_from_user(buf,	buffer,	count))	{
			E("%s: read	proc input error.\n",	__func__);
			return count;
	}

	sscanf(buf,	"%x",	&temp);
	if (temp > 0x1F) {
			I("%s: temp	=0x%x > 0x1F\n", __func__, temp);
			return count;
	}

	ts->limit_enable = temp;
	ts->limit_edge = ts->limit_enable	&	1;		/*bit0 to	control	the	feature	of edge	limit*/
	ts->limit_corner = ts->limit_enable	>> 1;	/*control	the	corner points	is release or	not*/

	I("%s: limit_enable	=0x%x,ts->limit_edge = 0x%x, ts->limit_corner=0x%x\n", __func__, ts->limit_enable, ts->limit_edge, ts->limit_corner);

	if(ts->limit_edge == 0)
	{
		I("%s: oppo send cmd 0 firmware will action in oppo_tp_direction\n", __func__);
		return count;
	}

	if (ts->suspended	== false)	{
		ret	=	g_core_fp.fp_edge_limit_en(ts->limit_edge);
			if (ret	== false)	{
					I("%s, Touchpanel operate mode switch failed\n", __func__);
			}
	}

	return count;
}

static const struct	file_operations	hx_proc_limit_control_ops	= {
	.read	 = hx_limit_ctrl_read,
	.write = hx_limit_ctrl_write,
	.open	 = simple_open,
	.owner = THIS_MODULE,
};

//hx_proc_limit_control_ops_end
//himax_double_tap_en start
#ifdef HX_SMART_WAKEUP
static ssize_t himax_double_tap_en_read(struct file *file,char __user *user_buf,size_t count,loff_t *ppos)
{
	struct himax_ts_data *ts = private_ts;
	size_t ret = 0;
	char page[PAGESIZE];

	if (!ts)
		return count;

	I("SMWP_enabl =: %d\n", ts->SMWP_enable);
	ret = snprintf(page, PAGESIZE,"%d\n", ts->SMWP_enable);
	ret = simple_read_from_buffer(user_buf,count, ppos, page, strlen(page));

	return ret;
}

static ssize_t himax_double_tap_en_write(struct file *file, const char *buff,/*double_tap_enable*/
								size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	char buf[80] = {0};
	int i = 0;
	/*if(ts->suspended) {
		I("%s: is already suspend\n", __func__);
		return -1;
	}*/

	if ( *pos ) {
    	E("is already read the file\n");
    	return 0;
	}
    *pos += len;

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len)) {
		return -EFAULT;
	}

	if (buf[0] == '0') {
		ts->SMWP_enable = 0;
		tp_gesture = 0;
		ts->gesture_cust_en[0] = 0;
	} else if (buf[0] == '1') {
		if (ts->SMWP_enable == 0 && ts->psensor_stus == 1) {
			mutex_lock(&private_ts->fw_update_lock);
			I("%s: Psensor far away +++++.\n", __func__);
			ts->SMWP_enable = 1;
			tp_gesture = 1;
			ts->psensor_stus = 0;
			g_core_fp.fp_black_gest_en(true);
			//g_core_fp.fp_set_SMWP_enable(ts->SMWP_enable, ts->suspended);
			mutex_unlock(&private_ts->fw_update_lock);
			if (ts->suspended == true)
				return len;
		}
		ts->SMWP_enable = 1;
		tp_gesture = 1;
		for (i = 0; i < 16; i++) {
			ts->gesture_cust_en[i] = 1;
		}
	}	else if (buf[0] == '2') {
			if (ts->SMWP_enable == 1 && ts->psensor_stus == 0) {
				mutex_lock(&private_ts->fw_update_lock);
				I("%s: Psensor near by ----.\n", __func__);
				ts->SMWP_enable = 0;
				ts->psensor_stus = 1;
				g_core_fp.fp_black_gest_en(false);
				mutex_unlock(&private_ts->fw_update_lock);
				return len;
			} else {
				return -EINVAL;
			}
	}	else

		return -EINVAL;

	g_core_fp.fp_set_SMWP_enable(ts->SMWP_enable, ts->suspended);
	I("%s: SMART_WAKEUP_enable = %d\n", __func__, ts->SMWP_enable);
	return len;
}

static struct file_operations himax_proc_double_tap_en_ops = {
	.owner = THIS_MODULE,
	.read = himax_double_tap_en_read,
	.write = himax_double_tap_en_write,
};
//himax_double_tap_en end
// himax_proc_coor_start
#ifdef HX_GESTURE_TRACK
/*oppo*/
static void *himax_coor_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}
static void *himax_coor_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}
static void himax_coor_stop(struct seq_file *m, void *v)
{
	return;
}

static int32_t himax_coor_show(struct seq_file *m, void *v)
{
    int i = 0;
    //size_t ret = 0;
	char temp_buf[256] = {0};
    //uint16_t len = sizeof(temp_buf);

	I("%s enter.\n",__func__);//,len);

		for (i = 0; i < 8; i++) {
			if (i == 0) {
				//ret += snprintf(temp_buf + ret, len - ret, "%d,", hx_gesture_coor[i]);
				I("hx_gesture_coor[%d] = %d \n",i,hx_gesture_coor[i]);
			} else if (i == 7) {
				//ret += snprintf(temp_buf + ret, len - ret, "%d\n", hx_gesture_coor[i * 2 - 1]);
				I("hx_gesture_coor[%d] = %d \n",i * 2 - 1,hx_gesture_coor[i * 2 - 1]);
			} else {
				//ret += snprintf(temp_buf + ret, len - ret, "%d:", hx_gesture_coor[i * 2 - 1]);
				//ret += snprintf(temp_buf + ret, len - ret, "%d,", hx_gesture_coor[i * 2]);
				I("hx_gesture_coor[%d] = %d \n",i * 2 - 1,hx_gesture_coor[i * 2 - 1]);
				I("hx_gesture_coor[%d] = %d \n",i * 2,hx_gesture_coor[i * 2]);
			}
		}

		sprintf(temp_buf, "%d,%d:%d,%d:%d,%d:%d,%d:%d,%d:%d,%d:%d,%d",
			hx_gesture_coor[0],
			hx_gesture_coor[1], hx_gesture_coor[2],
			hx_gesture_coor[3], hx_gesture_coor[4],
			hx_gesture_coor[5], hx_gesture_coor[6],
			hx_gesture_coor[7], hx_gesture_coor[8],
			hx_gesture_coor[9], hx_gesture_coor[10],
			hx_gesture_coor[11], hx_gesture_coor[12],
			hx_gesture_coor[13]);

		/* oppo gesture formate */
		seq_printf(m, "%s\n", temp_buf);
	I("%s end.\n",__func__);
	return 0;
}

const struct seq_operations oppo_coord_seq_ops =
{
    .start  = himax_coor_start,
    .next   = himax_coor_next,
    .stop   = himax_coor_stop,
    .show   = himax_coor_show,
};

static int32_t himax_diag_coor_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &oppo_coord_seq_ops);
}

static struct file_operations himax_proc_coor_ops = {
	.owner = THIS_MODULE,
	.open = himax_diag_coor_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
#endif
#endif
// himax_proc_coor_end

//himax_proc_tp_fw_update start
static ssize_t himax_tp_fw_update_write(struct file *file, const char __user *page, size_t size, loff_t *lo)
{
	struct himax_ts_data *ts = private_ts;
    int val = 0;
    int ret = 0;
    char buf[4] = {0};
    if (!ts)
        return size;
    if (size > 2)
        return size;

 #ifdef CONFIG_TOUCHPANEL_MTK_PLATFORM
    if (ts->boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT)
#else
    if (ts->boot_mode == MSM_BOOT_MODE__CHARGE)
#endif
    {
        I("boot mode is MSM_BOOT_MODE__CHARGE,not need update tp firmware\n");
        return size;
    }

    if (copy_from_user(buf, page, size)) {
        I("%s: read proc input error.\n", __func__);
        return size;
    }

    sscanf(buf, "%d", &val);
    ts->firmware_update_type = val;
    if ( !g_boot_upgrade_flag && ts->firmware_update_type != 2)
		 ts->force_update = !!val;

    schedule_work(&ts->fw_update_work);

    ret = wait_for_completion_killable_timeout(&ts->fw_complete, FW_UPDATE_COMPLETE_TIMEOUT);
    if (ret < 0) {
        I("kill signal interrupt\n");
    }

    I("fw update finished\n");
    return size;
}

static struct file_operations himax_proc_tp_fw_update_ops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = himax_tp_fw_update_write,
};
//himax_proc_tp_fw_update end
//HIMAX_PROC_OPPO_REG_INFO START
static ssize_t himax_oppo_reg_read_read(struct file *file, char *buf, size_t len, loff_t *pos)
{
	int ret = 0;
	uint16_t loop_i;
	uint8_t data[128];
	char *temp_buf;

	if ( *pos ) {
    	E("%s,is already read the file\n",__func__);
    	return 0;
	}
    *pos += len;
	memset(data, 0x00, sizeof(data));
	temp_buf = kzalloc(len, GFP_KERNEL);
	if (temp_buf == NULL) {
		E("%s: allocate memory fail!!!\n", __func__);
		return 0;
	}

	I("himax_register_show: %02X,%02X,%02X,%02X\n", reg_cmd[3], reg_cmd[2], reg_cmd[1], reg_cmd[0]);
	g_core_fp.fp_register_read(reg_cmd, 128, data, cfg_flag);
	ret += snprintf(temp_buf + ret, len - ret, "command:  %02X,%02X,%02X,%02X\n", reg_cmd[3], reg_cmd[2], reg_cmd[1], reg_cmd[0]);

	for (loop_i = 0; loop_i < 128; loop_i++) {
		ret += snprintf(temp_buf + ret, len - ret, "0x%2.2X ", data[loop_i]);
		if ((loop_i % 16) == 15)
			ret += snprintf(temp_buf + ret, len - ret, "\n");
	}

	ret += snprintf(temp_buf + ret, len - ret, "\n");

	if (copy_to_user(buf, temp_buf, len))
		I("%s,here:%d\n", __func__, __LINE__);
	kfree(temp_buf);
	return ret;
}

static ssize_t himax_oppo_reg_read_write(struct file *file, const char *buff,
		size_t len, loff_t *pos)
{
	char buf[80] = {0};
	char buf_tmp[16];
	uint8_t length = 0;
	unsigned long result	= 0;
	uint8_t loop_i			= 0;
	uint16_t base			= 2;
	char *data_str = NULL;
	uint8_t w_data[20];
	uint8_t x_pos[20];
	uint8_t count = 0;

	if ( *pos ) {
    	E("%s,is already read the file\n",__func__);
    	return 0;
	}

	if (len >= 80) {
		I("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buff, len)) {
		return -EFAULT;
	}

	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memset(w_data, 0x0, sizeof(w_data));
	memset(x_pos, 0x0, sizeof(x_pos));
	memset(reg_cmd, 0x0, sizeof(reg_cmd));

	I("himax %s \n", buf);

	if ((buf[0] == 'r' || buf[0] == 'w') && buf[1] == ':' && buf[2] == 'x') {
		length = strlen(buf);

		/* I("%s: length = %d.\n", __func__,length); */
		for (loop_i = 0; loop_i < length; loop_i++) { /* find postion of 'x' */
			if (buf[loop_i] == 'x') {
				x_pos[count] = loop_i;
				count++;
			}
		}

		data_str = strrchr(buf, 'x');
		I("%s: %s.\n", __func__, data_str);
		length = strlen(data_str + 1) - 1;

		if (buf[0] == 'r') {
			if (buf[3] == 'F' && buf[4] == 'E' && length == 4) {
				length = length - base;
				cfg_flag = 1;
				memcpy(buf_tmp, data_str + base + 1, length);
			} else {
				cfg_flag = 0;
				memcpy(buf_tmp, data_str + 1, length);
			}

			byte_length = length / 2;

			if (!kstrtoul(buf_tmp, 16, &result)) {
				for (loop_i = 0 ; loop_i < byte_length ; loop_i++) {
					reg_cmd[loop_i] = (uint8_t)(result >> loop_i * 8);
				}
			}

			if (strcmp(HX_85XX_H_SERIES_PWON, private_ts->chip_name) == 0 && cfg_flag == 0)
				cfg_flag = 2;
		} else if (buf[0] == 'w') {
			if (buf[3] == 'F' && buf[4] == 'E') {
				cfg_flag = 1;
				memcpy(buf_tmp, buf + base + 3, length);
			} else {
				cfg_flag = 0;
				memcpy(buf_tmp, buf + 3, length);
			}

			if (count < 3) {
				byte_length = length / 2;

				if (!kstrtoul(buf_tmp, 16, &result)) { /* command */
					for (loop_i = 0 ; loop_i < byte_length ; loop_i++) {
						reg_cmd[loop_i] = (uint8_t)(result >> loop_i * 8);
					}
				}

				if (!kstrtoul(data_str + 1, 16, &result)) { /* data */
					for (loop_i = 0 ; loop_i < byte_length ; loop_i++) {
						w_data[loop_i] = (uint8_t)(result >> loop_i * 8);
					}
				}

				g_core_fp.fp_register_write(reg_cmd, byte_length, w_data, cfg_flag);
			} else {
				for (loop_i = 0; loop_i < count; loop_i++) { /* parsing addr after 'x' */
					memset(buf_tmp, 0x0, sizeof(buf_tmp));
					if (cfg_flag != 0 && loop_i != 0)
						byte_length = 2;
					else
						byte_length = x_pos[1] - x_pos[0] - 2; /* original */

					memcpy(buf_tmp, buf + x_pos[loop_i] + 1, byte_length);

					/* I("%s: buf_tmp = %s\n", __func__,buf_tmp); */
					if (!kstrtoul(buf_tmp, 16, &result)) {
						if (loop_i == 0) {
							reg_cmd[loop_i] = (uint8_t)(result);
							/* I("%s: reg_cmd = %X\n", __func__,reg_cmd[0]); */
						} else {
							w_data[loop_i - 1] = (uint8_t)(result);
							/* I("%s: w_data[%d] = %2X\n", __func__,loop_i - 1,w_data[loop_i - 1]); */
						}
					}
				}

				byte_length = count - 1;
				if (strcmp(HX_85XX_H_SERIES_PWON, private_ts->chip_name) == 0 && cfg_flag == 0)
					cfg_flag = 2;
				g_core_fp.fp_register_write(reg_cmd, byte_length, &w_data[0], cfg_flag);
			}
		} else {
			return len;
		}
	}

	return len;
}

static struct file_operations himax_proc_oppo_reg_read_ops = {
	.owner = THIS_MODULE,
	.read = himax_oppo_reg_read_read,
	.write = himax_oppo_reg_read_write,
};
//HIMAX_PROC_OPPO_REG_INFO END

static int himax_oppo_ctp_openshort_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &himax_baseline_test_seq_ops);
}

static struct file_operations himax_proc_oppo_ctp_openshort_test_ops = {
	.owner = THIS_MODULE,
	.open = himax_oppo_ctp_openshort_open,
	.read = seq_read,
};

int OPPO_proc_node_init(void)
{
	himax_proc_touchscreen_dir = proc_mkdir(HIMAX_PROC_TOUCHSCREEN_FOLDER, NULL);

	if (himax_proc_touchscreen_dir == NULL) {
		E(" %s: himax_proc_touchscreen_dir file create failed!\n", __func__);
		return -ENOMEM;
	}

	himax_proc_oppo_ctp_openshort_test = proc_create(HIMAX_PROC_OPPO_CTP_OPENSHORT_TEST, (S_IWUSR | S_IRUGO),
		himax_proc_touchscreen_dir, &himax_proc_oppo_ctp_openshort_test_ops);
	if (himax_proc_oppo_ctp_openshort_test == NULL) {
		E(" %s: proc oppo_ctp_openshort_test file create failed!\n", __func__);
		return -ENOMEM;
	}

	himax_proc_touchpanel_dir = proc_mkdir(HIMAX_PROC_TOUCHPANEL_FOLDER, NULL);

		if (himax_proc_touchpanel_dir == NULL) {
			E(" %s: himax_proc_touchpanel_dir file create failed!\n", __func__);
			return -ENOMEM;
		}

	himax_proc_oppo_reg_info_file = proc_create(HIMAX_PROC_OPPO_REG_INFO_FILE, (S_IWUSR | S_IRUGO),
								  himax_proc_touchpanel_dir, &himax_proc_oppo_reg_read_ops);
	if (himax_proc_oppo_reg_info_file == NULL) {
		E(" %s: proc oppo_reg_info file create failed!\n", __func__);
		goto fail_oppo_proc_0;
	}

	himax_proc_tp_fw_update_file = proc_create(HIMAX_PROC_TP_FW_UPDATE_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
								  himax_proc_touchpanel_dir, &himax_proc_tp_fw_update_ops);
	if (himax_proc_tp_fw_update_file == NULL) {
		E(" %s: proc tp_fw_update file create failed!\n", __func__);
		goto fail_oppo_proc_1;
	}

#ifdef HX_SMART_WAKEUP
#ifdef HX_GESTURE_TRACK
	himax_proc_coordinate_file = proc_create(HIMAX_PROC_COORDINATE_FILE, (S_IWUSR | S_IRUGO),
									  himax_proc_touchpanel_dir, &himax_proc_coor_ops);
	if (himax_proc_coordinate_file == NULL) {
		E(" %s: himax_proc_coordinate_file create failed!\n", __func__);
		goto fail_oppo_proc_2;
	}
#endif
	himax_proc_double_tap_en_file = proc_create(HIMAX_PROC_DOUBLE_TAP_EN_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
								  himax_proc_touchpanel_dir, &himax_proc_double_tap_en_ops);
	if (himax_proc_double_tap_en_file == NULL) {
		E(" %s: proc double_tap_en file create failed!\n", __func__);
		goto fail_oppo_proc_3;
	}
#endif
	himax_proc_limit_enable_file = proc_create(HIMAX_PROC_LIMIT_ENABLE_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
										  himax_proc_touchpanel_dir, &hx_proc_limit_control_ops);

	if (himax_proc_limit_enable_file == NULL) {
		E(" %s: proc limit enable file create failed!\n", __func__);
		goto fail_oppo_proc_4;
	}
	himax_proc_limit_area_file = proc_create(HIMAX_PROC_LIMIT_AREA_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
										  himax_proc_touchpanel_dir, &proc_limit_area_ops);

	if (himax_proc_limit_area_file == NULL) {
		E(" %s: proc limit area file create failed!\n", __func__);
		goto fail_oppo_proc_5;
	}
	himax_proc_game_switch_en_file = proc_create(HIMAX_PROC_GAME_SWITCH_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
										  himax_proc_touchpanel_dir, &hx_proc_game_switch_ops);

	if (himax_proc_game_switch_en_file == NULL) {
		E(" %s: proc game switch en file create failed!\n", __func__);
		goto fail_oppo_proc_6;
	}

	himax_proc_oppo_tp_direction_file = proc_create(HIMAX_PROC_OPPO_TP_DIRECTION_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
								  himax_proc_touchpanel_dir, &himax_proc_oppo_tp_direction_ops);
	if (himax_proc_oppo_tp_direction_file == NULL) {
		E(" %s: proc oppo_tp_direction file create failed!\n", __func__);
		goto fail_oppo_proc_7;
	}

	himax_proc_incell_panel_file = proc_create(HIMAX_PROC_INCELL_PANEL_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
								  himax_proc_touchpanel_dir, &himax_proc_incell_panel_ops);
	if (himax_proc_incell_panel_file == NULL) {
		E(" %s: proc incell_panel file create failed!\n", __func__);
		goto fail_oppo_proc_8;
	}

	himax_proc_oppo_tx_hop_file = proc_create(HIMAX_PROC_OPPO_TX_HOP_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
								  himax_proc_touchpanel_dir, &himax_proc_oppo_tx_hop_ops);
	if (himax_proc_oppo_tx_hop_file == NULL) {
		E(" %s: proc tx freq change test create failed!\n", __func__);
		goto fail_oppo_proc_9;
	}

	himax_proc_baseline_test_file = proc_create(HIMAX_PROC_BASELINE_TEST_FILE, (S_IRUGO), himax_proc_touchpanel_dir, &himax_proc_baseline_test_ops);
	if (himax_proc_baseline_test_file == NULL) {
		E(" %s: proc baseline_test file create failed!\n", __func__);
		goto fail_oppo_proc_10;
	}
	#if defined(HX_SMART_WAKEUP)
	himax_proc_backscreen_baseline_file = proc_create(HIMAX_PROC_BACKSCREEN_BASELINE_FILE, (S_IRUGO | S_IWUGO), himax_proc_touchpanel_dir, &himax_black_screen_test_fops);
	if (himax_proc_backscreen_baseline_file == NULL) {
		E(" %s create proc/touchpanel/blackscreen_test Failed!\n", __func__);
		goto fail_oppo_proc_11;
	}
	#endif
	himax_proc_irq_depth_file = proc_create(HIMAX_PROC_IRQ_DEPTH_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
								  himax_proc_touchpanel_dir, &himax_proc_irq_depth_ops);
	if (himax_proc_irq_depth_file == NULL) {
		E(" %s: proc incell_panel file create failed!\n", __func__);
		goto fail_oppo_proc_12;
	}

	himax_proc_oppo_debug_level_file = proc_create(HIMAX_PROC_OPPO_DEBUG_LEVEL_FILE, (S_IWUSR | S_IRUGO | S_IWUGO),
								  himax_proc_touchpanel_dir, &himax_proc_oppo_debug_level_ops);
	if (himax_proc_oppo_debug_level_file == NULL) {
		E(" %s: proc incell_panel file create failed!\n", __func__);
		goto fail_oppo_proc_13;
	}

	himax_proc_debug_infor_dir = proc_mkdir(HIMAX_PROC_DEBUG_INFO_FOLDER, himax_proc_touchpanel_dir);

	if (himax_proc_debug_infor_dir!= NULL) {
		himax_proc_delta_o_file = proc_create(HIMAX_PROC_DELTA_O_FILE, (S_IWUSR | S_IRUGO),
									  himax_proc_debug_infor_dir, &himax_proc_delta_ops);
		if (himax_proc_delta_o_file == NULL) {
			E(" %s: proc dbg_info file create failed!\n", __func__);
			goto fail_oppo_proc_dbg_1;
		}

		himax_proc_baseline_O_file = proc_create(HIMAX_PROC_BASELINE_O_FILE, (S_IWUSR | S_IRUGO),
									  himax_proc_debug_infor_dir, &himax_proc_baseline_ops);
		if (himax_proc_baseline_O_file == NULL) {
			E(" %s: proc baseline file create failed!\n", __func__);
			goto fail_oppo_proc_dbg_2;
		}

		himax_proc_main_reg_file = proc_create(HIMAX_PROC_MAIN_REG_FILE, (S_IWUSR | S_IRUGO),
									  himax_proc_debug_infor_dir, &himax_proc_main_reg_ops);
		if (himax_proc_main_reg_file == NULL) {
			E(" %s: proc main_register file create failed!\n", __func__);
			goto fail_oppo_proc_dbg_3;
		}

		himax_proc_data_limit_file = proc_create(HIMAX_PROC_DATA_LIMIT_FILE, (S_IWUSR | S_IRUGO),
									  himax_proc_debug_infor_dir, &himax_proc_data_limit_ops);
		if (himax_proc_data_limit_file == NULL) {
			E(" %s: proc data_limit file create failed!\n", __func__);
			goto fail_oppo_proc_dbg_4;
		}
	} else {
		E(" %s: himax_proc_debug_infor_dir create failed!\n", __func__);
		goto fail_oppo_proc_dbg_0;
	}
	return 0;
fail_oppo_proc_dbg_4:
	remove_proc_entry(HIMAX_PROC_MAIN_REG_FILE, himax_proc_debug_infor_dir);
fail_oppo_proc_dbg_3:
	remove_proc_entry(HIMAX_PROC_BASELINE_O_FILE, himax_proc_debug_infor_dir);
fail_oppo_proc_dbg_2:
	remove_proc_entry(HIMAX_PROC_DELTA_O_FILE, himax_proc_debug_infor_dir);
fail_oppo_proc_dbg_1:
	remove_proc_entry(HIMAX_PROC_DEBUG_INFO_FOLDER, himax_proc_touchpanel_dir);
fail_oppo_proc_dbg_0:
	remove_proc_entry(HIMAX_PROC_OPPO_DEBUG_LEVEL_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_13:
	remove_proc_entry(HIMAX_PROC_IRQ_DEPTH_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_12:
#if defined(HX_SMART_WAKEUP)
	remove_proc_entry(HIMAX_PROC_BACKSCREEN_BASELINE_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_11:
#endif
	remove_proc_entry(HIMAX_PROC_BASELINE_TEST_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_10:
	remove_proc_entry(HIMAX_PROC_OPPO_TX_HOP_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_9:
	remove_proc_entry(HIMAX_PROC_INCELL_PANEL_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_8:
	remove_proc_entry(HIMAX_PROC_OPPO_TP_DIRECTION_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_7:
	remove_proc_entry(HIMAX_PROC_GAME_SWITCH_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_6:
	remove_proc_entry(HIMAX_PROC_LIMIT_AREA_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_5:
	remove_proc_entry(HIMAX_PROC_LIMIT_ENABLE_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_4:
#ifdef HX_SMART_WAKEUP
	remove_proc_entry(HIMAX_PROC_DOUBLE_TAP_EN_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_3:
#ifdef HX_GESTURE_TRACK
	remove_proc_entry(HIMAX_PROC_COORDINATE_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_2:
#endif
#endif
	remove_proc_entry(HIMAX_PROC_TP_FW_UPDATE_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_1:
	remove_proc_entry(HIMAX_PROC_OPPO_REG_INFO_FILE, himax_proc_touchpanel_dir);
fail_oppo_proc_0:
	if( strcmp( HIMAX_PROC_TOUCHPANEL_FOLDER, HIMAX_PROC_TOUCH_FOLDER)!= 0  ){
		remove_proc_entry(HIMAX_PROC_TOUCHPANEL_FOLDER, NULL);
	}
	return -ENOMEM;

}

int OPPO_proc_node_deinit(void)
{
	remove_proc_entry(HIMAX_PROC_MAIN_REG_FILE, himax_proc_debug_infor_dir);
	remove_proc_entry(HIMAX_PROC_BASELINE_O_FILE, himax_proc_debug_infor_dir);
	remove_proc_entry(HIMAX_PROC_DELTA_O_FILE, himax_proc_debug_infor_dir);
	remove_proc_entry(HIMAX_PROC_DATA_LIMIT_FILE, himax_proc_debug_infor_dir);
	remove_proc_entry(HIMAX_PROC_DEBUG_INFO_FOLDER, himax_proc_touchpanel_dir);
	#if defined(HX_SMART_WAKEUP)
	remove_proc_entry(HIMAX_PROC_BACKSCREEN_BASELINE_FILE, himax_proc_touchpanel_dir);
	#endif
	remove_proc_entry(HIMAX_PROC_OPPO_DEBUG_LEVEL_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_IRQ_DEPTH_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_BASELINE_TEST_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_OPPO_TX_HOP_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_INCELL_PANEL_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_OPPO_TP_DIRECTION_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_GAME_SWITCH_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_LIMIT_AREA_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_LIMIT_ENABLE_FILE, himax_proc_touchpanel_dir);
#ifdef HX_SMART_WAKEUP
	remove_proc_entry(HIMAX_PROC_DOUBLE_TAP_EN_FILE, himax_proc_touchpanel_dir);
#ifdef HX_GESTURE_TRACK
	remove_proc_entry(HIMAX_PROC_COORDINATE_FILE, himax_proc_touchpanel_dir);
#endif
#endif	
	remove_proc_entry(HIMAX_PROC_TP_FW_UPDATE_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_OPPO_REG_INFO_FILE, himax_proc_touchpanel_dir);
	remove_proc_entry(HIMAX_PROC_TOUCHPANEL_FOLDER, NULL);
	remove_proc_entry(HIMAX_PROC_OPPO_CTP_OPENSHORT_TEST, himax_proc_touchscreen_dir);
	remove_proc_entry(HIMAX_PROC_TOUCHSCREEN_FOLDER, NULL);
	return -ENOMEM;
}

//*******************************************************************************
#ifdef HX_UPDATE_FW_NOTIFIER
static void himax_notifie_resume_workqueue(struct work_struct *work)
{
	I("%s: Entering! \n", __func__);
	himax_chip_common_resume(private_ts);
	return;
}

static void himax_lcd_resume_func(void)
{
	schedule_work(&private_ts->resume_work_queue);
	return;
}

static void himax_esd_resume_func(void)
{

	I("%s: Entering! \n", __func__);
#ifdef HX_EXCP_RECOVERY
	if (!mutex_trylock(&private_ts->fw_update_lock))
	{
		I("%s: already in esd resume! \n", __func__);
		return;
	}
	himax_excp_hw_reset();
	mutex_unlock(&private_ts->fw_update_lock);
#endif
	I("%s: end! \n", __func__);
}

static int himax_notifie_update_fw(struct notifier_block *nb,unsigned long value, void *data)
{
	unsigned long update_fw_state = value;
	I("update_fw_state == %ld \n",update_fw_state);
	if(update_fw_state == 1) {
		himax_lcd_resume_func();
	}else if(update_fw_state == 2){
		himax_esd_resume_func();
	}else
		E("invalid update_fw_state == %ld \n",update_fw_state);
	return 0;
}

static void himax_update_fw_notifier_init(void)
{
	int res = 0;

	I("%s: Entering! \n", __func__);

	if(private_ts == NULL)
	{
		I("%s private_ts == NULL\n",__func__);
		return;
	}

	private_ts->notifier_update_fw.notifier_call = himax_notifie_update_fw;
	res = update_tpfw_register_client(&private_ts->notifier_update_fw);

}
#endif

#if defined(HX_USB_DETECT_GLOBAL)|| defined(HX_TP_USB_NOTIFIER)
static int himax_notifie_tp_usb(struct notifier_block *nb,unsigned long value, void *data)
{
	I("%s: Entering! \n", __func__);
	USB_detect_flag = value?true:false;
	I("%s:himax_tp_usb_state == %ld \n",__func__,value);

	return 0;
}
static void himax_tp_usb_notifier_init(void)
{
	int res = 0;

	I("%s: Entering! \n", __func__);

	if(private_ts == NULL)
	{
		I("%s private_ts == NULL\n",__func__);
		return;
	}
	private_ts->notifier_tp_usb.notifier_call = himax_notifie_tp_usb;
	res = tp_usb_register_client(&private_ts->notifier_tp_usb);
}
#endif

#ifdef HX_TP_HEADSET_NOTIFIER
bool himax_headset_mode_set(bool headset_sate)
{
	uint8_t tmp_data[DATA_LEN_4];
	uint8_t back_data[DATA_LEN_4];
	uint8_t retry_cnt = 0;

	do {
		if (headset_sate ) {
			himax_parse_assign_cmd(fw_func_handshaking_pwd, tmp_data, 4);
			g_core_fp.fp_register_write(pfw_op->addr_headline,DATA_LEN_4, tmp_data, 0);
			himax_parse_assign_cmd(fw_func_handshaking_pwd, back_data, 4);
			I("%s: headline IN!\n", __func__);
		} else {
			himax_parse_assign_cmd(fw_data_safe_mode_release_pw_reset, tmp_data, 4);
			g_core_fp.fp_register_write(pfw_op->addr_headline,DATA_LEN_4, tmp_data, 0);
			himax_parse_assign_cmd(fw_data_safe_mode_release_pw_reset, back_data, 4);
			I("%s: headline OUT!\n", __func__);
		}

		g_core_fp.fp_register_read(pfw_op->addr_headline, DATA_LEN_4, tmp_data, 0);

		retry_cnt++;
	} while ((tmp_data[3] != back_data[3] || tmp_data[2] != back_data[2] || tmp_data[1] != back_data[1]  || tmp_data[0] != back_data[0]) && retry_cnt < HIMAX_REG_RETRY_TIMES);

	if(retry_cnt < HIMAX_REG_RETRY_TIMES){
		return true;
	} else {
		return false;
	}
}

static void himax_tp_headset_workqueue(struct work_struct *work)
{
	int ret = 0;
	if(private_ts->suspended == false) {
		ret = himax_headset_mode_set( himax_tp_headset_state );
		if (!ret) {
			I("%s:himax headset mode set fail\n",__func__);
		}

	} else {

		I("%s:is suspended,do nothing",__func__);
	}

	return;
}

void himax_tp_headset_func(void)
{

	schedule_work(&private_ts->headset_work_queue);

}

static int himax_notifie_tp_headset(struct notifier_block *nb,unsigned long value, void *data)
{
	himax_tp_headset_state = value;
	I("%s:himax_tp_headset_state == %ld \n",__func__,himax_tp_headset_state);

	himax_tp_headset_func();

	return 0;
}

static void himax_tp_headset_notifier_init(void)
{
	int res = 0;

	I("%s \n",__func__);

	private_ts->notifier_headset.notifier_call = himax_notifie_tp_headset;
	res = headset_register_client(&private_ts->notifier_headset);
}
#endif
#endif
/*  File node for oppo  -- end*/


void himax_parse_assign_cmd(uint32_t addr, uint8_t *cmd, int len)
{
	/*I("%s: Entering!\n", __func__);*/

	switch (len) {
	case 1:
		cmd[0] = addr;
		/*I("%s: cmd[0] = 0x%02X\n", __func__, cmd[0]);*/
		break;

	case 2:
		cmd[0] = addr % 0x100;
		cmd[1] = (addr >> 8) % 0x100;
		/*I("%s: cmd[0] = 0x%02X,cmd[1] = 0x%02X\n",*/
		/*	__func__, cmd[0], cmd[1]);*/
		break;

	case 4:
		cmd[0] = addr % 0x100;
		cmd[1] = (addr >> 8) % 0x100;
		cmd[2] = (addr >> 16) % 0x100;
		cmd[3] = addr / 0x1000000;
		/*  I("%s: cmd[0] = 0x%02X,cmd[1] = 0x%02X,*/
		/*cmd[2] = 0x%02X,cmd[3] = 0x%02X\n", */
		/* __func__, cmd[0], cmd[1], cmd[2], cmd[3]);*/
		break;

	default:
		E("%s: input length fault,len = %d!\n", __func__, len);
	}
}
EXPORT_SYMBOL(himax_parse_assign_cmd);

int himax_input_register(struct himax_ts_data *ts)
{
	int ret = 0;
#if defined(HX_SMART_WAKEUP)
	int i = 0;
#endif
	ret = himax_dev_set(ts);

	if (ret < 0) {
		I("%s, input device register fail!\n", __func__);
		ret = INPUT_REGISTER_FAIL;
		goto input_device_fail;
	}

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	set_bit(KEY_F4, ts->input_dev->keybit);

#if defined(HX_SMART_WAKEUP)
	for (i = 0; i < GEST_SUP_NUM; i++)
		set_bit(gest_key_def[i], ts->input_dev->keybit);
#elif defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT) || defined(HX_PALM_REPORT)
	set_bit(KEY_POWER, ts->input_dev->keybit);
#endif
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(KEY_APPSELECT, ts->input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
#if defined(HX_PROTOCOL_A)
	/*ts->input_dev->mtsize = ts->nFinger_support;*/
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 3, 0, 0);
#else
	set_bit(MT_TOOL_FINGER, ts->input_dev->keybit);
#if defined(HX_PROTOCOL_B_3PA)
	input_mt_init_slots(ts->input_dev, ts->nFinger_support,
			INPUT_MT_DIRECT);
#else
	input_mt_init_slots(ts->input_dev, ts->nFinger_support);
#endif
#endif
	I("input_set_abs_params: mix_x %d, max_x %d, min_y %d, max_y %d\n",
			ts->pdata->abs_x_min,
			ts->pdata->abs_x_max,
			ts->pdata->abs_y_min,
			ts->pdata->abs_y_max);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
			ts->pdata->abs_x_min, ts->pdata->abs_x_max,
			ts->pdata->abs_x_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
			ts->pdata->abs_y_min, ts->pdata->abs_y_max,
			ts->pdata->abs_y_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,
			ts->pdata->abs_pressure_min,
			ts->pdata->abs_pressure_max,
			ts->pdata->abs_pressure_fuzz, 0);
#if !defined(HX_PROTOCOL_A)
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE,
			ts->pdata->abs_pressure_min,
			ts->pdata->abs_pressure_max,
			ts->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR,
			ts->pdata->abs_width_min,
			ts->pdata->abs_width_max,
			ts->pdata->abs_pressure_fuzz, 0);
#endif
/*	input_set_abs_params(ts->input_dev, ABS_MT_AMPLITUDE, 0,*/
/*			((ts->pdata->abs_pressure_max << 16)*/
/*			| ts->pdata->abs_width_max),*/
/*			0, 0);*/
/*	input_set_abs_params(ts->input_dev, ABS_MT_POSITION,*/
/*			0, (BIT(31)*/
/*			| (ts->pdata->abs_x_max << 16)*/
/*			| ts->pdata->abs_y_max),*/
/*			0, 0);*/

	if (himax_input_register_device(ts->input_dev) == 0) {
		ret = NO_ERR;
	} else {
		E("%s: input register fail\n", __func__);
		ret = INPUT_REGISTER_FAIL;
		goto input_device_fail;
	}

	if (!ic_data->HX_PEN_FUNC)
		goto skip_pen_operation;

	set_bit(EV_SYN, ts->hx_pen_dev->evbit);
	set_bit(EV_ABS, ts->hx_pen_dev->evbit);
	set_bit(EV_KEY, ts->hx_pen_dev->evbit);
	set_bit(BTN_TOUCH, ts->hx_pen_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, ts->hx_pen_dev->propbit);

	set_bit(BTN_TOOL_PEN, ts->hx_pen_dev->keybit);
	set_bit(BTN_TOOL_RUBBER, ts->hx_pen_dev->keybit);

	input_set_abs_params(ts->hx_pen_dev, ABS_PRESSURE, 0, 4095, 0, 0);
	input_set_abs_params(ts->hx_pen_dev, ABS_DISTANCE, 0, 1, 0, 0);
	input_set_abs_params(ts->hx_pen_dev, ABS_TILT_X, -60, 60, 0, 0);
	input_set_abs_params(ts->hx_pen_dev, ABS_TILT_Y, -60, 60, 0, 0);
	/*input_set_capability(ts->hx_pen_dev, EV_SW, SW_PEN_INSERT);*/
	input_set_capability(ts->hx_pen_dev, EV_KEY, BTN_TOUCH);
	input_set_capability(ts->hx_pen_dev, EV_KEY, BTN_STYLUS);
	input_set_capability(ts->hx_pen_dev, EV_KEY, BTN_STYLUS2);

	input_set_abs_params(ts->hx_pen_dev, ABS_X, ts->pdata->abs_x_min,
			ts->pdata->abs_x_max, ts->pdata->abs_x_fuzz, 0);
	input_set_abs_params(ts->hx_pen_dev, ABS_Y, ts->pdata->abs_y_min,
			ts->pdata->abs_y_max, ts->pdata->abs_y_fuzz, 0);

	if (himax_input_register_device(ts->hx_pen_dev) == 0) {
		ret = NO_ERR;
	} else {
		E("%s: input register pen fail\n", __func__);
		ret = INPUT_REGISTER_FAIL;
		goto input_device_fail;
	}

skip_pen_operation:

	I("%s, input device registered.\n", __func__);

input_device_fail:
	return ret;
}
EXPORT_SYMBOL(himax_input_register);

static void calcDataSize(void)
{
	struct himax_ts_data *ts_data = private_ts;

	ts_data->x_channel = ic_data->HX_RX_NUM;
	ts_data->y_channel = ic_data->HX_TX_NUM;
	ts_data->nFinger_support = ic_data->HX_MAX_PT;

	ts_data->coord_data_size = 4 * ts_data->nFinger_support;
	ts_data->area_data_size = ((ts_data->nFinger_support / 4)
				+ (ts_data->nFinger_support % 4 ? 1 : 0)) * 4;
	ts_data->coordInfoSize = ts_data->coord_data_size
				+ ts_data->area_data_size + 4;
	ts_data->raw_data_frame_size = 128 - ts_data->coord_data_size
				- ts_data->area_data_size - 4 - 4 - 1;

	if (ts_data->raw_data_frame_size == 0) {
		E("%s: could NOT calculate!\n", __func__);
		return;
	}

	ts_data->raw_data_nframes = ((uint32_t)ts_data->x_channel
					* ts_data->y_channel
					+ ts_data->x_channel
					+ ts_data->y_channel)
					/ ts_data->raw_data_frame_size
					+ (((uint32_t)ts_data->x_channel
					* ts_data->y_channel
					+ ts_data->x_channel
					+ ts_data->y_channel)
					% ts_data->raw_data_frame_size) ? 1 : 0;

	I("%s: coord_dsz:%d,area_dsz:%d,raw_data_fsz:%d,raw_data_nframes:%d",
		__func__,
		ts_data->coord_data_size,
		ts_data->area_data_size,
		ts_data->raw_data_frame_size,
		ts_data->raw_data_nframes);
}

static void calculate_point_number(void)
{
	HX_TOUCH_INFO_POINT_CNT = ic_data->HX_MAX_PT * 4;

	if ((ic_data->HX_MAX_PT % 4) == 0)
		HX_TOUCH_INFO_POINT_CNT += (ic_data->HX_MAX_PT / 4) * 4;
	else
		HX_TOUCH_INFO_POINT_CNT += ((ic_data->HX_MAX_PT / 4) + 1) * 4;
}

#if defined(HX_BOOT_UPGRADE) || defined(HX_ZERO_FLASH)
static int himax_auto_update_check(void)
{
	int32_t ret;

	I("%s: Entering!\n", __func__);
	if (g_core_fp.fp_fw_ver_bin() == 0) {
		if (((ic_data->vendor_fw_ver < g_i_FW_VER)
		|| (ic_data->vendor_config_ver < g_i_CFG_VER))) {
			I("%s: Need update\n", __func__);
			ret = 0;
		} else {
			I("%s: Need not update!\n", __func__);
			ret = 1;
		}
	} else {
		E("%s: FW bin fail!\n", __func__);
		ret = 1;
	}

	return ret;
}

static int i_get_FW(void)
{
	int ret = -1;
	int result = NO_ERR;
#ifdef OPPO_PROC_NODE
	struct firmware *tmp_fw_entry = NULL;
	if (tmp_fw_entry == NULL) {
			tmp_fw_entry = kzalloc(sizeof(struct firmware), GFP_KERNEL);
		}
	if(tmp_fw_entry == NULL) {
		I("%s kzalloc is failed!\n", __func__);
		ret = MEM_ALLOC_FAIL;
		return ret;
	}
	if(!private_ts->recovery_mode){
			// ret = request_firmware_select(&hxfw, hx_fw->firmware_sign_name, private_ts->dev);
			I("%s: file name = %s\n", __func__, hx_fw->firmware_name);
			ret = request_firmware(&hxfw, hx_fw->firmware_name, private_ts->dev);
		 if (ret < 0) {
			E("%s,%d: request fw fail,and use i file , error code = %d\n", __func__, __LINE__, ret);
			goto to_i_get_FW;
		 }
		 private_ts->using_headfile = false;

		 if (hxfw) {
			if (private_ts->g_fw_buf !=NULL) {
				private_ts->g_fw_len = hxfw->size;
				memcpy(private_ts->g_fw_buf, hxfw->data, hxfw->size);
				private_ts->g_fw_sta = true;
			}
		 }
	} else {
to_i_get_FW:
			if(private_ts->firmware_headfile.data != NULL){
				tmp_fw_entry->data = private_ts->firmware_headfile.data;
				tmp_fw_entry->size = private_ts->firmware_headfile.size;
				private_ts->using_headfile = true;
				hxfw = tmp_fw_entry;
				I("%s: recovery mode  ,so use i file!\n", __func__);
			} else {
				I("%s: firmware_data is NULL!!\n", __func__);
				result= OPEN_FILE_FAIL;
				goto i_get_FW_FAIL;
			}
	}
#else
	I("%s: file name = %s\n", __func__, hx_fw->firmware_name);
	ret = request_firmware(&hxfw, hx_fw->firmware_name, private_ts->dev);
	if (ret < 0) {
#if defined(__EMBEDDED_FW__)
		hxfw = &g_embedded_fw;
		I("%s: Not find FW in userspace, use embedded FW(size:%zu)",
			__func__, g_embedded_fw.size);
#else
		E("%s,%d: error code = %d\n", __func__, __LINE__, ret);
		result= OPEN_FILE_FAIL;
		goto i_get_FW_FAIL;
#endif
		}
#endif
i_get_FW_FAIL:
	return result;
}

static int i_update_FW(void)
{
#if !defined(HX_ZERO_FLASH)
	int upgrade_times = 0;
	int8_t ret = 0;
#endif
	int8_t result = 0;

	himax_int_enable(0);

#if defined(HX_ZERO_FLASH)

	g_core_fp.fp_firmware_update_0f(hxfw);

	g_core_fp.fp_reload_disable(0);

	g_core_fp.fp_read_FW_ver();

	g_core_fp.fp_touch_information();

	result = 1;/*upgrade success*/

#else
update_retry:

	if (hxfw->size == FW_SIZE_32k)
		ret = g_core_fp.fp_fts_ctpm_fw_upgrade_with_sys_fs_32k(
			(unsigned char *)hxfw->data, hxfw->size, false);
	else if (hxfw->size == FW_SIZE_60k)
		ret = g_core_fp.fp_fts_ctpm_fw_upgrade_with_sys_fs_60k(
			(unsigned char *)hxfw->data, hxfw->size, false);
	else if (hxfw->size == FW_SIZE_64k)
		ret = g_core_fp.fp_fts_ctpm_fw_upgrade_with_sys_fs_64k(
			(unsigned char *)hxfw->data, hxfw->size, false);
	else if (hxfw->size == FW_SIZE_124k)
		ret = g_core_fp.fp_fts_ctpm_fw_upgrade_with_sys_fs_124k(
			(unsigned char *)hxfw->data, hxfw->size, false);
	else if (hxfw->size == FW_SIZE_128k)
		ret = g_core_fp.fp_fts_ctpm_fw_upgrade_with_sys_fs_128k(
			(unsigned char *)hxfw->data, hxfw->size, false);

	if (ret == 0) {
		upgrade_times++;
		E("%s: TP upgrade error, upgrade_times = %d\n",
				__func__, upgrade_times);

		if (upgrade_times < 3)
			goto update_retry;
		else
			result = -1;


	} else {
		g_core_fp.fp_read_FW_ver();
		g_core_fp.fp_touch_information();
		result = 1;/*upgrade success*/
		I("%s: TP upgrade OK\n", __func__);
	}
#endif

#if defined(HX_RST_PIN_FUNC)
	g_core_fp.fp_ic_reset(true, false);
#else
	g_core_fp.fp_sense_on(0x00);
#endif

	himax_int_enable(1);

	return result;
}
#endif
/*
 *static int himax_loadSensorConfig(struct himax_i2c_platform_data *pdata)
 *{
 *	I("%s: initialization complete\n", __func__);
 *	return NO_ERR;
 *}
 */
#if defined(HX_EXCP_RECOVERY)
static void himax_excp_hw_reset(void)
{
#if defined(HX_ZERO_FLASH)
	int result = 0;
#endif
	if (g_ts_dbg != 0)
		I("%s: Entering\n", __func__);

	I("%s: START EXCEPTION Reset\n", __func__);

	if (private_ts->in_self_test == 1) {
		I("%s: In self test, not TP EXCEPTION Reset\n", __func__);
		return;
	}

	/*g_core_fp.fp_excp_ic_reset();*/
#if defined(HX_ZERO_FLASH)
	I("%s: It will update fw after exception event in zero flash mode!\n",
		__func__);
	result = g_core_fp.fp_0f_operation_dirly();
	if (result) {
		E("%s: Something is wrong! Skip Update with zero flash!\n",
			__func__);
		goto ESCAPE_0F_UPDATE;
	}
	g_core_fp.fp_reload_disable(0);
	g_core_fp.fp_sense_on(0x00);
	himax_report_all_leave_event(private_ts);
	himax_int_enable(1);
ESCAPE_0F_UPDATE:
#else
	g_core_fp.fp_excp_ic_reset();
#endif
	I("%s: END EXCEPTION Reset\n", __func__);
}
#endif

#if defined(HX_SMART_WAKEUP)
#if defined(HX_GESTURE_TRACK)
static void gest_pt_log_coordinate(int rx, int tx)
{
	/*driver report x y with range 0 - 255 , we scale it up to x/y pixel*/
	gest_pt_x[gest_pt_cnt] = rx * (ic_data->HX_X_RES) / 255;
	gest_pt_y[gest_pt_cnt] = tx * (ic_data->HX_Y_RES) / 255;
}
#endif
static int himax_wake_event_parse(struct himax_ts_data *ts, int ts_status)
{
	uint8_t *buf = wake_event_buffer;
#if defined(HX_GESTURE_TRACK)
#if !defined(OPPO_PROC_NODE)
	int tmp_max_x = 0x00;
	int tmp_min_x = 0xFFFF;
	int tmp_max_y = 0x00;
	int tmp_min_y = 0xFFFF;
#endif
	int gest_len;
#endif
	int i = 0, check_FC = 0, ret;
	int j = 0, gesture_pos = 0, gesture_flag = 0;

	if (g_ts_dbg != 0)
		I("%s: Entering!, ts_status=%d\n", __func__, ts_status);

	if (buf == NULL) {
		ret = -ENOMEM;
		goto END;
	}

	memcpy(buf, hx_touch_data->hx_event_buf, hx_touch_data->event_size);

	for (i = 0; i < GEST_PTLG_ID_LEN; i++) {
		for (j = 0; j < GEST_SUP_NUM; j++) {
			if (buf[i] == gest_event[j]) {
				gesture_flag = buf[i];
				gesture_pos = j;
				break;
			}
		}
		I("0x%2.2X ", buf[i]);
		if (buf[i] == gesture_flag) {
			check_FC++;
		} else {
			I("ID START at %x , value = 0x%2X skip the event\n",
					i, buf[i]);
			break;
		}
	}

	I("Himax gesture_flag= %x\n", gesture_flag);
	I("Himax check_FC is %d\n", check_FC);

	if (check_FC != GEST_PTLG_ID_LEN) {
		ret = 0;
		goto END;
	}

	if (buf[GEST_PTLG_ID_LEN] != GEST_PTLG_HDR_ID1
	|| buf[GEST_PTLG_ID_LEN + 1] != GEST_PTLG_HDR_ID2) {
		ret = 0;
		goto END;
	}

#if defined(HX_GESTURE_TRACK)

	if (buf[GEST_PTLG_ID_LEN] == GEST_PTLG_HDR_ID1
	&& buf[GEST_PTLG_ID_LEN + 1] == GEST_PTLG_HDR_ID2) {
		gest_len = buf[GEST_PTLG_ID_LEN + 2];
		I("gest_len = %d\n", gest_len);
		i = 0;
		gest_pt_cnt = 0;
		I("%s:gest doornidate start \n", __func__);

#if defined(OPPO_PROC_NODE) /*oppo gesture parsing*/

		for (i = 0; i < 14; i++) {/*default value initialize*/
			hx_gesture_coor[i] = 0;
			if (i == 13)
				hx_gesture_coor[i] = 1;
		}
		hx_gesture_coor[0] = gesture_flag; //buf[GEST_PTLG_ID_LEN + 4];
		if( hx_gesture_coor[0] > 0xfe )
			hx_gesture_coor[0]= 1;
		I("hx_gest_coor[0]=%d \n", hx_gesture_coor[0]);
		if (gest_len > 16) {
			gest_len = 16;
		}

		i = 0;
		while (i < (gest_len - 1) / 2) {
			if (i != 6) {
				gest_pt_log_coordinate(buf[GEST_PTLG_ID_LEN + 4 + i * 2], buf[GEST_PTLG_ID_LEN + 4 + i * 2 + 1]);
				hx_gesture_coor[i * 2 + 1] = gest_pt_x[gest_pt_cnt];
				hx_gesture_coor[i * 2 + 2] = gest_pt_y[gest_pt_cnt];
				I("hx_gest_coor[%d]=%d \n", i * 2 + 1, hx_gesture_coor[i * 2 + 1]);
				I("hx_gest_coor[%d]=%d \n", i * 2 + 2, hx_gesture_coor[i * 2 + 2]);
				gest_pt_cnt++;
			} else {
				hx_gesture_coor[i * 2 + 1] = buf[GEST_PTLG_ID_LEN + 4 + i * 2];
				I("hx_gest_coor[%d]=%d \n", i * 2 + 1, hx_gesture_coor[i * 2 + 1]);
			}
			i++;
		}
#else
		while (i < (gest_len + 1) / 2) {
			gest_pt_log_coordinate(
					buf[GEST_PTLG_ID_LEN + 4 + i * 2],
					buf[GEST_PTLG_ID_LEN + 4 + i * 2 + 1]);
			i++;
			I("gest_pt_x[%d]=%d,gest_pt_y[%d]=%d\n",
				gest_pt_cnt,
				gest_pt_x[gest_pt_cnt],
				gest_pt_cnt,
				gest_pt_y[gest_pt_cnt]);
			gest_pt_cnt += 1;
		}

		if (gest_pt_cnt) {
			for (i = 0; i < gest_pt_cnt; i++) {
				if (tmp_max_x < gest_pt_x[i])
					tmp_max_x = gest_pt_x[i];
				if (tmp_min_x > gest_pt_x[i])
					tmp_min_x = gest_pt_x[i];
				if (tmp_max_y < gest_pt_y[i])
					tmp_max_y = gest_pt_y[i];
				if (tmp_min_y > gest_pt_y[i])
					tmp_min_y = gest_pt_y[i];
			}

			I("gest_point x_min=%d,x_max=%d,y_min=%d,y_max=%d\n",
				tmp_min_x, tmp_max_x, tmp_min_y, tmp_max_y);

			gest_start_x = gest_pt_x[0];
			hx_gesture_coor[0] = gest_start_x;
			gest_start_y = gest_pt_y[0];
			hx_gesture_coor[1] = gest_start_y;
			gest_end_x = gest_pt_x[gest_pt_cnt - 1];
			hx_gesture_coor[2] = gest_end_x;
			gest_end_y = gest_pt_y[gest_pt_cnt - 1];
			hx_gesture_coor[3] = gest_end_y;
			gest_width = tmp_max_x - tmp_min_x;
			hx_gesture_coor[4] = gest_width;
			gest_height = tmp_max_y - tmp_min_y;
			hx_gesture_coor[5] = gest_height;
			gest_mid_x = (tmp_max_x + tmp_min_x) / 2;
			hx_gesture_coor[6] = gest_mid_x;
			gest_mid_y = (tmp_max_y + tmp_min_y) / 2;
			hx_gesture_coor[7] = gest_mid_y;
			/*gest_up_x*/
			hx_gesture_coor[8] = gest_mid_x;
			/*gest_up_y*/
			hx_gesture_coor[9] = gest_mid_y - gest_height / 2;
			/*gest_down_x*/
			hx_gesture_coor[10] = gest_mid_x;
			/*gest_down_y*/
			hx_gesture_coor[11] = gest_mid_y + gest_height / 2;
			/*gest_left_x*/
			hx_gesture_coor[12] = gest_mid_x - gest_width / 2;
			/*gest_left_y*/
			hx_gesture_coor[13] = gest_mid_y;
			/*gest_right_x*/
			hx_gesture_coor[14] = gest_mid_x + gest_width / 2;
			/*gest_right_y*/
			hx_gesture_coor[15] = gest_mid_y;
		}
#endif
	}

#endif

	if (!ts->gesture_cust_en[gesture_pos]) {
		I("%s NOT report key [%d] = %d\n", __func__,
				gesture_pos, gest_key_def[gesture_pos]);
		g_target_report_data->SMWP_event_chk = 0;
		ret = 0;
	} else {
		g_target_report_data->SMWP_event_chk =
				gest_key_def[gesture_pos];
		ret = gesture_pos;
	}
END:
	return ret;
}

static void himax_wake_event_report(void)
{
	int KEY_EVENT = g_target_report_data->SMWP_event_chk;

	if (g_ts_dbg != 0)
		I("%s: Entering!\n", __func__);

#ifdef OPPO_PROC_NODE
		KEY_EVENT = KEY_F4;
#endif


	if (KEY_EVENT) {
		I("%s SMART WAKEUP KEY event %d press\n", __func__, KEY_EVENT);
		input_report_key(private_ts->input_dev, KEY_EVENT, 1);
		input_sync(private_ts->input_dev);
		I("%s SMART WAKEUP KEY event %d release\n",
				__func__, KEY_EVENT);
		input_report_key(private_ts->input_dev, KEY_EVENT, 0);
		input_sync(private_ts->input_dev);
#if !defined(OPPO_PROC_NODE)
#if defined(HX_GESTURE_TRACK)
		I("gest_start_x=%d,start_y=%d,end_x=%d,end_y=%d\n",
			gest_start_x,
			gest_start_y,
			gest_end_x,
			gest_end_y);
		I("gest_width=%d,height=%d,mid_x=%d,mid_y=%d\n",
			gest_width,
			gest_height,
			gest_mid_x,
			gest_mid_y);
		I("gest_up_x=%d,up_y=%d,down_x=%d,down_y=%d\n",
			hx_gesture_coor[8],
			hx_gesture_coor[9],
			hx_gesture_coor[10],
			hx_gesture_coor[11]);
		I("gest_left_x=%d,left_y=%d,right_x=%d,right_y=%d\n",
			hx_gesture_coor[12],
			hx_gesture_coor[13],
			hx_gesture_coor[14],
			hx_gesture_coor[15]);
#endif
#endif
		g_target_report_data->SMWP_event_chk = 0;
	}
}

#endif

int himax_report_data_init(void)
{
	if (hx_touch_data->hx_coord_buf != NULL) {
		kfree(hx_touch_data->hx_coord_buf);
		hx_touch_data->hx_coord_buf = NULL;
	}

	if (hx_touch_data->hx_rawdata_buf != NULL) {
		kfree(hx_touch_data->hx_rawdata_buf);
		hx_touch_data->hx_rawdata_buf = NULL;
	}

#if defined(HX_SMART_WAKEUP)
	hx_touch_data->event_size = g_core_fp.fp_get_touch_data_size();

	if (hx_touch_data->hx_event_buf != NULL) {
		kfree(hx_touch_data->hx_event_buf);
		hx_touch_data->hx_event_buf = NULL;
	}

	if (wake_event_buffer != NULL) {
		kfree(wake_event_buffer);
		wake_event_buffer = NULL;
	}

#endif
	hx_touch_data->touch_all_size = g_core_fp.fp_get_touch_data_size();
	hx_touch_data->raw_cnt_max = ic_data->HX_MAX_PT / 4;
	hx_touch_data->raw_cnt_rmd = ic_data->HX_MAX_PT % 4;
	/* more than 4 fingers */
	if (hx_touch_data->raw_cnt_rmd != 0x00) {
		hx_touch_data->rawdata_size =
			g_core_fp.fp_cal_data_len(
				hx_touch_data->raw_cnt_rmd,
				ic_data->HX_MAX_PT,
				hx_touch_data->raw_cnt_max);

		hx_touch_data->touch_info_size = (ic_data->HX_MAX_PT
				+ hx_touch_data->raw_cnt_max + 2) * 4;
	} else { /* less than 4 fingers */
		hx_touch_data->rawdata_size =
			g_core_fp.fp_cal_data_len(
				hx_touch_data->raw_cnt_rmd,
				ic_data->HX_MAX_PT,
				hx_touch_data->raw_cnt_max);

		hx_touch_data->touch_info_size = (ic_data->HX_MAX_PT
				+ hx_touch_data->raw_cnt_max + 1) * 4;
	}

	if (ic_data->HX_PEN_FUNC) {
		hx_touch_data->touch_info_size += PEN_INFO_SZ;
		hx_touch_data->rawdata_size -= PEN_INFO_SZ;
	}

	if ((ic_data->HX_TX_NUM
	* ic_data->HX_RX_NUM
	+ ic_data->HX_TX_NUM
	+ ic_data->HX_RX_NUM)
	% hx_touch_data->rawdata_size == 0)
		hx_touch_data->rawdata_frame_size =
			(ic_data->HX_TX_NUM
			* ic_data->HX_RX_NUM
			+ ic_data->HX_TX_NUM
			+ ic_data->HX_RX_NUM)
			/ hx_touch_data->rawdata_size;
	else
		hx_touch_data->rawdata_frame_size =
			(ic_data->HX_TX_NUM
			* ic_data->HX_RX_NUM
			+ ic_data->HX_TX_NUM
			+ ic_data->HX_RX_NUM)
			/ hx_touch_data->rawdata_size
			+ 1;

	I("%s:rawdata_fsz = %d,HX_MAX_PT:%d,hx_raw_cnt_max:%d\n",
		__func__,
		hx_touch_data->rawdata_frame_size,
		ic_data->HX_MAX_PT,
		hx_touch_data->raw_cnt_max);
	I("%s:hx_raw_cnt_rmd:%d,g_hx_rawdata_size:%d,touch_info_size:%d\n",
		__func__,
		hx_touch_data->raw_cnt_rmd,
		hx_touch_data->rawdata_size,
		hx_touch_data->touch_info_size);

	hx_touch_data->hx_coord_buf = kzalloc(sizeof(uint8_t)
			* (hx_touch_data->touch_info_size),
			GFP_KERNEL);

	if (hx_touch_data->hx_coord_buf == NULL)
		goto mem_alloc_fail_coord_buf;

#if defined(HX_SMART_WAKEUP)
	wake_event_buffer = kcalloc(hx_touch_data->event_size,
			sizeof(uint8_t), GFP_KERNEL);
	if (wake_event_buffer == NULL)
		goto mem_alloc_fail_smwp;

	hx_touch_data->hx_event_buf = kzalloc(sizeof(uint8_t)
			* (hx_touch_data->event_size), GFP_KERNEL);
	if (hx_touch_data->hx_event_buf == NULL)
		goto mem_alloc_fail_event_buf;
#endif

	hx_touch_data->hx_rawdata_buf = kzalloc(sizeof(uint8_t)
		* (hx_touch_data->touch_all_size
		- hx_touch_data->touch_info_size),
		GFP_KERNEL);
	if (hx_touch_data->hx_rawdata_buf == NULL)
		goto mem_alloc_fail_rawdata_buf;

	if (g_target_report_data == NULL) {
		g_target_report_data =
			kzalloc(sizeof(struct himax_target_report_data),
			GFP_KERNEL);
		if (g_target_report_data == NULL)
			goto mem_alloc_fail_report_data;
/*
 *#if defined(HX_SMART_WAKEUP)
 *		g_target_report_data->SMWP_event_chk = 0;
 *#endif
 *		I("%s: SMWP_event_chk = %d\n", __func__,
 *			g_target_report_data->SMWP_event_chk);
 */
		g_target_report_data->x = kzalloc(sizeof(int)
				* (ic_data->HX_MAX_PT), GFP_KERNEL);
		if (g_target_report_data->x == NULL)
			goto mem_alloc_fail_report_data_x;

		g_target_report_data->y = kzalloc(sizeof(int)
				* (ic_data->HX_MAX_PT), GFP_KERNEL);
		if (g_target_report_data->y == NULL)
			goto mem_alloc_fail_report_data_y;

		g_target_report_data->w = kzalloc(sizeof(int)
				* (ic_data->HX_MAX_PT), GFP_KERNEL);
		if (g_target_report_data->w == NULL)
			goto mem_alloc_fail_report_data_w;

		g_target_report_data->finger_id = kzalloc(sizeof(int)
				* (ic_data->HX_MAX_PT), GFP_KERNEL);
		if (g_target_report_data->finger_id == NULL)
			goto mem_alloc_fail_report_data_fid;

		if (!ic_data->HX_PEN_FUNC)
			goto skip_pen_operation;

		g_target_report_data->p_x = kzalloc(sizeof(int)*2, GFP_KERNEL);
		if (g_target_report_data->p_x == NULL)
			goto mem_alloc_fail_report_data_px;

		g_target_report_data->p_y = kzalloc(sizeof(int)*2, GFP_KERNEL);
		if (g_target_report_data->p_y == NULL)
			goto mem_alloc_fail_report_data_py;

		g_target_report_data->p_w = kzalloc(sizeof(int)*2, GFP_KERNEL);
		if (g_target_report_data->p_w == NULL)
			goto mem_alloc_fail_report_data_pw;

		g_target_report_data->pen_id = kzalloc(sizeof(int)*2,
				GFP_KERNEL);
		if (g_target_report_data->pen_id == NULL)
			goto mem_alloc_fail_report_data_pid;

		g_target_report_data->p_hover = kzalloc(sizeof(int)*2,
				GFP_KERNEL);
		if (g_target_report_data->p_hover == NULL)
			goto mem_alloc_fail_report_data_ph;

		g_target_report_data->p_tilt_x = kzalloc(sizeof(int)*2,
				GFP_KERNEL);
		if (g_target_report_data->p_tilt_x == NULL)
			goto mem_alloc_fail_report_data_ptx;

		g_target_report_data->p_btn = kzalloc(sizeof(int)*2,
				GFP_KERNEL);
		if (g_target_report_data->p_btn == NULL)
			goto mem_alloc_fail_report_data_pb;

		g_target_report_data->p_btn2 = kzalloc(sizeof(int)*2,
				GFP_KERNEL);
		if (g_target_report_data->p_btn2 == NULL)
			goto mem_alloc_fail_report_data_pb2;

		g_target_report_data->p_tilt_y = kzalloc(sizeof(int)*2,
				GFP_KERNEL);
		if (g_target_report_data->p_tilt_y == NULL)
			goto mem_alloc_fail_report_data_pty;

		g_target_report_data->p_on = kzalloc(sizeof(int)*2, GFP_KERNEL);
		if (g_target_report_data->p_on == NULL)
			goto mem_alloc_fail_report_data_pon;
	}

skip_pen_operation:

	return NO_ERR;

mem_alloc_fail_report_data_pon:
	kfree(g_target_report_data->p_tilt_y);
	g_target_report_data->p_tilt_y = NULL;
mem_alloc_fail_report_data_pty:
	kfree(g_target_report_data->p_btn2);
	g_target_report_data->p_btn2 = NULL;
mem_alloc_fail_report_data_pb2:
	kfree(g_target_report_data->p_btn);
	g_target_report_data->p_btn = NULL;
mem_alloc_fail_report_data_pb:
	kfree(g_target_report_data->p_tilt_x);
	g_target_report_data->p_tilt_x = NULL;
mem_alloc_fail_report_data_ptx:
	kfree(g_target_report_data->p_hover);
	g_target_report_data->p_hover = NULL;
mem_alloc_fail_report_data_ph:
	kfree(g_target_report_data->pen_id);
	g_target_report_data->pen_id = NULL;
mem_alloc_fail_report_data_pid:
	kfree(g_target_report_data->p_w);
	g_target_report_data->p_w = NULL;
mem_alloc_fail_report_data_pw:
	kfree(g_target_report_data->p_y);
	g_target_report_data->p_y = NULL;
mem_alloc_fail_report_data_py:
	kfree(g_target_report_data->p_x);
	g_target_report_data->p_x = NULL;
mem_alloc_fail_report_data_px:

	kfree(g_target_report_data->finger_id);
	g_target_report_data->finger_id = NULL;
mem_alloc_fail_report_data_fid:
	kfree(g_target_report_data->w);
	g_target_report_data->w = NULL;
mem_alloc_fail_report_data_w:
	kfree(g_target_report_data->y);
	g_target_report_data->y = NULL;
mem_alloc_fail_report_data_y:
	kfree(g_target_report_data->x);
	g_target_report_data->x = NULL;
mem_alloc_fail_report_data_x:
	kfree(g_target_report_data);
	g_target_report_data = NULL;
mem_alloc_fail_report_data:
	kfree(hx_touch_data->hx_rawdata_buf);
	hx_touch_data->hx_rawdata_buf = NULL;
mem_alloc_fail_rawdata_buf:
#if defined(HX_SMART_WAKEUP)
	kfree(hx_touch_data->hx_event_buf);
	hx_touch_data->hx_event_buf = NULL;
mem_alloc_fail_event_buf:
	kfree(wake_event_buffer);
	wake_event_buffer = NULL;
mem_alloc_fail_smwp:
#endif
	kfree(hx_touch_data->hx_coord_buf);
	hx_touch_data->hx_coord_buf = NULL;
mem_alloc_fail_coord_buf:

	E("%s: Failed to allocate memory\n", __func__);
	return MEM_ALLOC_FAIL;
}
EXPORT_SYMBOL(himax_report_data_init);

void himax_report_data_deinit(void)
{
	if (ic_data->HX_PEN_FUNC) {
		kfree(g_target_report_data->p_on);
		g_target_report_data->p_on = NULL;
		kfree(g_target_report_data->p_tilt_y);
		g_target_report_data->p_tilt_y = NULL;
		kfree(g_target_report_data->p_btn2);
		g_target_report_data->p_btn2 = NULL;
		kfree(g_target_report_data->p_btn);
		g_target_report_data->p_btn = NULL;
		kfree(g_target_report_data->p_tilt_x);
		g_target_report_data->p_tilt_x = NULL;
		kfree(g_target_report_data->p_hover);
		g_target_report_data->p_hover = NULL;
		kfree(g_target_report_data->pen_id);
		g_target_report_data->pen_id = NULL;
		kfree(g_target_report_data->p_w);
		g_target_report_data->p_w = NULL;
		kfree(g_target_report_data->p_y);
		g_target_report_data->p_y = NULL;
		kfree(g_target_report_data->p_x);
		g_target_report_data->p_x = NULL;
	}

	kfree(g_target_report_data->finger_id);
	g_target_report_data->finger_id = NULL;
	kfree(g_target_report_data->w);
	g_target_report_data->w = NULL;
	kfree(g_target_report_data->y);
	g_target_report_data->y = NULL;
	kfree(g_target_report_data->x);
	g_target_report_data->x = NULL;
	kfree(g_target_report_data);
	g_target_report_data = NULL;

#if defined(HX_SMART_WAKEUP)
	kfree(wake_event_buffer);
	wake_event_buffer = NULL;
	kfree(hx_touch_data->hx_event_buf);
	hx_touch_data->hx_event_buf = NULL;
#endif
	kfree(hx_touch_data->hx_rawdata_buf);
	hx_touch_data->hx_rawdata_buf = NULL;
	kfree(hx_touch_data->hx_coord_buf);
	hx_touch_data->hx_coord_buf = NULL;
}

/*start ts_work*/
#if defined(HX_USB_DETECT_GLOBAL) || defined(HX_TP_USB_NOTIFIER)
void himax_cable_detect_func(bool force_renew)
{
	struct himax_ts_data *ts;

	uint8_t connect_status = 0;
	connect_status = USB_detect_flag;/* upmu_is_chr_det(); */
	ts = private_ts;

	/* I("Touch: cable status=%d, cable_config=%p, usb_connected=%d\n",
			connect_status, ts->cable_config, ts->usb_connected); */
	if (ts->cable_config) {
		if ((connect_status != ts->usb_connected) || force_renew) {
			if (connect_status) {
				ts->cable_config[1] = 0x01;
				ts->usb_connected = 0x01;
			} else {
				ts->cable_config[1] = 0x00;
				ts->usb_connected = 0x00;
			}

			g_core_fp.fp_usb_detect_set(ts->cable_config);
			I("%s: Cable status change: 0x%2.2X\n", __func__,
					ts->usb_connected);
		}

		/*else
			I("%s: Cable status is the same as
				previous one, ignore.\n", __func__);*/
	}
}
#endif

static int himax_ts_work_status(struct himax_ts_data *ts)
{
	/* 1: normal, 2:SMWP */
	int result = HX_REPORT_COORD;

	hx_touch_data->diag_cmd = ts->diag_cmd;
	if (hx_touch_data->diag_cmd)
		result = HX_REPORT_COORD_RAWDATA;

#if defined(HX_SMART_WAKEUP)
	if (atomic_read(&ts->suspend_mode)
	&& (ts->SMWP_enable)
	&& (!hx_touch_data->diag_cmd))
		result = HX_REPORT_SMWP_EVENT;
#endif
	/* I("Now Status is %d\n", result); */
	return result;
}

static int himax_touch_get(struct himax_ts_data *ts, uint8_t *buf,
		int ts_path, int ts_status)
{
	if (g_ts_dbg != 0)
		I("%s: Entering, ts_status=%d!\n", __func__, ts_status);

	switch (ts_path) {
	/*normal*/
	case HX_REPORT_COORD:
		if ((HX_HW_RESET_ACTIVATE)
#if defined(HX_EXCP_RECOVERY)
			|| (HX_EXCP_RESET_ACTIVATE)
#endif
			) {
			if (!g_core_fp.fp_read_event_stack(buf, 128)) {
				E("%s: can't read data from chip!\n", __func__);
				ts_status = HX_TS_GET_DATA_FAIL;
			}
		} else {
			if (!g_core_fp.fp_read_event_stack(buf,
			hx_touch_data->touch_info_size)) {
				E("%s: can't read data from chip!\n", __func__);
				ts_status = HX_TS_GET_DATA_FAIL;
			}
		}
		break;
#if defined(HX_SMART_WAKEUP)

	/*SMWP*/
	case HX_REPORT_SMWP_EVENT:
		__pm_wakeup_event(&ts->ts_SMWP_wake_lock, TS_WAKE_LOCK_TIMEOUT);
		msleep(20);
		g_core_fp.fp_burst_enable(0);

		if (!g_core_fp.fp_read_event_stack(buf,
		hx_touch_data->event_size)) {
			E("%s: can't read data from chip!\n", __func__);
			ts_status = HX_TS_GET_DATA_FAIL;
		}
		break;
#endif
	case HX_REPORT_COORD_RAWDATA:
		if (!g_core_fp.fp_read_event_stack(buf, 128)) {
			E("%s: can't read data from chip!\n", __func__);
			ts_status = HX_TS_GET_DATA_FAIL;
		}
		break;
	default:
		break;
	}

	return ts_status;
}

/* start error_control*/
static int himax_checksum_cal(struct himax_ts_data *ts, uint8_t *buf,
		int ts_path, int ts_status)
{
	uint16_t check_sum_cal = 0;
	int32_t	i = 0;
	int length = 0;
	int zero_cnt = 0;
	int raw_data_sel = 0;
	int ret_val = ts_status;

	if (g_ts_dbg != 0)
		I("%s: Entering, ts_status=%d!\n", __func__, ts_status);

	/* Normal */
	switch (ts_path) {
	case HX_REPORT_COORD:
		length = hx_touch_data->touch_info_size;
		break;
#if defined(HX_SMART_WAKEUP)
/* SMWP */
	case HX_REPORT_SMWP_EVENT:
		length = (GEST_PTLG_ID_LEN + GEST_PTLG_HDR_LEN);
		break;
#endif
	case HX_REPORT_COORD_RAWDATA:
		length = hx_touch_data->touch_info_size;
		break;
	default:
		I("%s, Neither Normal Nor SMWP error!\n", __func__);
		ret_val = HX_PATH_FAIL;
		goto END_FUNCTION;
	}

	for (i = 0; i < length; i++) {
		check_sum_cal += buf[i];
		if (buf[i] == 0x00)
			zero_cnt++;
	}

	if (check_sum_cal % 0x100 != 0) {
		I("point data_checksum not match check_sum_cal: 0x%02X",
			check_sum_cal);
		ret_val = HX_CHKSUM_FAIL;
	} else if (zero_cnt == length) {
		if (ts->use_irq)
			I("[HIMAX TP MSG] All Zero event\n");

		ret_val = HX_CHKSUM_FAIL;
	} else {
		raw_data_sel = buf[HX_TOUCH_INFO_POINT_CNT]>>4 & 0x0F;
		/*I("%s:raw_out_sel=%x , hx_touch_data->diag_cmd=%x.\n",*/
		/*		__func__, raw_data_sel,*/
		/*		hx_touch_data->diag_cmd);*/
		/*raw data out not match skip it*/
		if ((raw_data_sel != 0x0F)
		&& (raw_data_sel != hx_touch_data->diag_cmd)) {
			/*I("%s:raw data out not match.\n", __func__);*/
			if (!hx_touch_data->diag_cmd) {
				/*Need to clear event stack here*/
				g_core_fp.fp_read_event_stack(buf,
					(128-hx_touch_data->touch_info_size));
				/*I("%s: size =%d, buf[0]=%x ,buf[1]=%x,*/
				/*	buf[2]=%x, buf[3]=%x.\n",*/
				/*	__func__,*/
				/*	(128-hx_touch_data->touch_info_size),*/
				/*	buf[0], buf[1], buf[2], buf[3]);*/
				/*I("%s:also clear event stack.\n", __func__);*/
			}
			ret_val = HX_READY_SERVE;
		}
	}

END_FUNCTION:
	if (g_ts_dbg != 0)
		I("%s: END, ret_val=%d!\n", __func__, ret_val);
	return ret_val;
}

#if defined(HX_EXCP_RECOVERY)
#if defined(HX_ZERO_FLASH)
void hx_update_dirly_0f(void)
{
	I("It will update fw after exception event in zero flash mode!\n");
	g_core_fp.fp_0f_operation_dirly();
}
#endif
static int himax_ts_event_check(struct himax_ts_data *ts,
		uint8_t *buf, int ts_path, int ts_status)
{
	uint32_t hx_EB_event = 0;
	uint32_t hx_EC_event = 0;
	uint32_t hx_ED_event = 0;
	uint32_t hx_excp_event = 0;
	uint32_t hx_zero_event = 0;
	int shaking_ret = 0;

	uint32_t loop_i = 0;
	uint32_t length = 0;
	int ret_val = ts_status;

	if (g_ts_dbg != 0)
		I("%s: Entering, ts_status=%d!\n", __func__, ts_status);

	/* Normal */
	switch (ts_path) {
	case HX_REPORT_COORD:
		length = hx_touch_data->touch_info_size;
		break;
#if defined(HX_SMART_WAKEUP)
/* SMWP */
	case HX_REPORT_SMWP_EVENT:
		length = (GEST_PTLG_ID_LEN + GEST_PTLG_HDR_LEN);
		break;
#endif
	case HX_REPORT_COORD_RAWDATA:
		length = hx_touch_data->touch_info_size;
		break;
	default:
		I("%s, Neither Normal Nor SMWP error!\n", __func__);
		ret_val = HX_PATH_FAIL;
		goto END_FUNCTION;
	}

	if (g_ts_dbg != 0)
		I("Now Path=%d, Now status=%d, length=%d\n",
				ts_path, ts_status, length);

	for (loop_i = 0; loop_i < length; loop_i++) {
		if (ts_path == HX_REPORT_COORD
		|| ts_path == HX_REPORT_COORD_RAWDATA) {
			/* case 1 EXCEEPTION recovery flow */
			if (buf[loop_i] == 0xEB) {
				hx_EB_event++;
			} else if (buf[loop_i] == 0xEC) {
				hx_EC_event++;
			} else if (buf[loop_i] == 0xED) {
				hx_ED_event++;

			/* case 2 EXCEPTION recovery flow-Disable */
			} else if (buf[loop_i] == 0x00) {
				hx_zero_event++;
			} else {
				hx_EB_event = 0;
				hx_EC_event = 0;
				hx_ED_event = 0;
				hx_zero_event = 0;
				g_zero_event_count = 0;
			}
		}
	}

	if (hx_EB_event == length) {
		hx_excp_event = length;
		hx_EB_event_flag++;
		I("[HIMAX TP MSG]: EXCEPTION event checked - ALL 0xEB.\n");
	} else if (hx_EC_event == length) {
		hx_excp_event = length;
		hx_EC_event_flag++;
		I("[HIMAX TP MSG]: EXCEPTION event checked - ALL 0xEC.\n");
	} else if (hx_ED_event == length) {
		hx_excp_event = length;
		hx_ED_event_flag++;
		I("[HIMAX TP MSG]: EXCEPTION event checked - ALL 0xED.\n");
	}
/*#if defined(HX_ZERO_FLASH)
 *	//This is for previous version(a, b) because HW pull TSIX
 *		low continuely after watchdog timeout reset
 *	else if (hx_zero_event == length) {
 *		//check zero flash status
 *		if (g_core_fp.fp_0f_esd_check() < 0) {
 *			g_zero_event_count = 6;
 *			I("[HIMAX TP MSG]: ESD event checked
				- ALL Zero in ZF.\n");
 *		} else {
 *			I("[HIMAX TP MSG]: Status check pass in ZF.\n");
 *		}
 *	}
 *#endif
 */
	else
		hx_excp_event = 0;

	if ((hx_excp_event == length || hx_zero_event == length)
		&& (HX_HW_RESET_ACTIVATE == 0)
		&& (HX_EXCP_RESET_ACTIVATE == 0)
		&& (hx_touch_data->diag_cmd == 0)
		&& (ts->in_self_test == 0)) {
		shaking_ret = g_core_fp.fp_ic_excp_recovery(
			hx_excp_event, hx_zero_event, length);

		if (shaking_ret == HX_EXCP_EVENT) {
			g_core_fp.fp_read_FW_status();
			himax_excp_hw_reset();
			ret_val = HX_EXCP_EVENT;
		} else if (shaking_ret == HX_ZERO_EVENT_COUNT) {
			g_core_fp.fp_read_FW_status();
			ret_val = HX_ZERO_EVENT_COUNT;
		} else {
			I("I2C running. Nothing to be done!\n");
			ret_val = HX_IC_RUNNING;
		}

	/* drop 1st interrupts after chip reset */
	} else if (HX_EXCP_RESET_ACTIVATE) {
		HX_EXCP_RESET_ACTIVATE = 0;
		I("%s: Skip by HX_EXCP_RESET_ACTIVATE.\n", __func__);
		ret_val = HX_EXCP_REC_OK;
	}

END_FUNCTION:
	if (g_ts_dbg != 0)
		I("%s: END, ret_val=%d!\n", __func__, ret_val);

	return ret_val;
}
#endif

static int himax_err_ctrl(struct himax_ts_data *ts,
		uint8_t *buf, int ts_path, int ts_status)
{
#if defined(HX_RST_PIN_FUNC)
	if (HX_HW_RESET_ACTIVATE) {
		/* drop 1st interrupts after chip reset */
		HX_HW_RESET_ACTIVATE = 0;
		I("[HX_HW_RESET_ACTIVATE]%s:Back from reset,ready to serve.\n",
				__func__);
		ts_status = HX_RST_OK;
		goto END_FUNCTION;
	}
#endif

	ts_status = himax_checksum_cal(ts, buf, ts_path, ts_status);
	if (ts_status == HX_CHKSUM_FAIL) {
		goto CHK_FAIL;
	} else {
#if defined(HX_EXCP_RECOVERY)
		/* continuous N times record, not total N times. */
		g_zero_event_count = 0;
#endif
		goto END_FUNCTION;
	}

CHK_FAIL:
#if defined(HX_EXCP_RECOVERY)
	ts_status = himax_ts_event_check(ts, buf, ts_path, ts_status);
#endif

END_FUNCTION:
	if (g_ts_dbg != 0)
		I("%s: END, ts_status=%d!\n", __func__, ts_status);
	return ts_status;
}
/* end error_control*/

/* start distribute_data*/
static int himax_distribute_touch_data(uint8_t *buf,
		int ts_path, int ts_status)
{
	uint8_t hx_state_info_pos = hx_touch_data->touch_info_size - 3;

	if (ic_data->HX_PEN_FUNC)
		hx_state_info_pos -= PEN_INFO_SZ;

	if (g_ts_dbg != 0)
		I("%s: Entering, ts_status=%d!\n", __func__, ts_status);

	if (ts_path == HX_REPORT_COORD) {
		memcpy(hx_touch_data->hx_coord_buf, &buf[0],
				hx_touch_data->touch_info_size);

		if (buf[hx_state_info_pos] != 0xFF
		&& buf[hx_state_info_pos + 1] != 0xFF)
			memcpy(hx_touch_data->hx_state_info,
					&buf[hx_state_info_pos], 2);
		else
			memset(hx_touch_data->hx_state_info, 0x00,
					sizeof(hx_touch_data->hx_state_info));

		if ((HX_HW_RESET_ACTIVATE)
#if defined(HX_EXCP_RECOVERY)
		|| (HX_EXCP_RESET_ACTIVATE)
#endif
		) {
			memcpy(hx_touch_data->hx_rawdata_buf,
					&buf[hx_touch_data->touch_info_size],
					hx_touch_data->touch_all_size
					- hx_touch_data->touch_info_size);
		}
	} else if (ts_path == HX_REPORT_COORD_RAWDATA) {
		memcpy(hx_touch_data->hx_coord_buf, &buf[0],
				hx_touch_data->touch_info_size);

		if (buf[hx_state_info_pos] != 0xFF
		&& buf[hx_state_info_pos + 1] != 0xFF)
			memcpy(hx_touch_data->hx_state_info,
					&buf[hx_state_info_pos], 2);
		else
			memset(hx_touch_data->hx_state_info, 0x00,
					sizeof(hx_touch_data->hx_state_info));

		memcpy(hx_touch_data->hx_rawdata_buf,
				&buf[hx_touch_data->touch_info_size],
				hx_touch_data->touch_all_size
				- hx_touch_data->touch_info_size);
#if defined(HX_SMART_WAKEUP)
	} else if (ts_path == HX_REPORT_SMWP_EVENT) {
		memcpy(hx_touch_data->hx_event_buf, buf,
				hx_touch_data->event_size);
#endif
	} else {
		E("%s, Fail Path!\n", __func__);
		ts_status = HX_PATH_FAIL;
	}

	if (g_ts_dbg != 0)
		I("%s: End, ts_status=%d!\n", __func__, ts_status);
	return ts_status;
}
/* end assign_data*/

/* start parse_report_data*/
int himax_parse_report_points(struct himax_ts_data *ts,
		int ts_path, int ts_status)
{
	int x = 0, y = 0, w = 0;

	uint8_t p_hover = 0, p_btn = 0, p_btn2 = 0;
	int8_t p_tilt_x = 0, p_tilt_y = 0;
	int p_x = 0, p_y = 0, p_w = 0;
	static uint8_t p_p_on;

	int base = 0;
	int32_t	loop_i = 0;

	if (g_ts_dbg != 0)
		I("%s: start!\n", __func__);


	if (!ic_data->HX_PEN_FUNC)
		goto skip_pen_operation;

	p_p_on = 0;
	base = hx_touch_data->touch_info_size - PEN_INFO_SZ;

	p_x = hx_touch_data->hx_coord_buf[base] << 8
		| hx_touch_data->hx_coord_buf[base + 1];
	p_y = (hx_touch_data->hx_coord_buf[base + 2] << 8
		| hx_touch_data->hx_coord_buf[base + 3]);
	p_w = (hx_touch_data->hx_coord_buf[base + 4] << 8
		| hx_touch_data->hx_coord_buf[base + 5]);
	p_tilt_x = (int8_t)hx_touch_data->hx_coord_buf[base + 6];
	p_hover = hx_touch_data->hx_coord_buf[base + 7];
	p_btn = hx_touch_data->hx_coord_buf[base + 8];
	p_btn2 = hx_touch_data->hx_coord_buf[base + 9];
	p_tilt_y = (int8_t)hx_touch_data->hx_coord_buf[base + 10];

	if (g_ts_dbg != 0) {
		D("%s: p_x=%d, p_y=%d, p_w=%d,p_tilt_x=%d, p_hover=%d\n",
			__func__, p_x, p_y, p_w, p_tilt_x, p_hover);
		D("%s: p_btn=%d, p_btn2=%d, p_tilt_y=%d\n",
			__func__, p_btn, p_btn2, p_tilt_y);
	}

	if (p_x >= 0
	&& p_x <= ts->pdata->abs_x_max
	&& p_y >= 0
	&& p_y <= ts->pdata->abs_y_max) {
		g_target_report_data->p_x[0] = p_x;
		g_target_report_data->p_y[0] = p_y;
		g_target_report_data->p_w[0] = p_w;
		g_target_report_data->p_hover[0] = p_hover;
		g_target_report_data->pen_id[0] = 1;
		g_target_report_data->p_btn[0] = p_btn;
		g_target_report_data->p_btn2[0] = p_btn2;
		g_target_report_data->p_tilt_x[0] = p_tilt_x;
		g_target_report_data->p_tilt_y[0] = p_tilt_y;
		g_target_report_data->p_on[0] = 1;
		ts->hx_point_num++;
	} else {/* report coordinates */
		g_target_report_data->p_x[0] = 0;
		g_target_report_data->p_y[0] = 0;
		g_target_report_data->p_w[0] = 0;
		g_target_report_data->p_hover[0] = 0;
		g_target_report_data->pen_id[0] = 0;
		g_target_report_data->p_btn[0] = 0;
		g_target_report_data->p_btn2[0] = 0;
		g_target_report_data->p_tilt_x[0] = 0;
		g_target_report_data->p_tilt_y[0] = 0;
		g_target_report_data->p_on[0] = 0;
	}

	if (g_ts_dbg != 0) {
		if (p_p_on != g_target_report_data->p_on[0]) {
			I("p_on[0] = %d, hx_point_num=%d\n",
				g_target_report_data->p_on[0],
				ts->hx_point_num);
			p_p_on = g_target_report_data->p_on[0];
		}
	}
skip_pen_operation:

	ts->old_finger = ts->pre_finger_mask;
	if (ts->hx_point_num == 0) {
		if (g_ts_dbg != 0)
			I("%s: hx_point_num = 0!\n", __func__);
		return ts_status;
	}
	ts->pre_finger_mask = 0;
	hx_touch_data->finger_num =
			hx_touch_data->hx_coord_buf[ts->coordInfoSize - 4]
			& 0x0F;
	hx_touch_data->finger_on = 1;
	AA_press = 1;

	g_target_report_data->finger_num = hx_touch_data->finger_num;
	g_target_report_data->finger_on = hx_touch_data->finger_on;
	g_target_report_data->ig_count =
			hx_touch_data->hx_coord_buf[ts->coordInfoSize - 5];

	if (g_ts_dbg != 0)
		I("%s:finger_num = 0x%2X, finger_on = %d\n", __func__,
				g_target_report_data->finger_num,
				g_target_report_data->finger_on);

	for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
		base = loop_i * 4;
		x = hx_touch_data->hx_coord_buf[base] << 8
			| hx_touch_data->hx_coord_buf[base + 1];
		y = (hx_touch_data->hx_coord_buf[base + 2] << 8
			| hx_touch_data->hx_coord_buf[base + 3]);
		w = hx_touch_data->hx_coord_buf[(ts->nFinger_support * 4)
			+ loop_i];

		if (g_ts_dbg != 0)
			D("%s: now parsing[%d]:x=%d, y=%d, w=%d\n", __func__,
					loop_i, x, y, w);

		if (x >= 0
		&& x <= ts->pdata->abs_x_max
		&& y >= 0
		&& y <= ts->pdata->abs_y_max) {
			hx_touch_data->finger_num--;

			g_target_report_data->x[loop_i] = x;
			g_target_report_data->y[loop_i] = y;
			g_target_report_data->w[loop_i] = w;
			g_target_report_data->finger_id[loop_i] = 1;

			/*I("%s: g_target_report_data->x[loop_i]=%d,*/
			/*g_target_report_data->y[loop_i]=%d,*/
			/*g_target_report_data->w[loop_i]=%d",*/
			/*__func__, g_target_report_data->x[loop_i],*/
			/*g_target_report_data->y[loop_i],*/
			/*g_target_report_data->w[loop_i]); */


			if (!ts->first_pressed) {
				ts->first_pressed = 1;
				I("S1@%d, %d\n", x, y);
			}

			ts->pre_finger_data[loop_i][0] = x;
			ts->pre_finger_data[loop_i][1] = y;

			ts->pre_finger_mask = ts->pre_finger_mask
					+ (1 << loop_i);
		} else {/* report coordinates */
			g_target_report_data->x[loop_i] = x;
			g_target_report_data->y[loop_i] = y;
			g_target_report_data->w[loop_i] = w;
			g_target_report_data->finger_id[loop_i] = 0;

			if (loop_i == 0 && ts->first_pressed == 1) {
				ts->first_pressed = 2;
				I("E1@%d, %d\n", ts->pre_finger_data[0][0],
						ts->pre_finger_data[0][1]);
			}
		}
	}

	if (g_ts_dbg != 0) {
		for (loop_i = 0; loop_i < 10; loop_i++)
			D("DBG X=%d  Y=%d ID=%d\n",
				g_target_report_data->x[loop_i],
				g_target_report_data->y[loop_i],
				g_target_report_data->finger_id[loop_i]);

		D("DBG finger number %d\n", g_target_report_data->finger_num);
	}

	if (g_ts_dbg != 0)
		I("%s: end!\n", __func__);
	return ts_status;
}

static int himax_parse_report_data(struct himax_ts_data *ts,
		int ts_path, int ts_status)
{

	if (g_ts_dbg != 0)
		I("%s: start now_status=%d!\n", __func__, ts_status);


	EN_NoiseFilter =
		(hx_touch_data->hx_coord_buf[HX_TOUCH_INFO_POINT_CNT + 2] >> 3);
	/* I("EN_NoiseFilter=%d\n", EN_NoiseFilter); */
	EN_NoiseFilter = EN_NoiseFilter & 0x01;
	/* I("EN_NoiseFilter2=%d\n", EN_NoiseFilter); */
	p_point_num = ts->hx_point_num;

	if (hx_touch_data->hx_coord_buf[HX_TOUCH_INFO_POINT_CNT] == 0xff)
		ts->hx_point_num = 0;
	else
		ts->hx_point_num =
			hx_touch_data->hx_coord_buf[HX_TOUCH_INFO_POINT_CNT]
			& 0x0f;

	switch (ts_path) {
	case HX_REPORT_COORD:
		ts_status = himax_parse_report_points(ts, ts_path, ts_status);
		break;
	case HX_REPORT_COORD_RAWDATA:
		/* touch monitor rawdata */
		if (debug_data != NULL) {
			if (debug_data->fp_set_diag_cmd(ic_data, hx_touch_data))
				I("%s:raw data_checksum not match\n", __func__);
		} else {
			E("%s,There is no init set_diag_cmd\n", __func__);
		}
		ts_status = himax_parse_report_points(ts, ts_path, ts_status);
		break;
#if defined(HX_SMART_WAKEUP)
	case HX_REPORT_SMWP_EVENT:
		himax_wake_event_parse(ts, ts_status);
		break;
#endif
	default:
		E("%s:Fail Path!\n", __func__);
		ts_status = HX_PATH_FAIL;
		break;
	}
	if (g_ts_dbg != 0)
		I("%s: end now_status=%d!\n", __func__, ts_status);
	return ts_status;
}

/* end parse_report_data*/

static void himax_report_all_leave_event(struct himax_ts_data *ts)
{
	int loop_i = 0;

	for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
#if !defined(HX_PROTOCOL_A)
		input_mt_slot(ts->input_dev, loop_i);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
#endif
	}
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_sync(ts->input_dev);
}

/* start report_point*/
static void himax_finger_report(struct himax_ts_data *ts)
{
	int i = 0;
	bool valid = false;


	if (g_ts_dbg != 0) {
		I("%s:start hx_touch_data->finger_num=%d\n",
			__func__, hx_touch_data->finger_num);
	}
	for (i = 0; i < ts->nFinger_support; i++) {
		if (g_target_report_data->x[i] >= 0
		&& g_target_report_data->x[i] <= ts->pdata->abs_x_max
		&& g_target_report_data->y[i] >= 0
		&& g_target_report_data->y[i] <= ts->pdata->abs_y_max)
			valid = true;
		else
			valid = false;
		if (g_ts_dbg != 0)
			I("valid=%d\n", valid);
		if (valid) {
			if (g_ts_dbg != 0) {
				I("report_data->x[i]=%d,y[i]=%d,w[i]=%d",
					g_target_report_data->x[i],
					g_target_report_data->y[i],
					g_target_report_data->w[i]);
			}
#if !defined(HX_PROTOCOL_A)
			input_mt_slot(ts->input_dev, i);
#else
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
#endif
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
					g_target_report_data->w[i]);
#if !defined(HX_PROTOCOL_A)
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
					g_target_report_data->w[i]);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
					g_target_report_data->w[i]);
#else
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, i);
#endif
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
					g_target_report_data->x[i]);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
					g_target_report_data->y[i]);
#if !defined(HX_PROTOCOL_A)
			ts->last_slot = i;
			input_mt_report_slot_state(ts->input_dev,
					MT_TOOL_FINGER, 1);
#else
			input_mt_sync(ts->input_dev);
#endif
		} else {
#if !defined(HX_PROTOCOL_A)
			input_mt_slot(ts->input_dev, i);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
			input_mt_report_slot_state(ts->input_dev,
					MT_TOOL_FINGER, 0);
#endif
		}
	}
#if !defined(HX_PROTOCOL_A)
	input_report_key(ts->input_dev, BTN_TOUCH, 1);
#endif
	input_sync(ts->input_dev);

	if (!ic_data->HX_PEN_FUNC)
		goto skip_pen_operation;

	valid = false;

	if (g_target_report_data->p_x[0] >= 0
	&& g_target_report_data->p_x[0] <= ts->pdata->abs_x_max
	&& g_target_report_data->p_y[0] >= 0
	&& g_target_report_data->p_y[0] <= ts->pdata->abs_y_max
	&& (g_target_report_data->p_on[0] == 1))
		valid = true;
	else
		valid = false;

	if (g_ts_dbg != 0)
		I("pen valid=%d\n", valid);

	if (valid) {/*Pen down*/
		if (g_ts_dbg != 0)
			I("p_x[i]=%d, p_y[i]=%d, p_w[i]=%d\n",
					g_target_report_data->p_x[0],
					g_target_report_data->p_y[0],
					g_target_report_data->p_w[0]);

		input_report_abs(ts->hx_pen_dev, ABS_X,
				g_target_report_data->p_x[0]);
		input_report_abs(ts->hx_pen_dev, ABS_Y,
				g_target_report_data->p_y[0]);

		if (g_target_report_data->p_btn[0] !=
		g_target_report_data->pre_p_btn) {
			if (g_ts_dbg != 0)
				I("BTN_STYLUS:%d\n",
					g_target_report_data->p_btn[0]);

			input_report_key(ts->hx_pen_dev, BTN_STYLUS,
					g_target_report_data->p_btn[0]);

			g_target_report_data->pre_p_btn =
					g_target_report_data->p_btn[0];
		} else {
			if (g_ts_dbg != 0)
				I("BTN_STYLUS status no change, value=%d!\n",
						g_target_report_data->p_btn[0]);
		}

		if (g_target_report_data->p_btn2[0]
		!= g_target_report_data->pre_p_btn2) {
			if (g_ts_dbg != 0)
				I("BTN_STYLUS2:%d\n",
					g_target_report_data->p_btn2[0]);

			input_report_key(ts->hx_pen_dev, BTN_STYLUS2,
					g_target_report_data->p_btn2[0]);

			g_target_report_data->pre_p_btn2 =
					g_target_report_data->p_btn2[0];
		} else {
			if (g_ts_dbg != 0)
				I("BTN_STYLUS2 status no change, value=%d!\n",
					g_target_report_data->p_btn2[0]);
		}
		input_report_abs(ts->hx_pen_dev, ABS_TILT_X,
				g_target_report_data->p_tilt_x[0]);

		input_report_abs(ts->hx_pen_dev, ABS_TILT_Y,
				g_target_report_data->p_tilt_y[0]);

		input_report_key(ts->hx_pen_dev, BTN_TOOL_PEN, 1);

		if (g_target_report_data->p_hover[0] == 0) {
			input_report_key(ts->hx_pen_dev, BTN_TOUCH, 1);
			input_report_abs(ts->hx_pen_dev, ABS_DISTANCE, 0);
			input_report_abs(ts->hx_pen_dev, ABS_PRESSURE,
					g_target_report_data->p_w[0]);
		} else {
			input_report_key(ts->hx_pen_dev, BTN_TOUCH, 0);
			input_report_abs(ts->hx_pen_dev, ABS_DISTANCE, 1);
			input_report_abs(ts->hx_pen_dev, ABS_PRESSURE, 0);
		}
	} else {/*Pen up*/
		g_target_report_data->pre_p_btn = 0;
		g_target_report_data->pre_p_btn2 = 0;
		input_report_key(ts->hx_pen_dev, BTN_STYLUS, 0);
		input_report_key(ts->hx_pen_dev, BTN_STYLUS2, 0);
		input_report_key(ts->hx_pen_dev, BTN_TOUCH, 0);
		input_report_abs(ts->hx_pen_dev, ABS_PRESSURE, 0);
		input_sync(ts->hx_pen_dev);

		input_report_abs(ts->hx_pen_dev, ABS_DISTANCE, 0);
		input_report_key(ts->hx_pen_dev, BTN_TOOL_RUBBER, 0);
		input_report_key(ts->hx_pen_dev, BTN_TOOL_PEN, 0);
		input_report_abs(ts->hx_pen_dev, ABS_PRESSURE, 0);
	}
	input_sync(ts->hx_pen_dev);

skip_pen_operation:

	if (g_ts_dbg != 0)
		I("%s:end\n", __func__);
}

static void himax_finger_leave(struct himax_ts_data *ts)
{
#if !defined(HX_PROTOCOL_A)
	int32_t loop_i = 0;
#endif

	if (g_ts_dbg != 0)
		I("%s: start!\n", __func__);
#if defined(HX_PALM_REPORT)
	if (himax_palm_detect(hx_touch_data->hx_coord_buf) == PALM_REPORT) {
		I(" %s HX_PALM_REPORT KEY power event press\n", __func__);
		input_report_key(ts->input_dev, KEY_POWER, 1);
		input_sync(ts->input_dev);
		msleep(100);

		I(" %s HX_PALM_REPORT KEY power event release\n", __func__);
		input_report_key(ts->input_dev, KEY_POWER, 0);
		input_sync(ts->input_dev);
		return;
	}
#endif

	hx_touch_data->finger_on = 0;
	g_target_report_data->finger_on  = 0;
	g_target_report_data->finger_num = 0;
	AA_press = 0;

#if defined(HX_PROTOCOL_A)
	input_mt_sync(ts->input_dev);
#endif
#if !defined(HX_PROTOCOL_A)
	for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
		input_mt_slot(ts->input_dev, loop_i);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
	}
#endif
	if (ts->pre_finger_mask > 0)
		ts->pre_finger_mask = 0;

	if (ts->first_pressed == 1) {
		ts->first_pressed = 2;
		I("E1@%d, %d\n", ts->pre_finger_data[0][0],
				ts->pre_finger_data[0][1]);
	}

	/*if (ts->debug_log_level & BIT(1))*/
	/*	himax_log_touch_event(x, y, w, loop_i, EN_NoiseFilter,*/
	/*			HX_FINGER_LEAVE); */

	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_sync(ts->input_dev);

	if (ic_data->HX_PEN_FUNC) {
		input_report_key(ts->hx_pen_dev, BTN_STYLUS, 0);
		input_report_key(ts->hx_pen_dev, BTN_TOUCH, 0);
		input_report_abs(ts->hx_pen_dev, ABS_PRESSURE, 0);
		input_sync(ts->hx_pen_dev);

		input_report_abs(ts->hx_pen_dev, ABS_DISTANCE, 0);
		input_report_abs(ts->hx_pen_dev, ABS_TILT_X, 0);
		input_report_abs(ts->hx_pen_dev, ABS_TILT_Y, 0);
		input_report_key(ts->hx_pen_dev, BTN_TOOL_RUBBER, 0);
		input_report_key(ts->hx_pen_dev, BTN_TOOL_PEN, 0);
		input_sync(ts->hx_pen_dev);
	}

	if (g_ts_dbg != 0)
		I("%s: end!\n", __func__);


}

static void himax_report_points(struct himax_ts_data *ts)
{
	if (g_ts_dbg != 0)
		I("%s: start!\n", __func__);

	if (ts->hx_point_num != 0)
		himax_finger_report(ts);
	else
		himax_finger_leave(ts);

	Last_EN_NoiseFilter = EN_NoiseFilter;

	if (g_ts_dbg != 0)
		I("%s: end!\n", __func__);
}
/* end report_points*/

int himax_report_data(struct himax_ts_data *ts, int ts_path, int ts_status)
{
	if (g_ts_dbg != 0)
		I("%s: Entering, ts_status=%d!\n", __func__, ts_status);

	if (ts_path == HX_REPORT_COORD || ts_path == HX_REPORT_COORD_RAWDATA) {
		/* Touch Point information */
		himax_report_points(ts);

#if defined(HX_SMART_WAKEUP)
	} else if (ts_path == HX_REPORT_SMWP_EVENT) {
		himax_wake_event_report();
#endif
	} else {
		E("%s:Fail Path!\n", __func__);
		ts_status = HX_PATH_FAIL;
	}

	if (g_ts_dbg != 0)
		I("%s: END, ts_status=%d!\n", __func__, ts_status);
	return ts_status;
}
/* end report_data */

static int himax_ts_operation(struct himax_ts_data *ts,
		int ts_path, int ts_status)
{
	uint8_t hw_reset_check[2];

	memset(ts->xfer_buff, 0x00, 128 * sizeof(uint8_t));
	memset(hw_reset_check, 0x00, sizeof(hw_reset_check));

	ts_status = himax_touch_get(ts, ts->xfer_buff, ts_path, ts_status);
	if (ts_status == HX_TS_GET_DATA_FAIL)
		goto END_FUNCTION;

	ts_status = himax_distribute_touch_data(ts->xfer_buff,
			ts_path, ts_status);
	ts_status = himax_err_ctrl(ts, ts->xfer_buff, ts_path, ts_status);
	if (ts_status == HX_REPORT_DATA || ts_status == HX_TS_NORMAL_END)
		ts_status = himax_parse_report_data(ts, ts_path, ts_status);
	else
		goto END_FUNCTION;


	ts_status = himax_report_data(ts, ts_path, ts_status);


END_FUNCTION:
	return ts_status;
}

void himax_ts_work(struct himax_ts_data *ts)
{

	int ts_status = HX_TS_NORMAL_END;
	int ts_path = 0;

	if (debug_data != NULL)
		debug_data->fp_ts_dbg_func(ts, HX_FINGER_ON);

#if defined(HX_USB_DETECT_GLOBAL) || defined(HX_TP_USB_NOTIFIER)
	himax_cable_detect_func(false);
#endif

	ts_path = himax_ts_work_status(ts);
	switch (ts_path) {
	case HX_REPORT_COORD:
		ts_status = himax_ts_operation(ts, ts_path, ts_status);
		break;
	case HX_REPORT_SMWP_EVENT:
		ts_status = himax_ts_operation(ts, ts_path, ts_status);
		break;
	case HX_REPORT_COORD_RAWDATA:
		ts_status = himax_ts_operation(ts, ts_path, ts_status);
		break;
	default:
		E("%s:Path Fault! value=%d\n", __func__, ts_path);
		goto END_FUNCTION;
	}

	if (ts_status == HX_TS_GET_DATA_FAIL)
		goto GET_TOUCH_FAIL;
	else
		goto END_FUNCTION;

GET_TOUCH_FAIL:
	I("%s: Now reset the Touch chip.\n", __func__);
#if defined(HX_RST_PIN_FUNC)
	g_core_fp.fp_ic_reset(false, true);
#else
	g_core_fp.fp_system_reset();
#endif
#if defined(HX_ZERO_FLASH)
	if (g_core_fp.fp_0f_reload_to_active)
		g_core_fp.fp_0f_reload_to_active();
#endif
END_FUNCTION:
	if (debug_data != NULL)
		debug_data->fp_ts_dbg_func(ts, HX_FINGER_LEAVE);

}
/*end ts_work*/
enum hrtimer_restart himax_ts_timer_func(struct hrtimer *timer)
{
	struct himax_ts_data *ts;


	ts = container_of(timer, struct himax_ts_data, timer);
	queue_work(ts->himax_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

#if defined(HX_BOOT_UPGRADE) || defined(HX_ZERO_FLASH)
#ifdef OPPO_PROC_NODE
static void tp_fw_update_work(struct work_struct *work)
{
	struct himax_ts_data *ts = private_ts;
	int ret = 0, count_tmp = 0,retry = 5;
	const struct firmware *fw_entry = NULL;

	if ( g_core_fp.fp_firmware_update_0f != NULL) {
		mutex_lock(&private_ts->fw_update_lock);
		do {
			if(ts->firmware_update_type == 0 || ts->firmware_update_type == 1) {
				I("%s:request ,name= %s\n", __func__,hx_fw->firmware_name);
				ret = request_firmware(&fw_entry, hx_fw->firmware_name, ts->dev);
				if (ret < 0) {
					I("%s: fw request fail \n", __func__);
				} else {
					break;
				}
			} else {
				I("%s:request sign fw ,name= %s\n", __func__,hx_fw->firmware_sign_name);
				ret = request_firmware_select(&fw_entry, hx_fw->firmware_sign_name, ts->dev);
				//ret = request_firmware(&fw_entry, hx_fw->firmware_sign_name, ts->dev);
				if (ret < 0) {
					I("%s: fw request fail \n", __func__);
				}else {
					break;
				}
			}
		} while((ret < 0) && (--retry > 0));

		I("retry times %d\n", 5 - retry);

		if (fw_entry) {
			if (ts->g_fw_buf !=NULL) {
				ts->g_fw_len = fw_entry->size;
				memcpy(ts->g_fw_buf, fw_entry->data, fw_entry->size);
				ts->g_fw_sta = true;
			}

			himax_int_enable(0);
			do {
                count_tmp++;
				ret = g_core_fp.fp_firmware_update_0f(fw_entry);
				if (ret == 0) {
                    break;
                }
			} while((count_tmp < 2) && (ret != 0));

			if ( ret == 0) {
				I("%s: fw update ok \n", __func__);
			} else {
				I("%s: fw update fail \n", __func__);
			}

			g_core_fp.fp_reload_disable(0);
			g_core_fp.fp_read_FW_ver();
			g_core_fp.fp_touch_information();
			g_core_fp.fp_sense_on(0x00);
			himax_int_enable(1);
		} else {
			I("%s: fw failed \n", __func__);
			goto EXIT;
		}

		if(fw_entry != NULL) {
			release_firmware(fw_entry);
		}
	} else {
		if(ts->firmware_update_type == 0 || ts->firmware_update_type == 1) {
			I("%s: fw_name request failed %s %d\n", __func__, hx_fw->firmware_name, ret);
		}else{
			I("%s: fw_name request failed %s %d\n", __func__, hx_fw->firmware_sign_name, ret);
		}
		goto EXIT;
	}

EXIT:
	mutex_unlock(&private_ts->fw_update_lock);
	complete(&ts->fw_complete); //notify to init.rc that fw update finished
	  return;
}
#endif

static void himax_boot_upgrade(struct work_struct *work)
{
	if (i_get_FW() != 0)
		return;

	g_core_fp.fp_bin_desc_get((unsigned char *)hxfw->data, HX1K);

	if (g_boot_upgrade_flag == true) {
		I("%s: Forced upgrade\n", __func__);
		goto UPDATE_FW;
	}

	if (himax_auto_update_check() != 0)
		goto SKIP_UPDATE_FW;

UPDATE_FW:
	if (i_update_FW() <= 0)
		E("%s: Update FW fail\n", __func__);
	else
		I("%s: Update FW success\n", __func__);

SKIP_UPDATE_FW:
#ifdef OPPO_PROC_NODE
	if(!private_ts->recovery_mode){
		if( hxfw != NULL) {
			release_firmware(hxfw);
			hxfw = NULL;
		}
	}
#else
	if( hxfw != NULL) {
		release_firmware(hxfw);
		hxfw = NULL;
	}
#endif
}
#endif

#if !defined(HX_ZERO_FLASH)
static int hx_chk_flash_sts(void)
{
	int rslt = 0;

	I("%s: Entering\n", __func__);

	rslt = (!g_core_fp.fp_calculateChecksum(false, FW_SIZE_128k));
	/*avoid the FW is full of zero*/
	rslt |= g_core_fp.fp_flash_lastdata_check(FW_SIZE_128k);

	return rslt;
}
#endif

#if defined(HX_CONFIG_FB) || defined(HX_CONFIG_DRM)
static void himax_fb_register(struct work_struct *work)
{
	int ret = 0;

	struct himax_ts_data *ts = container_of(work, struct himax_ts_data,
			work_att.work);

	I("%s in\n", __func__);
#if defined(HX_CONFIG_FB)
	ts->fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&ts->fb_notif);
#elif defined(HX_CONFIG_DRM)
#if defined(__HIMAX_MOD__)
	hx_msm_drm_register_client =
		(void *)kallsyms_lookup_name("msm_drm_register_client");
	if (hx_msm_drm_register_client != NULL) {
		ts->fb_notif.notifier_call = drm_notifier_callback;
		ret = hx_msm_drm_register_client(&ts->fb_notif);
	}	else
		E("hx_msm_drm_register_client is NULL\n");
#else
	ts->fb_notif.notifier_call = drm_notifier_callback;
	ret = msm_drm_register_client(&ts->fb_notif);
#endif
#endif
	if (ret)
		E("Unable to register fb_notifier: %d\n", ret);
}
#endif

#if defined(HX_CONTAINER_SPEED_UP)
static void himax_resume_work_func(struct work_struct *work)
{
	himax_chip_common_resume(private_ts);
}

#endif
int himax_chip_common_init(void)
{

	int i = 0, ret = 0, idx = 0;
	int err = PROBE_FAIL;
	struct himax_ts_data *ts = private_ts;
	struct himax_i2c_platform_data *pdata;
	struct himax_chip_entry *entry;

	I("Prepare kernel fp\n");
	kp_getname_kernel = (void *)kallsyms_lookup_name("getname_kernel");
	if (!kp_getname_kernel) {
		E("prepare kp_getname_kernel failed!\n");
		/*goto err_xfer_buff_fail;*/
	}
	kp_file_open_name = (void *)kallsyms_lookup_name("file_open_name");
	if (!kp_file_open_name) {
		E("prepare kp_file_open_name failed!\n");
		goto err_xfer_buff_fail;
	}
#if defined(__EMBEDDED_FW__)
	g_embedded_fw.size = (size_t)_binary___Himax_firmware_bin_end -
			(size_t)_binary___Himax_firmware_bin_start;
#endif
	ts->xfer_buff = devm_kzalloc(ts->dev, 128 * sizeof(uint8_t),
			GFP_KERNEL);
	if (ts->xfer_buff == NULL) {
		err = -ENOMEM;
		goto err_xfer_buff_fail;
	}

	I("PDATA START\n");
	pdata = kzalloc(sizeof(struct himax_i2c_platform_data), GFP_KERNEL);

	if (pdata == NULL) { /*Allocate Platform data space*/
		err = -ENOMEM;
		goto err_dt_platform_data_fail;
	}

	I("ic_data START\n");
	ic_data = kzalloc(sizeof(struct himax_ic_data), GFP_KERNEL);
	if (ic_data == NULL) { /*Allocate IC data space*/
		err = -ENOMEM;
		goto err_dt_ic_data_fail;
	}

	/* allocate report data */
	hx_touch_data = kzalloc(sizeof(struct himax_report_data), GFP_KERNEL);
	if (hx_touch_data == NULL) {
		err = -ENOMEM;
		goto err_alloc_touch_data_failed;
	}
#if defined(OPPO_PROC_NODE)
	ts->recovery_mode = false;
	ts->boot_mode = get_boot_mode();
	I("==========ts->boot_mode = %d\n",ts->boot_mode);
	if (ts->boot_mode == 2) {
		ts->recovery_mode = true;
		I("boot_mode is recovery\n");
	}
#endif
	ts->pdata = pdata;

	if (himax_parse_dt(ts, pdata) < 0) {
		I(" pdata is NULL for DT\n");
		goto err_alloc_dt_pdata_failed;
	}

#if defined(HX_RST_PIN_FUNC)
	ts->rst_gpio = pdata->gpio_reset;
#endif
	himax_gpio_power_config(pdata);
#if !defined(CONFIG_OF)

	if (pdata->power) {
		ret = pdata->power(1);

		if (ret < 0) {
			E("%s: power on failed\n", __func__);
			goto err_power_failed;
		}
	}

#endif

	g_hx_chip_inited = 0;
	idx = himax_get_ksym_idx();
	if (idx >= 0) {
		if (isEmpty(idx) != 0) {
			I("%s: no chip registered, please insmod ic.ko!\n",
					__func__);
			goto error_ic_detect_failed;
		}
		entry = get_chip_entry_by_index(idx);

		for (i = 0; i < entry->hx_ic_dt_num; i++) {
			if (entry->core_chip_dt[i].fp_chip_detect != NULL) {
				if (entry->core_chip_dt[i].fp_chip_detect()) {
					I("%s: chip found! list_num=%d\n",
							__func__, i);
					goto found_hx_chip;
				} else {
					I("%s:num=%d,chip NOT found! go Next\n",
							__func__, i);
					continue;
				}
			}
		}
	} else {
		I("%s: No available chip exist!\n", __func__);
		goto error_ic_detect_failed;
	}

	if (i == entry->hx_ic_dt_num) {
		E("%s: chip detect failed!\n", __func__);
		goto error_ic_detect_failed;
	}

found_hx_chip:
	if (g_core_fp.fp_chip_init != NULL) {
		g_core_fp.fp_chip_init();
	} else {
		E("%s: function point of chip_init is NULL!\n", __func__);
		goto error_ic_detect_failed;
	}

	if (pdata->virtual_key)
		ts->button = pdata->virtual_key;

#if defined(HX_ZERO_FLASH)
	g_boot_upgrade_flag = 1;
#else
	if (hx_chk_flash_sts() == 1) {
		E("%s: check flash fail, please upgrade FW\n", __func__);
	#if defined(HX_BOOT_UPGRADE)
		g_boot_upgrade_flag = 1;
	#endif
	} else {
		g_core_fp.fp_read_FW_ver();
	}
#endif

#if defined(HX_BOOT_UPGRADE) || defined(HX_ZERO_FLASH)
#if defined(OPPO_PROC_NODE)
		ts->firmware_headfile.data = hx_fw->fw_file;
		ts->firmware_headfile.size = hx_fw->fw_len;
		ts->g_fw_sta = false;
		ts->using_headfile = false;
		ts->g_fw_buf = vmalloc(128 * 1024);
		if (ts->g_fw_buf == NULL) {
			I("fw buf vmalloc error\n");
			goto err_g_fw_buf;
		}
		init_completion(&ts->fw_complete);
		INIT_WORK(&ts->fw_update_work, tp_fw_update_work);
#endif
		ts->himax_boot_upgrade_wq =
			create_singlethread_workqueue("HX_boot_upgrade");
		if (!ts->himax_boot_upgrade_wq) {
			E("allocate himax_boot_upgrade_wq failed\n");
			err = -ENOMEM;
			goto err_boot_upgrade_wq_failed;
		}
		INIT_DELAYED_WORK(&ts->work_boot_upgrade, himax_boot_upgrade);
#if defined(OPPO_HX_UPDATE_FW) && defined(OPPO_PROC_NODE)
		if(private_ts->recovery_mode){
			queue_delayed_work(ts->himax_boot_upgrade_wq, &ts->work_boot_upgrade,
				msecs_to_jiffies(500));
		}
#else
		queue_delayed_work(ts->himax_boot_upgrade_wq, &ts->work_boot_upgrade,
				msecs_to_jiffies(2000));
#endif
#endif

#if defined(HX_CONTAINER_SPEED_UP)
	ts->ts_int_workqueue =
			create_singlethread_workqueue("himax_ts_resume_wq");
	if (!ts->ts_int_workqueue) {
		E("%s: create ts_resume workqueue failed\n", __func__);
		goto err_create_ts_resume_wq_failed;
	}
	INIT_DELAYED_WORK(&ts->ts_int_work, himax_resume_work_func);
#endif

	/*Himax Power On and Load Config*/
/*	if (himax_loadSensorConfig(pdata)) {
 *		E("%s: Load Sesnsor configuration failed, unload driver.\n",
 *				__func__);
 *		goto err_detect_failed;
 *	}
 */
	g_core_fp.fp_power_on_init();
	calculate_point_number();
	mutex_init(&ts->fw_update_lock);

#if defined(CONFIG_OF)
	ts->power = pdata->power;
#endif

	/*calculate the i2c data size*/
	calcDataSize();
	I("%s: calcDataSize complete\n", __func__);

#if defined(CONFIG_OF)
	ts->pdata->abs_pressure_min        = 0;
	ts->pdata->abs_pressure_max        = 200;
	ts->pdata->abs_width_min           = 0;
	ts->pdata->abs_width_max           = 200;
	pdata->cable_config[0]             = 0xF0;
	pdata->cable_config[1]             = 0x00;
#endif

	ts->suspended                      = false;
#if defined(HX_USB_DETECT_GLOBAL) || defined(HX_TP_USB_NOTIFIER)
	ts->usb_connected = 0x00;
	ts->cable_config = pdata->cable_config;
#endif

#if defined(HX_PROTOCOL_A)
	ts->protocol_type = PROTOCOL_TYPE_A;
#else
	ts->protocol_type = PROTOCOL_TYPE_B;
#endif
	I("%s: Use Protocol Type %c\n", __func__,
		ts->protocol_type == PROTOCOL_TYPE_A ? 'A' : 'B');

	ret = himax_input_register(ts);
	if (ret) {
		E("%s: Unable to register %s input device\n",
			__func__, ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	spin_lock_init(&ts->irq_lock);
	ts->initialized = true;

#if defined(HX_CONFIG_FB) || defined(HX_CONFIG_DRM)
	ts->himax_att_wq = create_singlethread_workqueue("HMX_ATT_request");

	if (!ts->himax_att_wq) {
		E(" allocate himax_att_wq failed\n");
		err = -ENOMEM;
		goto err_get_intr_bit_failed;
	}

	INIT_DELAYED_WORK(&ts->work_att, himax_fb_register);
	queue_delayed_work(ts->himax_att_wq, &ts->work_att,
			msecs_to_jiffies(15000));
#endif

#if defined(HX_SMART_WAKEUP)
	ts->SMWP_enable = 0;
	wakeup_source_init(&ts->ts_SMWP_wake_lock, HIMAX_common_NAME);
#endif
#if defined(HX_HIGH_SENSE)
	ts->HSEN_enable = 0;
#endif

	/*touch data init*/
	err = himax_report_data_init();

	if (err)
		goto err_report_data_init_failed;

#if defined(OPPO_PROC_NODE)
	if (OPPO_proc_node_init()) {
		E(" %s:OPPO_proc_node_init failed!\n", __func__);
		goto err_creat_oppo_proc_failed;
	}

	ts->himax_Freq_hop_test_wq = create_singlethread_workqueue("HIMAX_FREQ_hop_test_wq");

	if (!ts->himax_Freq_hop_test_wq) {
		E(" allocate himax_Freq_hop_test_wq failed\n");
		goto err_get_Freq_hop_test_wq_failed;
	}

	INIT_DELAYED_WORK(&ts->himax_Freq_hop_test_wrok, himax_Freq_hop_test_sub);
	ts->Freq_hop_delay_work_start = 0;

#ifdef HX_UPDATE_FW_NOTIFIER
	INIT_WORK(&ts->resume_work_queue, himax_notifie_resume_workqueue);
	himax_update_fw_notifier_init();
#endif

#ifdef HX_TP_USB_NOTIFIER
	//INIT_WORK(&ts->usb_detect_work_queue, himax_usb_detect_workqueue);
	himax_tp_usb_notifier_init();
#endif

#ifdef HX_TP_HEADSET_NOTIFIER
	INIT_WORK(&ts->headset_work_queue, himax_tp_headset_workqueue);
	himax_tp_headset_notifier_init();
#endif
#endif

	if (himax_common_proc_init()) {
		E(" %s: himax_common proc_init failed!\n", __func__);
		goto err_creat_proc_file_failed;
	}

	himax_ts_register_interrupt();

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
	if (himax_debug_init())
		E(" %s: debug initial failed!\n", __func__);
#endif

#if defined(HX_BOOT_UPGRADE) || defined(HX_ZERO_FLASH)
	if (g_boot_upgrade_flag)
		himax_int_enable(0);
#endif

	//ts->boot_mode = get_boot_mode();
	I("%s: ==========idev->boot_mode = %d\n", __func__, ts->boot_mode);
	if ((ts->boot_mode == 3 || ts->boot_mode == 4 || ts->boot_mode == 5)) {
		I("%s: boot_mode is FACTORY,RF and WLAN not need to enable irq\n", __func__);
		himax_int_enable(0);/* disable irq */
	}

	g_hx_chip_inited = true;
	tpd_load_status = 1;

	return 0;

err_creat_proc_file_failed:

#if defined(OPPO_PROC_NODE)
cancel_delayed_work_sync(&ts->himax_Freq_hop_test_wrok);
destroy_workqueue(ts->himax_Freq_hop_test_wq);
err_get_Freq_hop_test_wq_failed:
OPPO_proc_node_deinit();
err_creat_oppo_proc_failed:
#endif

	himax_report_data_deinit();
err_report_data_init_failed:
#if defined(HX_SMART_WAKEUP)
	wakeup_source_trash(&ts->ts_SMWP_wake_lock);
#endif
#if defined(HX_CONFIG_FB) || defined(HX_CONFIG_DRM)
	cancel_delayed_work_sync(&ts->work_att);
	destroy_workqueue(ts->himax_att_wq);
err_get_intr_bit_failed:
#endif
err_input_register_device_failed:
	input_free_device(ts->input_dev);
/*err_detect_failed:*/

#if defined(HX_CONTAINER_SPEED_UP)
	cancel_delayed_work_sync(&ts->ts_int_work);
	destroy_workqueue(ts->ts_int_workqueue);
err_create_ts_resume_wq_failed:
#endif

#if defined(HX_BOOT_UPGRADE) || defined(HX_ZERO_FLASH)
#if defined(OPPO_HX_UPDATE_FW) && defined(OPPO_PROC_NODE)
	if(private_ts->recovery_mode){
		cancel_delayed_work_sync(&ts->work_boot_upgrade);
		destroy_workqueue(ts->himax_boot_upgrade_wq);
		}
#else
	cancel_delayed_work_sync(&ts->work_boot_upgrade);
	destroy_workqueue(ts->himax_boot_upgrade_wq);
#endif

err_boot_upgrade_wq_failed:
#endif

#if defined(OPPO_PROC_NODE)
	if (ts->g_fw_buf) {
        vfree(ts->g_fw_buf);
    }
err_g_fw_buf:
#endif
error_ic_detect_failed:
	himax_gpio_power_deconfig(pdata);
#if !defined(CONFIG_OF)
err_power_failed:
#endif
err_alloc_dt_pdata_failed:
	kfree(hx_touch_data);
	hx_touch_data = NULL;
err_alloc_touch_data_failed:
	kfree(ic_data);
	ic_data = NULL;
err_dt_ic_data_fail:
	kfree(pdata);
	pdata = NULL;
err_dt_platform_data_fail:
	devm_kfree(ts->dev, ts->xfer_buff);
	ts->xfer_buff = NULL;
err_xfer_buff_fail:
	probe_fail_flag = 1;
	return err;
}

void himax_chip_common_deinit(void)
{
	struct himax_ts_data *ts = private_ts;

	himax_ts_unregister_interrupt();

#if defined(CONFIG_TOUCHSCREEN_HIMAX_INSPECT)
	himax_inspect_data_clear();
#endif

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
	himax_debug_remove();
#endif

	himax_common_proc_deinit();
#if defined(OPPO_PROC_NODE)
	OPPO_proc_node_deinit();
#endif
	himax_report_data_deinit();

#if defined(HX_SMART_WAKEUP)
	wakeup_source_trash(&ts->ts_SMWP_wake_lock);
#endif
#if defined(HX_CONFIG_FB)
	if (fb_unregister_client(&ts->fb_notif))
		E("Error occurred while unregistering fb_notifier.\n");
	cancel_delayed_work_sync(&ts->work_att);
	destroy_workqueue(ts->himax_att_wq);
#elif defined(HX_CONFIG_DRM)
#if defined(__HIMAX_MOD__)
	hx_msm_drm_unregister_client =
		(void *)kallsyms_lookup_name("msm_drm_unregister_client");
	if (hx_msm_drm_unregister_client != NULL) {
		if (hx_msm_drm_unregister_client(&ts->fb_notif))
			E("Error occurred while unregistering drm_notifier.\n");
	} else
		E("hx_msm_drm_unregister_client is NULL\n");
#else
	if (msm_drm_unregister_client(&ts->fb_notif))
		E("Error occurred while unregistering drm_notifier.\n");
#endif
	cancel_delayed_work_sync(&ts->work_att);
	destroy_workqueue(ts->himax_att_wq);
#endif
	input_free_device(ts->input_dev);
#if defined(HX_CONTAINER_SPEED_UP)
	cancel_delayed_work_sync(&ts->ts_int_work);
	destroy_workqueue(ts->ts_int_workqueue);
#endif

#if defined(HX_BOOT_UPGRADE) || defined(HX_ZERO_FLASH)
	cancel_delayed_work_sync(&ts->work_boot_upgrade);
	destroy_workqueue(ts->himax_boot_upgrade_wq);
#endif
	himax_gpio_power_deconfig(ts->pdata);
	if (himax_mcu_cmd_struct_free)
		himax_mcu_cmd_struct_free();

	kfree(hx_touch_data);
	hx_touch_data = NULL;
	kfree(ic_data);
	ic_data = NULL;
	kfree(ts->pdata->virtual_key);
	ts->pdata->virtual_key = NULL;
	devm_kfree(ts->dev, ts->xfer_buff);
	ts->xfer_buff = NULL;
	kfree(ts->pdata);
	ts->pdata = NULL;
	kfree(ts);
	ts = NULL;
	probe_fail_flag = 0;

	I("%s: Common section deinited!\n", __func__);
}

int himax_chip_common_suspend(struct himax_ts_data *ts)
{
	if (ts->suspended) {
		I("%s: Already suspended. Skipped.\n", __func__);
		goto END;
	} else {
		ts->suspended = true;
		I("%s: enter\n", __func__);
	}

	if (debug_data != NULL && debug_data->flash_dump_going == true) {
		I("[himax] %s: Flash dump is going, reject suspend\n",
				__func__);
		goto END;
	}

#if defined(HX_SMART_WAKEUP)\
	|| defined(HX_HIGH_SENSE)\
	|| defined(HX_USB_DETECT_GLOBAL) || defined(HX_TP_USB_NOTIFIER)
#if !defined(HX_RESUME_SEND_CMD)
	g_core_fp.fp_resend_cmd_func(ts->suspended);
#endif
#endif
#if defined(HX_SMART_WAKEUP)

	if (ts->SMWP_enable) {
#if defined(HX_CODE_OVERLAY)
		if (ts->in_self_test == 0)
			g_core_fp.fp_0f_overlay(2, 0);
#endif

		atomic_set(&ts->suspend_mode, 1);
		ts->pre_finger_mask = 0;
		I("[himax] %s: SMART_WAKEUP enable, reject suspend\n",
				__func__);
		goto END;
	}

#endif
	himax_int_enable(0);
	if (g_core_fp.fp_suspend_ic_action != NULL)
		g_core_fp.fp_suspend_ic_action();

	if (!ts->use_irq) {
		int32_t cancel_state;

		cancel_state = cancel_work_sync(&ts->work);
		if (cancel_state)
			himax_int_enable(1);
	}

	/*ts->first_pressed = 0;*/
	atomic_set(&ts->suspend_mode, 1);
	ts->pre_finger_mask = 0;

	if (ts->pdata)
		if (ts->pdata->powerOff3V3 && ts->pdata->power)
			ts->pdata->power(0);

END:
	if (ts->in_self_test == 1)
		ts->suspend_resume_done = 1;
#if defined(OPPO_PROC_NODE)
	ts->suspend_state = TP_SUSPEND_COMPLETE;
#endif
	I("%s: END\n", __func__);

	return 0;
}

int himax_chip_common_resume(struct himax_ts_data *ts)
{
#if defined(HX_ZERO_FLASH) && defined(HX_RESUME_SET_FW)
	int result = 0;
#endif
	I("%s: enter\n", __func__);

	if (ts->suspended == false) {
		I("%s: It had entered resume, skip this step\n", __func__);
		goto END;
	} else {
		ts->suspended = false;
	}

#if defined(HX_EXCP_RECOVERY)
		/* continuous N times record, not total N times. */
		g_zero_event_count = 0;
#endif

	atomic_set(&ts->suspend_mode, 0);
	ts->diag_cmd = 0;

	if (ts->pdata)
		if (ts->pdata->powerOff3V3 && ts->pdata->power)
			ts->pdata->power(1);

#if defined(HX_RST_PIN_FUNC) && defined(HX_RESUME_HW_RESET)
	if (g_core_fp.fp_ic_reset != NULL)
		g_core_fp.fp_ic_reset(false, false);
#endif

#if defined(HX_ZERO_FLASH) && defined(HX_RESUME_SET_FW)
		private_ts->in_self_test = 0;
		mutex_lock(&private_ts->fw_update_lock);
		I("It will update fw after resume in zero flash mode!\n");
		if (g_core_fp.fp_0f_operation_dirly != NULL) {
			result = g_core_fp.fp_0f_operation_dirly();
			if (result) {
				E("Something wrong! Skip Update zero flash!\n");
				mutex_unlock(&private_ts->fw_update_lock);
				goto ESCAPE_0F_UPDATE;
			}
		}
		if (g_core_fp.fp_reload_disable != NULL)
			g_core_fp.fp_reload_disable(0);
		if (g_core_fp.fp_sense_on != NULL)
			g_core_fp.fp_sense_on(0x00);
		mutex_unlock(&private_ts->fw_update_lock);
#endif
#if defined(HX_SMART_WAKEUP)\
	|| defined(HX_HIGH_SENSE)\
	|| defined(HX_USB_DETECT_GLOBAL) || defined(HX_TP_USB_NOTIFIER)
	if (g_core_fp.fp_resend_cmd_func != NULL)
		g_core_fp.fp_resend_cmd_func(ts->suspended);

#endif
	himax_report_all_leave_event(ts);

	himax_int_enable(1);
#if defined(HX_ZERO_FLASH) && defined(HX_RESUME_SET_FW)
ESCAPE_0F_UPDATE:
#endif
END:
	if (ts->in_self_test == 1)
		ts->suspend_resume_done = 1;
#if defined(OPPO_PROC_NODE)
    ts->suspend_state = TP_SPEEDUP_RESUME_COMPLETE;

#endif
	I("%s: END\n", __func__);
	return 0;
}

