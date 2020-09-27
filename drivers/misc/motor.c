/************************************************************************************
** Copyright (C), 2008-2017, OPPO Mobile Comm Corp., Ltd
** VENDOR_EDIT
** File: motor.c
**
** Description:
**	Definitions for oppo_motor common software.
**
** Version: 1.0
** Date created: 2018/01/14,20:27
** Author: Fei.Mo@PSW.BSP.Sensor
**
** --------------------------- Revision History: ------------------------------------
* <version>		<date>		<author>		<desc>
* Revision 1.0		2018/01/14	Fei.Mo@PSW.BSP.Sensor	Created
**************************************************************************************/
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/serio.h>
#include <soc/oppo/boot_mode.h>
#include <soc/oppo/oppo_project.h>
#include "oppo_motor/oppo_motor.h"
#include "oppo_motor/oppo_motor_notifier.h"
#include "oppo_motor/digital_hall_ic/oppo_m1120.h"
//#include <mt-plat/mtk_boot_common.h>
/*if can not compile success, please update vendor/oppo_motor*/

void oppo_parse_motor_info(struct oppo_motor_chip * chip)
{

	if (!chip)
		return;
	MOTOR_ERR("customize %s;%d\n",__func__,__LINE__);

	chip->info.type = MOTOR_FI5;
	chip->info.motor_ic = STSPIN220;
	chip->dir_sign = NEGATIVE;
	chip->is_support_ffd = false;
	MOTOR_LOG("boot_mode is %d \n",get_boot_mode());
	if ((RECOVERY_BOOT == get_boot_mode()) || (FACTORY_BOOT == get_boot_mode()) ||
		oppo_is_power_off_charging() || (OPPO_SAU_BOOT == get_boot_mode()))
		chip->boot_mode = OTHER_MODE;
	else
		chip->boot_mode = NORMAL_MODE;
}
EXPORT_SYMBOL(oppo_parse_motor_info);

