/*
 * Copyright (C) 2018 MediaTek Inc.
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
#ifndef __TMP_BOARD_H__
#define __TMP_BOARD_H__

#define APPLY_PRECISE_NTC_TABLE
#define APPLY_AUXADC_CALI_DATA

#define AUX_IN0_NTC (0)
#define AUX_IN1_NTC (1)
#define AUX_IN4_NTC (4)

#define BOARD_RAP_PULL_UP_R		390000 /* 390K, pull up resister */

#define BOARD_TAP_OVER_CRITICAL_LOW	4397119 /* base on 100K NTC temp
						 * default value -40 deg
						 */

#define BOARD_RAP_PULL_UP_VOLTAGE		1800 /* 1.8V ,pull up voltage */

#define BOARD_RAP_NTC_TABLE		7 /* default is NCP15WF104F03RC(100K) */

#define BOARD_RAP_ADC_CHANNEL		AUX_IN4_NTC /* default is 4 */

extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
extern int IMM_IsAdcInitReady(void);

#endif	/* __TMP_BOARD_H__ */
