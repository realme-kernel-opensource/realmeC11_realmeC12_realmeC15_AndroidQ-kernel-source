/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef _LENS_LIST_H

#define _LENS_LIST_H

#if 0
#define AK7371AF_SetI2Cclient AK7371AF_SetI2Cclient_Sub2
#define AK7371AF_Ioctl AK7371AF_Ioctl_Sub2
#define AK7371AF_Release AK7371AF_Release_Sub2
#define AK7371AF_PowerDown AK7371AF_PowerDown_Sub2
#define AK7371AF_GetFileName AK7371AF_GetFileName_Sub2
extern int AK7371AF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long AK7371AF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int AK7371AF_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int AK7371AF_PowerDown(struct i2c_client *pstAF_I2Cclient,
				int *pAF_Opened);
extern int AK7371AF_GetFileName(unsigned char *pFileName);
#endif
#ifdef VENDOR_EDIT
/*Henry.Chang@Cam.Drv add for 19551 20191006*/
#define SEM1215SAF_SetI2Cclient SEM1215SAF_SetI2Cclient_Sub2
#define SEM1215SAF_Ioctl SEM1215SAF_Ioctl_Sub2
#define SEM1215SAF_Release SEM1215SAF_Release_Sub2
#define SEM1215SAF_PowerDown SEM1215SAF_PowerDown_Sub2
#define SEM1215SAF_GetFileName SEM1215SAF_GetFileName_Sub2
#define SEM1215SAF_OisGetHallPos SEM1215SAF_OisGetHallPos_Sub2
#define SEM1215SAF_OisGetHallInfo SEM1215SAF_OisGetHallInfo_Sub2
extern int SEM1215SAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long SEM1215SAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int SEM1215SAF_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int SEM1215SAF_PowerDown(struct i2c_client *pstAF_I2Cclient,
				int *pAF_Opened);
extern int SEM1215SAF_GetFileName(unsigned char *pFileName);
extern int SEM1215SAF_OisGetHallPos(int *PosX, int *PosY);
extern int SEM1215SAF_OisGetHallInfo(void *a_pOisPosInfo);

/*Henry.Chang@Camera.Drv add for mainaf 20191014*/
#define BU64253TEAF_SetI2Cclient BU64253TEAF_SetI2Cclient_Sub2
#define BBU64253TEAF_Ioctl BU64253TEAF_Ioctl_Sub2
#define BBU64253TEAF_Release BU64253TEAF_Release_Sub2
#define BU64253TEAF_GetFileName BU64253TEAF_GetFileName_Sub2
extern int BU64253TEAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
				 spinlock_t *pAF_SpinLock, int *pAF_Opened);
extern long BU64253TEAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
			   unsigned long a_u4Param);
extern int BU64253TEAF_Release(struct inode *a_pstInode, struct file *a_pstFile);
extern int BU64253TEAF_GetFileName(unsigned char *pFileName);
#endif
extern void AFRegulatorCtrl(int Stage);
#endif
