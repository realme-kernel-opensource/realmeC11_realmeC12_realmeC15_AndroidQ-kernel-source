/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __IMGSENSOR_COMMON_H__
#define __IMGSENSOR_COMMON_H__

#include "kd_camera_feature.h"
#include "kd_imgsensor_define.h"

/*Henry.Chang@Cam.Drv add for 19551 20191010*/
#ifndef VENDOR_EDIT
#define VENDOR_EDIT
#endif

#ifdef VENDOR_EDIT
/*Henry.Chang@Cam.Drv add for 19551 20191010*/
#include <soc/oppo/oppo_project.h>
#endif
/******************************************************************************
 * Debug configuration
 ******************************************************************************/
#define PREFIX "[imgsensor]"
#define PLATFORM_POWER_SEQ_NAME "platform_power_seq"
#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG(fmt, arg...)  pr_debug(PREFIX fmt, ##arg)
#define PK_PR_ERR(fmt, arg...)  pr_err(fmt, ##arg)
#define PK_INFO(fmt, arg...) pr_debug(PREFIX fmt, ##arg)
#else
#define PK_DBG(fmt, arg...)
#define PK_PR_ERR(fmt, arg...)  pr_err(fmt, ##arg)
#define PK_INFO(fmt, arg...) pr_debug(PREFIX fmt, ##arg)
#endif

#define IMGSENSOR_LEGACY_COMPAT

#define IMGSENSOR_TOSTRING(value)           #value
#define IMGSENSOR_STRINGIZE(stringizedName) IMGSENSOR_TOSTRING(stringizedName)

enum IMGSENSOR_ARCH {
	IMGSENSOR_ARCH_V1 = 0,
	IMGSENSOR_ARCH_V2,
	IMGSENSOR_ARCH_V3
};

enum IMGSENSOR_RETURN {
	IMGSENSOR_RETURN_SUCCESS = 0,
	IMGSENSOR_RETURN_ERROR   = -1,
};

#ifdef VENDOR_EDIT
/*Henry.Chang@Cam.Drv add for 19551 20191010*/
#define CAMERA_MODULE_INFO_LENGTH (8)
#define CAMERA_MODULE_SN_LENGTH    (20)
/*Henry.Chang@Cam.Drv add for 19551 20191107*/
#define AESYNC_DATA_LENGTH_TOTAL          (65)
#define DUALCAM_CALI_DATA_LENGTH_8ALIGN   (1568)
#define DUALCAM_CALI_DATA_LENGTH          (1561)
#define S5KGM1_STEREO_START_ADDR          (0x1640)
#define IMX586_STEREO_START_ADDR          (0x24D0)
#define IMX586_STEREO_START_ADDR_WIDE     (0x2A00)
#define IMX586_STEREO_START_ADDR_TELE     (0x3040)
#define IMX586_AESYNC_START_ADDR          (0x24C0)
#define GC5035_STEREO_START_ADDR          (0x1600)
//#define HI846_STEREO_START_ADDR          (0x1A20)
#define IMX398_STEREO_START_ADDR          (0x1640)
#define GC2375H_STEREO_START_ADDR         (0x1020)
#define S5KGW1_STEREO_START_ADDR_WIDE     (0x1900)
#define S5KGW1_STEREO_START_ADDR_TELE     (0x1F20)
#define S5KGW1_AESYNC_START_ADDR          (0x2540)
#define S5K3M5_STEREO_START_ADDR          (0x2600)
#define S5K3M5_AESYNC_START_ADDR          (0x2D00)
#define S5K3M5SX_STEREO_START_ADDR        (0xD000)
#define S5K3M5SX_AESYNC_START_ADDR        (0xD000)
#define GC8054_STEREO_START_ADDR          (0x2180)
#define GC8054F_STEREO_START_ADDR         (0x1A00)
#define GC8054_AESYNC_START_ADDR          (0x0C90)
#define HI846_STEREO_START_ADDR           (0x1E7D)
#define HI846_AESYNC_START_ADDR           (0x0C90)
#define S5KGH1_STEREO_START_ADDR          (0x1360)
#define GC02M0F_STEREO_START_ADDR         (0x1408)

#define DUALCAM_CALI_DATA_LENGTH_TOTAL_TELE    (2450)
#define DUALCAM_CALI_DATA_LENGTH_TELE          (909)

extern char gOtpCheckdata[7][40];
#endif
#define LENGTH_FOR_SNPRINTF 256
#endif

