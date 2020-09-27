/************************************************************************************
** File: - android\kernel\arch\arm\mach-msm\include\mach\oppo_boot.h
** VENDOR_EDIT
** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
**
** Description:
**     change define of boot_mode here for other place to use it
** Version: 1.0
** --------------------------- Revision History: --------------------------------
** 	           <author>	           <data>			    <desc>
** kun.wangkun@BasicDrv.TP&LCD      06/01/2019          add this file
************************************************************************************/
#ifndef _OPPO_BOOT_H
#define _OPPO_BOOT_H
#include <soc/oppo/boot_mode_types.h>
enum{
	    NORMAL_BOOT = 0,
	    META_BOOT = 1,
	    RECOVERY_BOOT = 2,
	    SW_REBOOT = 3,
	    FACTORY_BOOT = 4,
	    ADVMETA_BOOT = 5,
	    ATE_FACTORY_BOOT = 6,
	    ALARM_BOOT = 7,
	    KERNEL_POWER_OFF_CHARGING_BOOT = 8,
	    LOW_POWER_OFF_CHARGING_BOOT = 9,
	    DONGLE_BOOT = 10,
#ifdef VENDOR_EDIT
/* Bin.Li@EXP.BSP.bootloader.bootflow, 2017/05/24, Add for oppo boot mode */
	    OPPO_SAU_BOOT = 11,
	    SILENCE_BOOT = 12,
	    SAFE_BOOT = 999,
#endif /* VENDOR_EDIT */
};

//#ifdef VENDOR_EDIT
//Ke.Li@ROM.Security, KernelEvent, 2019-9-30, remove the "static" flag for kernel kevent
//extern static int get_boot_mode(void);
extern int get_boot_mode(void);
//#endif /* VENDOR_EDIT */
#ifdef VENDOR_EDIT
//Fuchun.Liao@Mobile.BSP.CHG 2016-01-14 add for charge
extern bool qpnp_is_power_off_charging(void);
#endif
#ifdef VENDOR_EDIT
/*Fuchun.Liao@Mobile.BSP.CHG 2016-01-14 add for charge*/
extern bool oppo_is_power_off_charging(void);
#endif
#endif
