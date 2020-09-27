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

/*
 * SEM1215SAF voice coil motor driver
 * SEM1215S : OIS driver
 * SZ3720AF   : VCM driver be the same as AK7371AF
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>

/* kernel standard for PMIC*/
#if !defined(CONFIG_MTK_LEGACY)
#include <linux/regulator/consumer.h>
#endif
#include "lens_info.h"

#define AF_DRVNAME "SEM1215SAF_DRV"
#define AF_I2C_SLAVE_ADDR 0x68
#define EEPROM_I2C_SLAVE_ADDR 0x68

#define SZ3720AF_I2C_SLAVE_ADDR 0x68

#define AF_DEBUG
#ifdef AF_DEBUG
#define LOG_INF(format, args...)                                               \
	pr_info(AF_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

static struct i2c_client *g_pstAF_I2Cclient;
static int *g_pAF_Opened;
static spinlock_t *g_pAF_SpinLock;

static unsigned long g_u4AF_INF;
static unsigned long g_u4AF_MACRO = 1023;
static unsigned long g_u4CurrPosition;
static struct stAF_OisPosInfo gOisPosInfoObj;
/* PMIC */
#if !defined(CONFIG_MTK_LEGACY)
static struct regulator *regVCAMAF;
static struct device *lens_device;
#endif

static int s4SZ3720AF_WriteReg_8(unsigned short a_u2Addr, unsigned short a_u2Data)
{
	int i4RetValue = 0;

	char puSendCmd[3] = {(char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF),
	                     (char)(a_u2Data & 0xFF)};

	g_pstAF_I2Cclient->addr = SZ3720AF_I2C_SLAVE_ADDR;

	g_pstAF_I2Cclient->addr = g_pstAF_I2Cclient->addr >> 1;

	i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 3);

	if (i4RetValue < 0) {
		LOG_INF("I2C write failed!!\n");
		return -1;
	}

	return 0;
}

static char s4SZ3720AF_ReadReg(unsigned short a_uAddr)
{
    char a_uData = -1;
	char puSendCmd[2] = {(char)(a_uAddr >> 8), (char)(a_uAddr & 0xFF)};

	g_pstAF_I2Cclient->addr = (AF_I2C_SLAVE_ADDR) >> 1;

	if (i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 2) < 0) {
		LOG_INF("ReadI2C send failed!!\n");
		return -1;
	}

	if (i2c_master_recv(g_pstAF_I2Cclient, &a_uData, 1) < 0) {
		LOG_INF("ReadI2C recv failed!!\n");
		return -1;
	}

	LOG_INF("RDI2C addr[0x%x]: 0x%x\n", a_uAddr, a_uData);

	return a_uData;
}


static inline int setSZ3720AFPos(unsigned long a_u4Position)
{
	int i4RetValue = 0;
	unsigned short i4PosH = (unsigned short) ((a_u4Position >> 8) & 0xff);
    unsigned short i4PosL = (unsigned short) (a_u4Position & 0xff);
	i4RetValue = s4SZ3720AF_WriteReg_8(0x0204, i4PosL);
	i4RetValue = s4SZ3720AF_WriteReg_8(0x0205, i4PosH);
	LOG_INF("setSZ3720AFPos readback: %x %x\n", s4SZ3720AF_ReadReg(0x0204), s4SZ3720AF_ReadReg(0x0205));
	if (i4RetValue < 0) {
		LOG_INF("setSZ3720AFPos setDac: %d failed!!\n", a_u4Position);
		return -1;
	}

	return i4RetValue;
}

static inline int getAFInfo(__user struct stAF_MotorInfo *pstMotorInfo)
{
	struct stAF_MotorInfo stMotorInfo;

	stMotorInfo.u4MacroPosition = g_u4AF_MACRO;
	stMotorInfo.u4InfPosition = g_u4AF_INF;
	stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	stMotorInfo.bIsSupportSR = 1;

	stMotorInfo.bIsMotorMoving = 1;

	if (*g_pAF_Opened >= 1)
		stMotorInfo.bIsMotorOpen = 1;
	else
		stMotorInfo.bIsMotorOpen = 0;

	if (copy_to_user(pstMotorInfo, &stMotorInfo,
			 sizeof(struct stAF_MotorInfo)))
		LOG_INF("copy to user failed when getting motor information\n");

	return 0;
}

/* initAF include driver initialization and standby mode */
static int initAF(void)
{
    int i4DelayCnt = 60;
	char i4AFSts = -1, i4OISSts = -1;
	LOG_INF("+\n");
	if (*g_pAF_Opened == 1) {
        i4AFSts = s4SZ3720AF_ReadReg(0x0201);
        i4OISSts = s4SZ3720AF_ReadReg(0x0001);
        while ((i4DelayCnt > 1) && !(i4AFSts == 0x1)) {
            i4AFSts = s4SZ3720AF_ReadReg(0x0201); 
		    LOG_INF("i4AFSts:%x, i4DelayCnt:%d\n", i4AFSts, i4DelayCnt);
			msleep(5);
            i4DelayCnt --;
		}
        i4OISSts = s4SZ3720AF_ReadReg(0x0001);
		LOG_INF("step1 i4AFSts:%x, i4OISSts:%x\n", i4AFSts, i4OISSts);
		/* Start Lens Control 0x0200: 1: AF_ON 0:AF_OFF */
		s4SZ3720AF_WriteReg_8(0x0200, 0x01);
		s4SZ3720AF_WriteReg_8(0x0612, 0x01);
		s4SZ3720AF_WriteReg_8(0x0600, 0x01);
		LOG_INF("step2 i4AFSts:%x, i4OISSts:%x\n", i4AFSts, i4OISSts);
		LOG_INF("stepn 0600:%x, 0612:%x\n", s4SZ3720AF_ReadReg(0x0600), s4SZ3720AF_ReadReg(0x0612));
        i4DelayCnt = 60;
        while ((i4DelayCnt > 1) && !(i4OISSts & 0x1)) {
            i4OISSts = s4SZ3720AF_ReadReg(0x0001);
		    LOG_INF("i4OISSts:%x, i4DelayCnt:%d\n", i4OISSts, i4DelayCnt);
			msleep(5);
            i4DelayCnt --;
		}
        /* Set OIS mode£º0-still 1-video 2-zoom 3-Center 11-Fixed 12-Sine 13-Square */
		s4SZ3720AF_WriteReg_8(0x0002, 0);
		LOG_INF("step3 i4AFSts:%x, i4OISSts:%x\n", i4AFSts, i4OISSts);
        /* Start OIS control£º0x0000  1:OIS_ON 0:OIS_OFF */
		s4SZ3720AF_WriteReg_8(0x0000, 0x01);
        i4AFSts = s4SZ3720AF_ReadReg(0x0200);
        i4OISSts = s4SZ3720AF_ReadReg(0x0000);
		LOG_INF("step4 i4AFSts:%x, i4OISSts:%x\n", i4AFSts, i4OISSts);

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 2;
		spin_unlock(g_pAF_SpinLock);
	}

	LOG_INF("-\n");

	return 0;
}

/* moveAF only use to control moving the motor */
static inline int moveAF(unsigned long a_u4Position)
{
	int ret = 0;

	if (setSZ3720AFPos(a_u4Position) == 0) {
		g_u4CurrPosition = a_u4Position;
		ret = 0;
	} else {
		LOG_INF("set I2C failed when moving the motor\n");
		ret = -1;
	}

	return ret;
}

static inline int setAFInf(unsigned long a_u4Position)
{
	spin_lock(g_pAF_SpinLock);
	g_u4AF_INF = a_u4Position;
	spin_unlock(g_pAF_SpinLock);
	return 0;
}

static inline int setAFMacro(unsigned long a_u4Position)
{
	spin_lock(g_pAF_SpinLock);
	g_u4AF_MACRO = a_u4Position;
	spin_unlock(g_pAF_SpinLock);
	return 0;
}

int SEM1215SAF_OisGetHallPos(int *PosX, int *PosY)
{
	if (*g_pAF_Opened == 2) {
		*PosX = 0;
		*PosY = 0;
	}

	return 0;
}

static inline int64_t getCurNS(void)
{
	int64_t ns;
	struct timespec time;

	time.tv_sec = time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	ns = time.tv_sec * 1000000000LL + time.tv_nsec;

	return ns;
}

#define OIS_HALLDATA_NUM       (8)
#define OIS_HALLDATA_ADDR_BASE (0x1100)

int SEM1215SAF_OisGetHallInfo(void *a_pOisPosInfo)
{
	struct stAF_OisPosInfo *pOisPosInfo;
	char readL = -1, readH = -1;
	int ois_pos_x[OIS_HALLDATA_NUM];
	int ois_pos_y[OIS_HALLDATA_NUM];
	int64_t ois_pos_ts[OIS_HALLDATA_NUM];
	int64_t timestamp;
	int i;

	pOisPosInfo = (struct stAF_OisPosInfo *)a_pOisPosInfo;

	timestamp = getCurNS();
	/* LOG_INF("read 0x8A, data transfering timing : %lld ns\n", timestamp); */

	for (i = 0; i < OIS_HALLDATA_NUM; i++)
	{
        readL = s4SZ3720AF_ReadReg(4*i + OIS_HALLDATA_ADDR_BASE);
        readH = s4SZ3720AF_ReadReg(4*i + 1 + OIS_HALLDATA_ADDR_BASE);
		ois_pos_x[i] = (int)((readH << 8) | readL);
        readL = s4SZ3720AF_ReadReg(4*i + 2 + OIS_HALLDATA_ADDR_BASE);
        readH = s4SZ3720AF_ReadReg(4*i + 3 + OIS_HALLDATA_ADDR_BASE);
		ois_pos_y[i] = (int)((readH << 8) | readL);
		ois_pos_ts[i] = timestamp - 4000000 * (i - 1);
		pOisPosInfo->i4OISHallPosX[i] = ois_pos_x[i];
		pOisPosInfo->i4OISHallPosY[i] = ois_pos_y[i];
		pOisPosInfo->TimeStamp[i]     = ois_pos_ts[i];
		LOG_INF("i4OISHallPosX: %d PosY:%d \n", ois_pos_x[i], ois_pos_x[i]);
	}

	LOG_INF("X axis Gyro Output: %x%x\n", s4SZ3720AF_ReadReg(0x0B05), s4SZ3720AF_ReadReg(0x0B04));
	LOG_INF("X axis Gyro Output: %x%x\n", s4SZ3720AF_ReadReg(0x0B07), s4SZ3720AF_ReadReg(0x0B06));

	return 0;
}

static inline int setAFPara(__user struct stAF_MotorCmd *pstMotorCmd)
{
	int retvalue = 0;
	struct stAF_MotorCmd stMotorCmd;
	struct stAF_OisPosInfo *pOisPosInfo = &gOisPosInfoObj;

	if (copy_from_user(&stMotorCmd, pstMotorCmd, sizeof(stMotorCmd)))
		LOG_INF("copy to user failed when getting motor command\n");

	LOG_INF("Motor CmdID : %x %x\n", stMotorCmd.u4CmdID, stMotorCmd.u4Param);

	retvalue = SEM1215SAF_OisGetHallInfo((void *)pOisPosInfo);
	LOG_INF("OisGetHallInfo ret:%d,g_u4CurrPosition :%d\n", retvalue, g_u4CurrPosition);

	LOG_INF("0x0204-07:[%x %x %x %x]\n", s4SZ3720AF_ReadReg(0x0204)
				, s4SZ3720AF_ReadReg(0x0205)
				, s4SZ3720AF_ReadReg(0x0206)
				, s4SZ3720AF_ReadReg(0x0207));

	switch (stMotorCmd.u4CmdID) {
	#if 0
	case 1:
		/* oisenable: 1:on 0:off*/
		if (*g_pAF_Opened == 2 && stMotorCmd.u4Param >= 0) {
            unsigned short OisEnable = stMotorCmd.u4Param & 0xFF;

            if (!s4SZ3720AF_WriteReg_8(0x0000, !(!OisEnable))) {
				LOG_INF("setoismode[%d] scuess!\n", OisEnable);
			} else {
				LOG_INF("setoismode[%d] failed!\n", OisEnable);
			}

		}
		break;
	case 2:
		/* setoismode:0-still mode  1-video mode  2-Zoom mode  3-Centering mode*/
		if (*g_pAF_Opened == 2 && stMotorCmd.u4Param >= 0) {
            unsigned short OisMode = stMotorCmd.u4Param & 0xFF;

            if (!s4SZ3720AF_WriteReg_8(0x0002, OisMode)) {
				LOG_INF("setoismode[%d] scuess!\n", OisMode);
			} else {
				LOG_INF("setoismode[%d] failed!\n", OisMode);
			}

		}
		break;
	case 3:
		if (*g_pAF_Opened == 2 && stMotorCmd.u4Param > 0) {
			unsigned short PosX, PosY;

			PosX = stMotorCmd.u4Param / 10000;
			PosY = stMotorCmd.u4Param - PosX * 10000;

			LOG_INF("Targe (%d , %d)\n", PosX, PosY);
		}
		break;
	#endif
	default:
		break;
	}

	return 0;
}

/* ////////////////////////////////////////////////////////////// */
long SEM1215SAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command,
		     unsigned long a_u4Param)
{
	long i4RetValue = 0;

	switch (a_u4Command) {
	case AFIOC_G_MOTORINFO:
		i4RetValue =
			getAFInfo((__user struct stAF_MotorInfo *)(a_u4Param));
		break;

	case AFIOC_T_MOVETO:
		i4RetValue = moveAF(a_u4Param);
		break;

	case AFIOC_T_SETINFPOS:
		i4RetValue = setAFInf(a_u4Param);
		break;

	case AFIOC_T_SETMACROPOS:
		i4RetValue = setAFMacro(a_u4Param);
		break;

	case AFIOC_S_SETPARA:
		i4RetValue =
			setAFPara((__user struct stAF_MotorCmd *)(a_u4Param));
		break;

          default:
		LOG_INF("No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	return i4RetValue;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
int SEM1215SAF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	LOG_INF("Start\n");

	if (*g_pAF_Opened == 2) {
		LOG_INF("Wait\n");
		//OIS_Standby();
		msleep(20);
	}

	if (*g_pAF_Opened) {
		LOG_INF("Free\n");

		spin_lock(g_pAF_SpinLock);
		*g_pAF_Opened = 0;
		spin_unlock(g_pAF_SpinLock);

	}
	/*Stop Lens Control*/
    s4SZ3720AF_WriteReg_8(0x0200, 0x00);
    /* OFF OIS */
    s4SZ3720AF_WriteReg_8(0x0000, 0x00);
	LOG_INF("End\n");

	return 0;
}

int SEM1215SAF_PowerDown(struct i2c_client *pstAF_I2Cclient,
			int *pAF_Opened)
{
	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_Opened = pAF_Opened;

	LOG_INF("+\n");
	if (*g_pAF_Opened == 0) {
		char data = -1;
		int cnt = 0;

		while (1) {
			data = -1;

			s4SZ3720AF_WriteReg_8(0x0000, 0x0);
			s4SZ3720AF_WriteReg_8(0x0200, 0x0);

			data = s4SZ3720AF_ReadReg(0x0000);

			LOG_INF("Addr : 0x0000 , Data : %x\n", data);

			//IS_Standby();

			if (data == 0x00 || cnt == 1)
				break;

			cnt++;
		}
	}
	LOG_INF("-\n");

	return 0;
}

int SEM1215SAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient,
			   spinlock_t *pAF_SpinLock, int *pAF_Opened)
{
	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_pAF_SpinLock = pAF_SpinLock;
	g_pAF_Opened = pAF_Opened;
#if !defined(CONFIG_MTK_LEGACY)
	regVCAMAF = NULL;
	lens_device = NULL;
#endif

	LOG_INF("SetI2Cclient\n");

	initAF();

	return 1;
}

int SEM1215SAF_GetFileName(unsigned char *pFileName)
{
	#if SUPPORT_GETTING_LENS_FOLDER_NAME
	char FilePath[256];
	char *FileString;

	sprintf(FilePath, "%s", __FILE__);
	FileString = strrchr(FilePath, '/');
	*FileString = '\0';
	FileString = (strrchr(FilePath, '/') + 1);
	strncpy(pFileName, FileString, AF_MOTOR_NAME);
	LOG_INF("FileName : %s\n", pFileName);
	#else
	pFileName[0] = '\0';
	#endif
	return 1;
}
