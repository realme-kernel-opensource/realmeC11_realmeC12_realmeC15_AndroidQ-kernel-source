/*
 * Copyright (c) 2019 Guangdong OPPO Mobile Communication(Shanghai)
 * Corp.,Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------- Revision History: ----------------------------
 * <author>                   <date>       <version>   <desc>
 * fangxiang@oppo.com         2019/11/19   1.0         Add thermal_zone:mtktsboard
 * ------------------------------------------------------------------------------
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <mt-plat/aee.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
#include <linux/slab.h>
#include <tmp_board.h>
#include "mt-plat/mtk_thermal_monitor.h"
#include "mt-plat/mtk_thermal_platform.h"

static unsigned int interval = 1;	/* seconds, 0 : no auto polling */
static int trip_temp[10] = { 120000, 110000, 100000, 90000, 80000,
				70000, 65000, 60000, 55000, 50000 };

static struct thermal_zone_device *thz_dev;
static int mtkts_board_debug_log;
static int kernelmode;
static int g_THERMAL_TRIP[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static int num_trip = 1;

/**
 * If curr_temp >= polling_trip_temp1, use interval
 * else if cur_temp >= polling_trip_temp2 && curr_temp < polling_trip_temp1,
 *	use interval*polling_factor1
 * else, use interval*polling_factor2
 */
static int polling_trip_temp1 = 40000;
static int polling_trip_temp2 = 20000;
static int polling_factor1 = 5000;
static int polling_factor2 = 10000;

int board_cur_temp = 1;


#define mtkts_board_TEMP_CRIT 60000	/* 60.000 degree Celsius */

#define mtkts_board_dprintk(fmt, args...)   \
do {                                    \
	if (mtkts_board_debug_log) {                \
		pr_debug("[Thermal/TZ/BOARD]" fmt, ##args); \
	}                                   \
} while (0)


#define mtkts_board_printk(fmt, args...) \
pr_debug("[Thermal/TZ/BOARD]" fmt, ##args)

struct BOARD_TEMPERATURE {
	__s32 BOARD_Temp;
	__s32 TemperatureR;
};

static int g_RAP_pull_up_R = BOARD_RAP_PULL_UP_R;
static int g_TAP_over_critical_low = BOARD_TAP_OVER_CRITICAL_LOW;
static int g_RAP_pull_up_voltage = BOARD_RAP_PULL_UP_VOLTAGE;
static int g_RAP_ntc_table = BOARD_RAP_NTC_TABLE;
static int g_RAP_ADC_channel = BOARD_RAP_ADC_CHANNEL;
static int g_AP_TemperatureR;

static struct BOARD_TEMPERATURE *BOARD_Temperature_Table;
static int ntc_tbl_size;

/* AP_NTC_BL197 */
static struct BOARD_TEMPERATURE BOARD_Temperature_Table1[] = {
	{-40, 74354},		/* FIX_ME */
	{-35, 74354},		/* FIX_ME */
	{-30, 74354},		/* FIX_ME */
	{-25, 74354},		/* FIX_ME */
	{-20, 74354},
	{-15, 57626},
	{-10, 45068},
	{-5, 35548},
	{0, 28267},
	{5, 22650},
	{10, 18280},
	{15, 14855},
	{20, 12151},
	{25, 10000},		/* 10K */
	{30, 8279},
	{35, 6892},
	{40, 5768},
	{45, 4852},
	{50, 4101},
	{55, 3483},
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970}		/* FIX_ME */
};

/* AP_NTC_TSM_1 */
static struct BOARD_TEMPERATURE BOARD_Temperature_Table2[] = {
	{-40, 70603},		/* FIX_ME */
	{-35, 70603},		/* FIX_ME */
	{-30, 70603},		/* FIX_ME */
	{-25, 70603},		/* FIX_ME */
	{-20, 70603},
	{-15, 55183},
	{-10, 43499},
	{-5, 34569},
	{0, 27680},
	{5, 22316},
	{10, 18104},
	{15, 14773},
	{20, 12122},
	{25, 10000},		/* 10K */
	{30, 8294},
	{35, 6915},
	{40, 5795},
	{45, 4882},
	{50, 4133},
	{55, 3516},
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004},		/* FIX_ME */
	{60, 3004}		/* FIX_ME */
};

/* AP_NTC_10_SEN_1 */
static struct BOARD_TEMPERATURE BOARD_Temperature_Table3[] = {
	{-40, 74354},		/* FIX_ME */
	{-35, 74354},		/* FIX_ME */
	{-30, 74354},		/* FIX_ME */
	{-25, 74354},		/* FIX_ME */
	{-20, 74354},
	{-15, 57626},
	{-10, 45068},
	{-5, 35548},
	{0, 28267},
	{5, 22650},
	{10, 18280},
	{15, 14855},
	{20, 12151},
	{25, 10000},		/* 10K */
	{30, 8279},
	{35, 6892},
	{40, 5768},
	{45, 4852},
	{50, 4101},
	{55, 3483},
	{60, 2970},
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970},		/* FIX_ME */
	{60, 2970}		/* FIX_ME */
};

/* AP_NTC_10(TSM0A103F34D1RZ) */
static struct BOARD_TEMPERATURE BOARD_Temperature_Table4[] = {
	{-40, 188500},
	{-35, 144290},
	{-30, 111330},
	{-25, 86560},
	{-20, 67790},
	{-15, 53460},
	{-10, 42450},
	{-5, 33930},
	{0, 27280},
	{5, 22070},
	{10, 17960},
	{15, 14700},
	{20, 12090},
	{25, 10000},		/* 10K */
	{30, 8310},
	{35, 6940},
	{40, 5830},
	{45, 4910},
	{50, 4160},
	{55, 3540},
	{60, 3020},
	{65, 2590},
	{70, 2230},
	{75, 1920},
	{80, 1670},
	{85, 1450},
	{90, 1270},
	{95, 1110},
	{100, 975},
	{105, 860},
	{110, 760},
	{115, 674},
	{120, 599},
	{125, 534}
};

/* AP_NTC_47 */
static struct BOARD_TEMPERATURE BOARD_Temperature_Table5[] = {
	{-40, 483954},		/* FIX_ME */
	{-35, 483954},		/* FIX_ME */
	{-30, 483954},		/* FIX_ME */
	{-25, 483954},		/* FIX_ME */
	{-20, 483954},
	{-15, 360850},
	{-10, 271697},
	{-5, 206463},
	{0, 158214},
	{5, 122259},
	{10, 95227},
	{15, 74730},
	{20, 59065},
	{25, 47000},		/* 47K */
	{30, 37643},
	{35, 30334},
	{40, 24591},
	{45, 20048},
	{50, 16433},
	{55, 13539},
	{60, 11210},
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210},		/* FIX_ME */
	{60, 11210}		/* FIX_ME */
};


/* NTCG104EF104F(100K) */
static struct BOARD_TEMPERATURE BOARD_Temperature_Table6[] = {
	{-40, 4251000},
	{-35, 3005000},
	{-30, 2149000},
	{-25, 1554000},
	{-20, 1135000},
	{-15, 837800},
	{-10, 624100},
	{-5, 469100},
	{0, 355600},
	{5, 271800},
	{10, 209400},
	{15, 162500},
	{20, 127000},
	{25, 100000},		/* 100K */
	{30, 79230},
	{35, 63180},
	{40, 50680},
	{45, 40900},
	{50, 33190},
	{55, 27090},
	{60, 22220},
	{65, 18320},
	{70, 15180},
	{75, 12640},
	{80, 10580},
	{85, 8887},
	{90, 7500},
	{95, 6357},
	{100, 5410},
	{105, 4623},
	{110, 3965},
	{115, 3415},
	{120, 2951},
	{125, 2560}
};

/* NCP15WF104F03RC(100K) */
static struct BOARD_TEMPERATURE BOARD_Temperature_Table7[] = {
	{-40, 4397119},
	{-35, 3088599},
	{-30, 2197225},
	{-25, 1581881},
	{-20, 1151037},
	{-15, 846579},
	{-10, 628988},
	{-5, 471632},
	{0, 357012},
	{5, 272500},
	{10, 209710},
	{15, 162651},
	{20, 127080},
	{25, 100000},		/* 100K */
	{30, 79222},
	{35, 63167},
#if defined(APPLY_PRECISE_NTC_TABLE)
	{40, 50677},
	{41, 48528},
	{42, 46482},
	{43, 44533},
	{44, 42675},
	{45, 40904},
	{46, 39213},
	{47, 37601},
	{48, 36063},
	{49, 34595},
	{50, 33195},
	{51, 31859},
	{52, 30584},
	{53, 29366},
	{54, 28203},
	{55, 27091},
	{56, 26028},
	{57, 25013},
	{58, 24042},
	{59, 23113},
	{60, 22224},
	{61, 21374},
	{62, 20560},
	{63, 19782},
	{64, 19036},
	{65, 18322},
	{66, 17640},
	{67, 16986},
	{68, 16360},
	{69, 15759},
	{70, 15184},
	{71, 14631},
	{72, 14100},
	{73, 13591},
	{74, 13103},
	{75, 12635},
	{76, 12187},
	{77, 11756},
	{78, 11343},
	{79, 10946},
	{80, 10565},
	{81, 10199},
	{82,  9847},
	{83,  9509},
	{84,  9184},
	{85,  8872},
	{86,  8572},
	{87,  8283},
	{88,  8005},
	{89,  7738},
	{90,  7481},
#else
	{40, 50677},
	{45, 40904},
	{50, 33195},
	{55, 27091},
	{60, 22224},
	{65, 18323},
	{70, 15184},
	{75, 12635},
	{80, 10566},
	{85, 8873},
	{90, 7481},
#endif
	{95, 6337},
	{100, 5384},
	{105, 4594},
	{110, 3934},
	{115, 3380},
	{120, 2916},
	{125, 2522}
};


/* convert register to temperature  */
static __s16 mtkts_board_thermistor_conver_temp(__s32 Res)
{
	int i = 0;
	int asize = 0;
	__s32 RES1 = 0, RES2 = 0;
	__s32 TAP_Value = -200, TMP1 = 0, TMP2 = 0;

	asize = (ntc_tbl_size / sizeof(struct BOARD_TEMPERATURE));

	/* mtkts_board_dprintk("mtkts_board_thermistor_conver_temp() :
	 * asize = %d, Res = %d\n",asize,Res);
	 */
	if (Res >= BOARD_Temperature_Table[0].TemperatureR) {
		TAP_Value = -40;	/* min */
	} else if (Res <= BOARD_Temperature_Table[asize - 1].TemperatureR) {
		TAP_Value = 125;	/* max */
	} else {
		RES1 = BOARD_Temperature_Table[0].TemperatureR;
		TMP1 = BOARD_Temperature_Table[0].BOARD_Temp;
		/* mtkts_board_dprintk("%d : RES1 = %d,TMP1 = %d\n",
		 * __LINE__,RES1,TMP1);
		 */

		for (i = 0; i < asize; i++) {
			if (Res >= BOARD_Temperature_Table[i].TemperatureR) {
				RES2 = BOARD_Temperature_Table[i].TemperatureR;
				TMP2 = BOARD_Temperature_Table[i].BOARD_Temp;
				/* mtkts_board_dprintk("%d :i=%d, RES2 = %d,
				 * TMP2 = %d\n",__LINE__,i,RES2,TMP2);
				 */
				break;
			}
			RES1 = BOARD_Temperature_Table[i].TemperatureR;
			TMP1 = BOARD_Temperature_Table[i].BOARD_Temp;
			/* mtkts_board_dprintk("%d :i=%d, RES1 = %d,TMP1 = %d\n",
			 * __LINE__,i,RES1,TMP1);
			 */
		}

		TAP_Value = (((Res - RES2) * TMP1) + ((RES1 - Res) * TMP2))
								/ (RES1 - RES2);
	}

	return TAP_Value;
}

/* convert ADC_AP_temp_volt to register */
/*Volt to Temp formula same with 6589*/
static __s16 mtk_ts_board_volt_to_temp(__u32 dwVolt)
{
	__s32 TRes;
	__u64 dwVCriAP = 0;
	__s32 BOARD_TMP = -100;
	__u64 dwVCriAP2 = 0;
	/* SW workaround-----------------------------------------------------
	 * dwVCriAP = (TAP_OVER_CRITICAL_LOW * 1800) /
	 *		(TAP_OVER_CRITICAL_LOW + 39000);
	 * dwVCriAP = (TAP_OVER_CRITICAL_LOW * RAP_PULL_UP_VOLT) /
	 *		(TAP_OVER_CRITICAL_LOW + RAP_PULL_UP_R);
	 */

	dwVCriAP = ((__u64)g_TAP_over_critical_low *
		(__u64)g_RAP_pull_up_voltage);
	dwVCriAP2 = (g_TAP_over_critical_low + g_RAP_pull_up_R);
	do_div(dwVCriAP, dwVCriAP2);


	if (dwVolt > ((__u32)dwVCriAP)) {
		TRes = g_TAP_over_critical_low;
	} else {
		/* TRes = (39000*dwVolt) / (1800-dwVolt); */
		/* TRes = (RAP_PULL_UP_R*dwVolt) / (RAP_PULL_UP_VOLT-dwVolt); */
		TRes = (g_RAP_pull_up_R * dwVolt) /
					(g_RAP_pull_up_voltage - dwVolt);
	}
	/* ------------------------------------------------------------------ */
	g_AP_TemperatureR = TRes;

	/* convert register to temperature */
	BOARD_TMP = mtkts_board_thermistor_conver_temp(TRes);

	return BOARD_TMP;
}

static int get_hw_board_temp(void)
{


	int ret = 0, data[4], i, ret_value = 0, ret_temp = 0, output;
	int times = 1, Channel = g_RAP_ADC_channel; /* 6752=0(AUX_IN0_NTC) */
	static int valid_temp;

#if defined(APPLY_AUXADC_CALI_DATA)
	int auxadc_cali_temp;
#endif

	if (IMM_IsAdcInitReady() == 0) {
		mtkts_board_printk(
			"[thermal_auxadc_get_data]: AUXADC is not ready\n");
		return 0;
	}

	i = times;
	while (i--) {
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		if (ret_value) {/* AUXADC is busy */
#if defined(APPLY_AUXADC_CALI_DATA)
			auxadc_cali_temp = valid_temp;
#else
			ret_temp = valid_temp;
#endif
		} else {
#if defined(APPLY_AUXADC_CALI_DATA)
		/*
		 * by reference mtk_auxadc.c
		 *
		 * convert to volt:
		 *      data[0] = (rawdata * 1500 / (4096 + cali_ge)) / 1000;
		 *
		 * convert to mv, need multiply 10:
		 *      data[1] = (rawdata * 150 / (4096 + cali_ge)) % 100;
		 *
		 * provide high precision mv:
		 *      data[2] = (rawdata * 1500 / (4096 + cali_ge)) % 1000;
		 */
			auxadc_cali_temp = data[0]*1000+data[2];
			valid_temp = auxadc_cali_temp;
#else
			valid_temp = ret_temp;
#endif
		}

#if defined(APPLY_AUXADC_CALI_DATA)
		ret += auxadc_cali_temp;
		mtkts_board_dprintk(
			"[thermal_auxadc_get_data(AUX_IN0_NTC)]: ret_temp=%d\n",
			auxadc_cali_temp);
#else
		ret += ret_temp;
		mtkts_board_dprintk(
			"[thermal_auxadc_get_data(AUX_IN0_NTC)]: ret_temp=%d\n",
			ret_temp);
#endif
	}

	/* Mt_auxadc_hal.c */
	/* #define VOLTAGE_FULL_RANGE  1500 // VA voltage */
	/* #define AUXADC_PRECISE      4096 // 12 bits */
#if defined(APPLY_AUXADC_CALI_DATA)
#else
	ret = ret * 1500 / 4096;
#endif

	/* ret = ret*1800/4096;//82's ADC power */
	mtkts_board_dprintk("APtery output mV = %d\n", ret);
	output = mtk_ts_board_volt_to_temp(ret);
	mtkts_board_dprintk("BOARD output temperature = %d\n", output);
	return output;
}

static DEFINE_MUTEX(BOARD_lock);
/*int ts_AP_at_boot_time = 0;*/
int mtkts_board_get_hw_temp(void)
{
	int t_ret = 0;

	mutex_lock(&BOARD_lock);

	/* get HW AP temp (TSAP) */
	/* cat /sys/class/power_supply/AP/AP_temp */
	t_ret = get_hw_board_temp();
	t_ret = t_ret * 1000;

	mutex_unlock(&BOARD_lock);

	board_cur_temp = t_ret;

	if (t_ret > 40000)	/* abnormal high temp */
		mtkts_board_printk("T_AP=%d\n", t_ret);

	mtkts_board_dprintk("[%s] T_AP, %d\n", __func__, t_ret);
	return t_ret;
}

static int mtkts_board_get_temp(struct thermal_zone_device *thermal, int *t)
{
	*t = mtkts_board_get_hw_temp();

	/* if ((int) *t > 52000) */
	/* mtkts_board_dprintk("T=%d\n", (int) *t); */

	if ((int)*t >= polling_trip_temp1)
		thermal->polling_delay = interval * 1000;
	else if ((int)*t < polling_trip_temp2)
		thermal->polling_delay = interval * polling_factor2;
	else
		thermal->polling_delay = interval * polling_factor1;

	return 0;
}

static int mtkts_board_get_mode(struct thermal_zone_device *thermal,
				enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED : THERMAL_DEVICE_DISABLED;
	return 0;
}

static int mtkts_board_set_mode(struct thermal_zone_device *thermal,
				enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int mtkts_board_get_trip_type(struct thermal_zone_device *thermal,
					int trip, enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int mtkts_board_get_trip_temp(struct thermal_zone_device *thermal,
					int trip, int *temp)
{
	*temp = trip_temp[trip];
	return 0;
}

static int mtkts_board_get_crit_temp(struct thermal_zone_device *thermal,
					int *temperature)
{
	*temperature = mtkts_board_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtkts_board_dev_ops = {
	.get_temp = mtkts_board_get_temp,
	.get_mode = mtkts_board_get_mode,
	.set_mode = mtkts_board_set_mode,
	.get_trip_type = mtkts_board_get_trip_type,
	.get_trip_temp = mtkts_board_get_trip_temp,
	.get_crit_temp = mtkts_board_get_crit_temp,
};

void mtkts_board_prepare_table(int table_num)
{

	switch (table_num) {
	case 1:		/* AP_NTC_BL197 */
		BOARD_Temperature_Table = BOARD_Temperature_Table1;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table1);
		break;
	case 2:		/* AP_NTC_TSM_1 */
		BOARD_Temperature_Table = BOARD_Temperature_Table2;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table2);
		break;
	case 3:		/* AP_NTC_10_SEN_1 */
		BOARD_Temperature_Table = BOARD_Temperature_Table3;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table3);
		break;
	case 4:		/* AP_NTC_10 */
		BOARD_Temperature_Table = BOARD_Temperature_Table4;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table4);
		break;
	case 5:		/* AP_NTC_47 */
		BOARD_Temperature_Table = BOARD_Temperature_Table5;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table5);
		break;
	case 6:		/* NTCG104EF104F */
		BOARD_Temperature_Table = BOARD_Temperature_Table6;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table6);
		break;
	case 7:		/* NCP15WF104F03RC */
		BOARD_Temperature_Table = BOARD_Temperature_Table7;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table7);
		break;
	default:		/* AP_NTC_10 */
		BOARD_Temperature_Table = BOARD_Temperature_Table4;
		ntc_tbl_size = sizeof(BOARD_Temperature_Table4);
		break;
	}

	pr_notice("[Thermal/TZ/BOARD] %s table_num=%d\n", __func__, table_num);
}

static int mtkts_board_register_thermal(void)
{
	mtkts_board_dprintk("[%s]\n", __func__);

	/* trips : trip 0~1 */
	thz_dev = mtk_thermal_zone_device_register("mtktsboard", num_trip, NULL,
						&mtkts_board_dev_ops, 0, 0, 0,
						interval * 1000);

	return 0;
}

static void mtkts_board_unregister_thermal(void)
{
	mtkts_board_dprintk("[%s]\n", __func__);

	if (thz_dev) {
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}

static int __init mtkts_board_init(void)
{
	/* setup default table */
	mtkts_board_prepare_table(g_RAP_ntc_table);

	mtkts_board_register_thermal();
	return 0;
}

static void __exit mtkts_board_exit(void)
{
	mtkts_board_dprintk("[%s]\n", __func__);
	mtkts_board_unregister_thermal();
}

module_init(mtkts_board_init);
module_exit(mtkts_board_exit);
