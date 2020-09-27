/*****************************************************************************
 *
 * Filename:
 * ---------
 *	 S5K3M5SXmipi_Sensor.c
 *
 * Project:
 * --------
 *	 ALPS
 *
 * Description:
 * ------------
 *	 Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "s5k3m5sxmipiraw_Sensor.h"
#include "imgsensor_common.h"

#ifndef VENDOR_EDIT
#define VENDOR_EDIT
#endif

#ifdef VENDOR_EDIT
/*Caohua.Lin@Camera.Driver add for 18011/18311  board 20180723*/
#define DEVICE_VERSION_S5K3M5SX     "s5k3msx"
extern void register_imgsensor_deviceinfo(char *name, char *version, u8 module_id);
static kal_uint8 deviceInfo_register_value = 0x00;
#endif
static kal_uint32 streaming_control(kal_bool enable);
#define MODULE_ID_OFFSET 0x0000
#define PFX "s5k3msx_camera_sensor"
#define LOG_INF(format,  args...)	pr_debug(PFX "[%s] " format,  __FUNCTION__,  ##args)

extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length, u16 timing);


static DEFINE_SPINLOCK(imgsensor_drv_lock);


static struct imgsensor_info_struct imgsensor_info = {
		.sensor_id = S5K3M5SX_SENSOR_ID,
		.module_id = 0x04,	//0x01 Sunny,0x05 QTEK
		.checksum_value = 0xffb1ec31,
		.pre = {
			.pclk = 482000000,
			.linelength = 4848,
			.framelength = 3312,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 4000,
			.grabwindow_height = 3000,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 576000000,
			.max_framerate = 300,
		},
		.cap = {
			.pclk = 482000000,
			.linelength = 4848,
			.framelength = 3312,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 4000,
			.grabwindow_height = 3000,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 576000000,
			.max_framerate = 300,
		},
		.normal_video = {
			.pclk = 482000000,
			.linelength = 4848,
			.framelength = 3312,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 4000,
			.grabwindow_height = 3000,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 576000000,
			.max_framerate = 300,
		},
		.hs_video = {
			.pclk = 482000000,
			.linelength = 4848,
			.framelength = 3312,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 4000,
			.grabwindow_height = 3000,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 576000000,
			.max_framerate = 300,
		},
		.slim_video = {
			.pclk = 482000000,
			.linelength = 4848,
			.framelength = 3312,
			.startx = 0,
			.starty = 0,
			.grabwindow_width = 4208,
			.grabwindow_height = 3120,
			.mipi_data_lp2hs_settle_dc = 85,
			.mipi_pixel_rate = 576000000,
			.max_framerate = 300,
		},

		.margin = 2,
		.min_shutter = 3,
		.max_frame_length = 0xffff-5,
		.ae_shut_delay_frame = 0,
		.ae_sensor_gain_delay_frame = 0,
		.ae_ispGain_delay_frame = 2,
		.ihdr_support = 0,   /*1, support; 0,not support*/
		.ihdr_le_firstline = 0,  /*1,le first; 0, se first*/
		.sensor_mode_num = 5,    /*support sensor mode num*/

		.cap_delay_frame = 2,  /*3 guanjd modify for cts*/
		.pre_delay_frame = 2,  /*3 guanjd modify for cts*/
		.video_delay_frame = 3,
		.hs_video_delay_frame = 3,
		.slim_video_delay_frame = 3,

		.isp_driving_current = ISP_DRIVING_2MA,
		.sensor_interface_type = SENSOR_INTERFACE_TYPE_MIPI,
		.mipi_sensor_type = MIPI_OPHY_NCSI2,  /*0,MIPI_OPHY_NCSI2;  1,MIPI_OPHY_CSI2*/
		.mipi_settle_delay_mode = 1,  /*0,MIPI_SETTLEDELAY_AUTO; 1,MIPI_SETTLEDELAY_MANNUAL*/
		.sensor_output_dataformat = SENSOR_OUTPUT_FORMAT_RAW_B,//s5k3m5sx
		.mclk = 24,
		.mipi_lane_num = SENSOR_MIPI_4_LANE,
		.i2c_addr_table = {0x5a, 0x20, 0xff},
		.i2c_speed = 400,
};


static struct imgsensor_struct imgsensor = {
	.mirror = IMAGE_V_MIRROR,				//mirrorflip information
	.sensor_mode = IMGSENSOR_MODE_INIT, /*IMGSENSOR_MODE enum value,record current sensor mode,such as: INIT, Preview, Capture, Video,High Speed Video, Slim Video*/
	.shutter = 0x3D0,					/*current shutter*/
	.gain = 0x100,						/*current gain*/
	.dummy_pixel = 0,					/*current dummypixel*/
	.dummy_line = 0,					/*current dummyline*/
	.current_fps = 0,  /*full size current fps : 24fps for PIP, 30fps for Normal or ZSD*/
	.autoflicker_en = KAL_FALSE,  /*auto flicker enable: KAL_FALSE for disable auto flicker, KAL_TRUE for enable auto flicker*/
	.test_pattern = KAL_FALSE,		/*test pattern mode or not. KAL_FALSE for in test pattern mode, KAL_TRUE for normal output*/
	.enable_secure = KAL_FALSE,
	.current_scenario_id = MSDK_SCENARIO_ID_CAMERA_PREVIEW,/*current scenario id*/
	.ihdr_mode = 0, /*sensor need support LE, SE with HDR feature*/
	.i2c_write_id = 0x20,

};


/* Sensor output window information */
static struct SENSOR_WINSIZE_INFO_STRUCT imgsensor_winsize_info[5] =
{
	{4208, 3120,  104,   60, 4000, 3000, 4000, 3000, 0, 0, 4000, 3000, 0, 0, 4000, 3000}, /*Preview*/
	{4208, 3120,  104,   60, 4000, 3000, 4000, 3000, 0, 0, 4000, 3000, 0, 0, 4000, 3000}, // capture remosic
	{4208, 3120,  104,   60, 4000, 3000, 4000, 3000, 0, 0, 4000, 3000, 0, 0, 4000, 3000}, /*video*/
	{4208, 3120,  104,   60, 4000, 3000, 4000, 3000, 0, 0, 4000, 3000, 0, 0, 4000, 3000}, /*hs_video,don't use*/
	{4208, 3120,    0,    0, 4208, 3120, 4208, 3120, 0, 0, 4208, 3120, 0, 0, 4208, 3120}, /* slim video*/
}; /*cpy from preview*/

 /*VC1 for HDR(DT=0X35), VC2 for PDAF(DT=0X36), unit : 10bit */
static struct SENSOR_VC_INFO_STRUCT SENSOR_VC_INFO[2] = {
	/* Preview mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x0FA0, 0x0BB8, 0x00, 0x00, 0x00, 0x00,
	 0x01, 0x30, 0x026C, 0x02E0, 0x00, 0x00, 0x0000, 0x0000},
	/* FullSize mode setting */
	{0x03, 0x0a, 0x00, 0x08, 0x40, 0x00,
	 0x00, 0x2b, 0x1F40, 0x1770, 0x00, 0x00, 0x00, 0x00,
	 0x01, 0x30, 0x028A, 0x0300, 0x00, 0x00, 0x0000, 0x0000}
};

/* If mirror flip */
static struct SET_PD_BLOCK_INFO_T imgsensor_pd_info =
{
	.i4OffsetX =  24,
	.i4OffsetY =  24,
	.i4PitchX  =  32,
	.i4PitchY  =  32,
	.i4PairNum  = 16,
	.i4SubBlkW  = 8,
	.i4SubBlkH  = 8,
	.i4PosL = {{26,25},{34,25},{42,25},{50,25},{30,37},{38,37},{46,37},{54,37},
		{26,45},{34,45},{42,45},{50,45},{30,49},{38,49},{46,49},{54,49}},
	.i4PosR = {{26,29},{34,29},{42,29},{50,29},{30,33},{38,33},{46,33},{54,33},
		{26,41},{34,41},{42,41},{50,41},{30,53},{38,53},{46,53},{54,53}},
	.i4BlockNumX = 130,
	.i4BlockNumY = 96,
	.iMirrorFlip = 0,
	.i4Crop = { {104,60}, {104,60}, {104,60}, {104,60}, {0,0}},
};
/*no mirror flip*/
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
/****hope add end****/

static kal_uint16 read_module_id(void)
{
	kal_uint16 get_byte=0;
	char pusendcmd[2] = {(char)(MODULE_ID_OFFSET >> 8) , (char)(MODULE_ID_OFFSET & 0xFF) };
	iReadRegI2C(pusendcmd , 2, (u8*)&get_byte,1,0xA2/*EEPROM_READ_ID*/);
	return get_byte;

}

static kal_uint16 read_cmos_sensor_16_16(kal_uint32 addr)
{
	kal_uint16 get_byte= 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};
	/*kdSetI2CSpeed(imgsensor_info.i2c_speed); Add this func to set i2c speed by each sensor*/
	iReadRegI2C(pusendcmd, 2, (u8*)&get_byte, 2, imgsensor.i2c_write_id);
	return ((get_byte << 8) & 0xff00) | ((get_byte >> 8) & 0x00ff);
}


static void write_cmos_sensor_16_16(kal_uint16 addr, kal_uint16 para)
{
	char pusendcmd[4] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para >> 8), (char)(para & 0xFF)};
	/* kdSetI2CSpeed(imgsensor_info.i2c_speed); Add this func to set i2c speed by each sensor*/
	iWriteRegI2C(pusendcmd, 4, imgsensor.i2c_write_id);
}

static kal_uint16 read_cmos_sensor_16_8(kal_uint16 addr)
{
	kal_uint16 get_byte= 0;
	char pusendcmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF)};
	/*kdSetI2CSpeed(imgsensor_info.i2c_speed);  Add this func to set i2c speed by each sensor*/
	iReadRegI2C(pusendcmd, 2, (u8*)&get_byte, 1, imgsensor.i2c_write_id);
	return get_byte;
}

static void write_cmos_sensor_16_8(kal_uint16 addr, kal_uint8 para)
{
	char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para & 0xFF)};
	 /* kdSetI2CSpeed(imgsensor_info.i2c_speed);Add this func to set i2c speed by each sensor*/
	iWriteRegI2C(pusendcmd , 3, imgsensor.i2c_write_id);
}

#define MULTI_WRITE 1
#define I2C_BUFFER_LEN 225	/* trans# max is 255, each 3 bytes */

static kal_uint16 table_write_cmos_sensor(kal_uint16 *para, kal_uint32 len)
{
	char puSendCmd[I2C_BUFFER_LEN];
	kal_uint32 tosend, IDX;
	kal_uint16 addr = 0, addr_last = 0, data;

	tosend = 0;
	IDX = 0;
	while (len > IDX) {
		addr = para[IDX];

		{
			puSendCmd[tosend++] = (char)(addr >> 8);
			puSendCmd[tosend++] = (char)(addr & 0xFF);
			data = para[IDX + 1];
			puSendCmd[tosend++] = (char)(data >> 8);
			puSendCmd[tosend++] = (char)(data & 0xFF);
			IDX += 2;
			addr_last = addr;

		}
		/* Write when remain buffer size is less than 4 bytes or reach end of data */
		if ((I2C_BUFFER_LEN - tosend) < 4 || IDX == len || addr != addr_last) {
			iBurstWriteReg_multi(puSendCmd, tosend, imgsensor.i2c_write_id,
								4, imgsensor_info.i2c_speed);
			tosend = 0;
		}
	}
	return 0;
}

#ifdef VENDOR_EDIT
/*Henry.Chang@Camera.Driver add for 18073 ModuleSN*/
#define  CAMERA_MODULE_INFO_LENGTH  (8)
static kal_uint8 gS5k3m5sx_SN[CAMERA_MODULE_SN_LENGTH];
static kal_uint8 gS5k3m5sx_CamInfo[CAMERA_MODULE_INFO_LENGTH];
/*Henry.Chang@Camera.Driver add for google ARCode Feature verify 20190531*/
static void read_eeprom_CamInfo(void)
{
	kal_uint16 idx = 0;
	kal_uint8 get_byte[12];
	for (idx = 0; idx <12; idx++) {
		char pusendcmd[2] = {0x00 , (char)((0x00 + idx) & 0xFF) };
		iReadRegI2C(pusendcmd , 2, (u8*)&get_byte[idx],1, 0xA0);
		LOG_INF("s5k3m5sx_info[%d]: 0x%x\n", idx, get_byte[idx]);
	}

	gS5k3m5sx_CamInfo[0] = get_byte[0];
	gS5k3m5sx_CamInfo[1] = get_byte[1];
	gS5k3m5sx_CamInfo[2] = get_byte[6];
	gS5k3m5sx_CamInfo[3] = get_byte[7];
	gS5k3m5sx_CamInfo[4] = get_byte[8];
	gS5k3m5sx_CamInfo[5] = get_byte[9];
	gS5k3m5sx_CamInfo[6] = get_byte[10];
	gS5k3m5sx_CamInfo[7] = get_byte[11];
}

static void read_eeprom_SN(void)
{
	kal_uint16 idx = 0;
	kal_uint8 *get_byte= &gS5k3m5sx_SN[0];
	for (idx = 0; idx <CAMERA_MODULE_SN_LENGTH; idx++) {
		char pusendcmd[2] = {0x00 , (char)((0xB0 + idx) & 0xFF) };
		iReadRegI2C(pusendcmd , 2, (u8*)&get_byte[idx],1, 0xA0);
		LOG_INF("gS5k3m5sx_SN[%d]: 0x%x  0x%x\n", idx, get_byte[idx], gS5k3m5sx_SN[idx]);
	}
}

/*Henry.Chang@camera.driver 20181129, add for sensor Module SET*/
#define   WRITE_DATA_MAX_LENGTH     (16)
static kal_int32 table_write_eeprom_30Bytes(kal_uint16 addr, kal_uint8 *para, kal_uint32 len)
{
	kal_int32 ret = IMGSENSOR_RETURN_SUCCESS;
	char pusendcmd[WRITE_DATA_MAX_LENGTH+2];
	pusendcmd[0] = (char)(addr >> 8);
	pusendcmd[1] = (char)(addr & 0xFF);

	memcpy(&pusendcmd[2], para, len);

	ret = iBurstWriteReg((kal_uint8 *)pusendcmd , (len + 2), 0xA0);

	return ret;
}
static kal_uint16 read_cmos_eeprom_8(kal_uint16 addr)
{
	kal_uint16 get_byte=0;
	char pusendcmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	iReadRegI2C(pusendcmd , 2, (u8*)&get_byte, 1, 0xA0);
	return get_byte;
}

static kal_int32 write_eeprom_protect(kal_uint16 enable)
{
	kal_int32 ret = IMGSENSOR_RETURN_SUCCESS;
	char pusendcmd[3];
	pusendcmd[0] = 0x80;
	pusendcmd[1] = 0x00;
	if (enable)
		pusendcmd[2] = 0xE0;
	else
		pusendcmd[2] = 0x00;

	ret = iBurstWriteReg((kal_uint8 *)pusendcmd , 3, 0xA0);

	return ret;
}

/*Henry.Chang@camera.driver 20181129, add for sensor Module SET*/
static kal_int32 write_Module_data(ACDK_SENSOR_ENGMODE_STEREO_STRUCT * pStereodata)
{
	kal_int32  ret = IMGSENSOR_RETURN_SUCCESS;
	kal_uint16 data_base, data_length;
	kal_uint32 idx, idy;
	kal_uint8 *pData;
	UINT32 i = 0;
	if(pStereodata != NULL) {
		pr_debug("s5k3m5sx SET_SENSOR_OTP: 0x%x %d 0x%x %d\n",
                       pStereodata->uSensorId,
                       pStereodata->uDeviceId,
                       pStereodata->baseAddr,
                       pStereodata->dataLength);

		data_base = pStereodata->baseAddr;
		data_length = pStereodata->dataLength;
		pData = pStereodata->uData;
		if ((pStereodata->uSensorId == S5K3M5SX_SENSOR_ID)
			&& (data_length == DUALCAM_CALI_DATA_LENGTH_TELE)
			&& (data_base == S5K3M5SX_STEREO_START_ADDR)) {
			pr_debug("s5k3m5 Write: %x %x %x %x %x %x %x %x\n", pData[0], pData[39], pData[40], pData[904],
					pData[905], pData[906], pData[907], pData[908]);
			idx = data_length/WRITE_DATA_MAX_LENGTH;
			idy = data_length%WRITE_DATA_MAX_LENGTH;
			/* close write protect */
			write_eeprom_protect(0);
			msleep(6);
			for (i = 0; i < idx; i++ ) {
				ret = table_write_eeprom_30Bytes((data_base+WRITE_DATA_MAX_LENGTH*i),
					    &pData[WRITE_DATA_MAX_LENGTH*i], WRITE_DATA_MAX_LENGTH);
				if (ret != IMGSENSOR_RETURN_SUCCESS) {
				    pr_err("write_eeprom error: i= %d\n", i);
					/* open write protect */
					write_eeprom_protect(1);
					msleep(6);
					return IMGSENSOR_RETURN_ERROR;
				}
				msleep(6);
			}
			ret = table_write_eeprom_30Bytes((data_base+WRITE_DATA_MAX_LENGTH*idx),
				      &pData[WRITE_DATA_MAX_LENGTH*idx], idy);
			if (ret != IMGSENSOR_RETURN_SUCCESS) {
				pr_err("write_eeprom error: idx= %d idy= %d\n", idx, idy);
				/* open write protect */
				write_eeprom_protect(1);
				msleep(6);
				return IMGSENSOR_RETURN_ERROR;
			}
			msleep(6);
			/* open write protect */
			write_eeprom_protect(1);
			msleep(6);
			pr_debug("com_0:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR));
			pr_debug("com_39:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR+39));
			pr_debug("innal_40:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR+40));
			pr_debug("innal_1556:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR+904));
			pr_debug("tail1_1557:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR+905));
			pr_debug("tail2_1558:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR+906));
			pr_debug("tail3_1559:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR+907));
			pr_debug("tail4_1560:0x%x\n", read_cmos_eeprom_8(S5K3M5SX_STEREO_START_ADDR+908));
			pr_debug("s5k3m5write_Module_data Write end\n");
		} else if ((pStereodata->uSensorId == S5K3M5SX_SENSOR_ID)
			&& (data_base == S5K3M5SX_AESYNC_START_ADDR)
			&& (data_length < AESYNC_DATA_LENGTH_TOTAL)) {
			pr_debug("write main aesync: %x %x %x %x %x %x %x %x\n", pData[0], pData[1],
				pData[2], pData[3], pData[4], pData[5], pData[6], pData[7]);
			/* close write protect */
			write_eeprom_protect(0);
			msleep(6);
			ret = table_write_eeprom_30Bytes(data_base, &pData[0], AESYNC_DATA_LENGTH_TOTAL);
			msleep(6);
			/* open write protect */
			write_eeprom_protect(1);
			msleep(6);
			if (ret != IMGSENSOR_RETURN_SUCCESS) {
				pr_err("write TELE_aesync_eeprom error\n");
				return IMGSENSOR_RETURN_ERROR;
			}
			pr_debug("readback TELE aesync: %x %x %x %x %x %x %x %x\n"
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR)
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR+1)
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR+2)
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR+3)
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR+4)
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR+5)
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR+6)
				, read_cmos_eeprom_8(S5K3M5SX_AESYNC_START_ADDR+7));
		} else {
			pr_err("Invalid Sensor id:0x%x write_gm1 eeprom\n", pStereodata->uSensorId);
			return IMGSENSOR_RETURN_ERROR;
		}
	} else {
		pr_err("s5k3m5sx write_Module_data pStereodata is null\n");
		return IMGSENSOR_RETURN_ERROR;
	}
	return ret;
}
#endif
static void set_dummy(void)
{
	LOG_INF("dummyline = %d, dummypixels = %d \n", imgsensor.dummy_line, imgsensor.dummy_pixel);
	write_cmos_sensor_16_16(0x0340, imgsensor.frame_length);
	write_cmos_sensor_16_16(0x0342, imgsensor.line_length);
}	/*	set_dummy  */


static void set_max_framerate(UINT16 framerate,kal_bool min_framelength_en)
{

	kal_uint32 frame_length = imgsensor.frame_length;

	LOG_INF("framerate = %d, min framelength should enable %d \n", framerate,min_framelength_en);

	frame_length = imgsensor.pclk / framerate * 10 / imgsensor.line_length;
	spin_lock(&imgsensor_drv_lock);
	if (frame_length >= imgsensor.min_frame_length) {
		imgsensor.frame_length = frame_length;
	} else {
		imgsensor.frame_length = imgsensor.min_frame_length;
	}
	imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
		imgsensor.dummy_line = imgsensor.frame_length - imgsensor.min_frame_length;
	}
	if (min_framelength_en) {
		imgsensor.min_frame_length = imgsensor.frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);
	set_dummy();
}	/*	set_max_framerate  */

static void write_shutter(kal_uint16 shutter)
{
	kal_uint16 realtime_fps = 0;

	spin_lock(&imgsensor_drv_lock);
	if (shutter > imgsensor.min_frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	} else {
		imgsensor.frame_length = imgsensor.min_frame_length;
	}
	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	}
	spin_unlock(&imgsensor_drv_lock);
	if (shutter < imgsensor_info.min_shutter) {
		shutter = imgsensor_info.min_shutter;
	}

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296,0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146,0);
		} else {
			/* Extend frame length*/
			write_cmos_sensor_16_16(0x0340, imgsensor.frame_length);
		}
	} else {
		/* Extend frame length*/
		write_cmos_sensor_16_16(0x0340, imgsensor.frame_length);
	}

	/* Update Shutter*/
	write_cmos_sensor_16_16(0x0202, shutter);
	LOG_INF("shutter = %d, framelength = %d\n", shutter,imgsensor.frame_length);

}	/*	write_shutter  */



/*************************************************************************
* FUNCTION
*	set_shutter
*
* DESCRIPTION
*	This function set e-shutter of sensor to change exposure time.
*
* PARAMETERS
*	iShutter : exposured lines
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void set_shutter(kal_uint16 shutter)
{
	unsigned long flags;
	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	write_shutter(shutter);
}	/*	set_shutter */

/*	write_shutter  */
static void set_shutter_frame_length(kal_uint16 shutter, kal_uint16 frame_length)
{
	unsigned long flags;
	kal_uint16 realtime_fps = 0;
	kal_int32 dummy_line = 0;

	spin_lock_irqsave(&imgsensor_drv_lock, flags);
	imgsensor.shutter = shutter;
	spin_unlock_irqrestore(&imgsensor_drv_lock, flags);

	spin_lock(&imgsensor_drv_lock);
	/* Change frame time */
	if (frame_length > 1) {
		dummy_line = frame_length - imgsensor.frame_length;
	}
	imgsensor.frame_length = imgsensor.frame_length + dummy_line;


	if (shutter > imgsensor.frame_length - imgsensor_info.margin) {
		imgsensor.frame_length = shutter + imgsensor_info.margin;
	}

	if (imgsensor.frame_length > imgsensor_info.max_frame_length) {
		imgsensor.frame_length = imgsensor_info.max_frame_length;
	}

	spin_unlock(&imgsensor_drv_lock);
	shutter = (shutter < imgsensor_info.min_shutter) ? imgsensor_info.min_shutter : shutter;
	shutter = (shutter > (imgsensor_info.max_frame_length - imgsensor_info.margin))
		? (imgsensor_info.max_frame_length - imgsensor_info.margin) : shutter;

	if (imgsensor.autoflicker_en) {
		realtime_fps = imgsensor.pclk / imgsensor.line_length * 10 / imgsensor.frame_length;
		if (realtime_fps >= 297 && realtime_fps <= 305) {
			set_max_framerate(296, 0);
		} else if (realtime_fps >= 147 && realtime_fps <= 150) {
			set_max_framerate(146, 0);
		} else {
			/* Extend frame length */
			write_cmos_sensor_16_16(0x0340, imgsensor.frame_length & 0xFFFF);
		}
	} else {
		/* Extend frame length */
		write_cmos_sensor_16_16(0x0340, imgsensor.frame_length & 0xFFFF);
	}

	/* Update Shutter */
	write_cmos_sensor_16_16(0x0202, shutter & 0xFFFF);

	LOG_INF("shutter = %d, framelength = %d/%d, dummy_line= %d\n", shutter, imgsensor.frame_length,
		frame_length, dummy_line);

}/*      write_shutter  */


static kal_uint16 gain2reg(const kal_uint16 gain)
{
	 kal_uint16 reg_gain = 0x0;

	reg_gain = gain/2;
	return (kal_uint16)reg_gain;
}

/*************************************************************************
* FUNCTION
*	set_gain
*
* DESCRIPTION
*	This function is to set global gain to sensor.
*
* PARAMETERS
*	iGain : sensor global gain(base: 0x40)
*
* RETURNS
*	the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint16 set_gain(kal_uint16 gain)
{
	kal_uint16 reg_gain;

	/*gain= 1024;for test*/
	/*return; for test*/

	if (gain < BASEGAIN || gain > 32 * BASEGAIN) {
		LOG_INF("Error gain setting");

		if (gain < BASEGAIN) {
			gain = BASEGAIN;
		} else if (gain > 32 * BASEGAIN) {
			gain = 32 * BASEGAIN;
		}
	}

	reg_gain = gain2reg(gain);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.gain = reg_gain;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("gain = %d , reg_gain = 0x%x\n ", gain, reg_gain);

	write_cmos_sensor_16_16(0x0204,reg_gain);
	/*write_cmos_sensor_16_8(0x0204,(reg_gain>>8));*/
	/*write_cmos_sensor_16_8(0x0205,(reg_gain&0xff));*/

	return gain;
}	/*	set_gain  */

static void set_mirror_flip(kal_uint8 image_mirror)
{
	switch (image_mirror) {

		case IMAGE_NORMAL:
			write_cmos_sensor_16_8(0x0101, 0x00);   /* Gr*/
			break;

		case IMAGE_H_MIRROR:
			write_cmos_sensor_16_8(0x0101, 0x01);
			break;

		case IMAGE_V_MIRROR:
			write_cmos_sensor_16_8(0x0101, 0x02);
			break;

		case IMAGE_HV_MIRROR:
			write_cmos_sensor_16_8(0x0101, 0x03);  /*Gb*/
			break;
		default:
			LOG_INF("Error image_mirror setting\n");
	}
}

/*************************************************************************
* FUNCTION
*	night_mode
*
* DESCRIPTION
*	This function night mode of sensor.
*
* PARAMETERS
*	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 streaming_control(kal_bool enable)
{
	int timeout = (10000 / imgsensor.current_fps) + 1;
	int i = 0;
	int framecnt = 0;

	LOG_INF("streaming_enable(0= Sw Standby,1= streaming): %d\n", enable);
	if (enable) {
		write_cmos_sensor_16_8(0x0100, 0X01);
		mDELAY(10);
	} else {
		write_cmos_sensor_16_8(0x0100, 0x00);
		for (i = 0; i < timeout; i++) {
			mDELAY(5);
			framecnt = read_cmos_sensor_16_8(0x0005);
			if (framecnt == 0xFF) {
				LOG_INF(" Stream Off OK at i=%d.\n", i);
				return ERROR_NONE;
			}
		}
		LOG_INF("Stream Off Fail! framecnt= %d.\n", framecnt);
	}
	return ERROR_NONE;
}

static kal_uint16 addr_data_pair_global[] = {
0x0A02, 0x7800,
0x6028, 0x2000,
0x602A, 0x3EAC,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0549,
0x6F12, 0x0448,
0x6F12, 0x054A,
0x6F12, 0xC1F8,
0x6F12, 0xC804,
0x6F12, 0x101A,
0x6F12, 0xA1F8,
0x6F12, 0xCC04,
0x6F12, 0x00F0,
0x6F12, 0x70BA,
0x6F12, 0x2000,
0x6F12, 0x4594,
0x6F12, 0x2000,
0x6F12, 0x2E50,
0x6F12, 0x2000,
0x6F12, 0x7000,
0x6F12, 0x10B5,
0x6F12, 0x00F0,
0x6F12, 0xB7FA,
0x6F12, 0xFF49,
0x6F12, 0x0120,
0x6F12, 0x0880,
0x6F12, 0x10BD,
0x6F12, 0x2DE9,
0x6F12, 0xF041,
0x6F12, 0xFD4C,
0x6F12, 0xFB4F,
0x6F12, 0x0026,
0x6F12, 0xB4F8,
0x6F12, 0x6A52,
0x6F12, 0x3888,
0x6F12, 0x08B1,
0x6F12, 0xA4F8,
0x6F12, 0x6A62,
0x6F12, 0x00F0,
0x6F12, 0xABFA,
0x6F12, 0x3E80,
0x6F12, 0xA4F8,
0x6F12, 0x6A52,
0x6F12, 0xBDE8,
0x6F12, 0xF081,
0x6F12, 0x2DE9,
0x6F12, 0xF041,
0x6F12, 0x0746,
0x6F12, 0xF248,
0x6F12, 0x0E46,
0x6F12, 0x0022,
0x6F12, 0x4068,
0x6F12, 0x84B2,
0x6F12, 0x050C,
0x6F12, 0x2146,
0x6F12, 0x2846,
0x6F12, 0x00F0,
0x6F12, 0x9EFA,
0x6F12, 0x3146,
0x6F12, 0x3846,
0x6F12, 0x00F0,
0x6F12, 0x9FFA,
0x6F12, 0xED4F,
0x6F12, 0x4DF2,
0x6F12, 0x0C26,
0x6F12, 0x4FF4,
0x6F12, 0x8061,
0x6F12, 0x3A78,
0x6F12, 0x3046,
0x6F12, 0x00F0,
0x6F12, 0x91FA,
0x6F12, 0x7878,
0x6F12, 0xB8B3,
0x6F12, 0x0022,
0x6F12, 0x8021,
0x6F12, 0x3046,
0x6F12, 0x00F0,
0x6F12, 0x8AFA,
0x6F12, 0xE648,
0x6F12, 0x0088,
0x6F12, 0xE64B,
0x6F12, 0xA3F8,
0x6F12, 0x5C02,
0x6F12, 0xE448,
0x6F12, 0x001D,
0x6F12, 0x0088,
0x6F12, 0xA3F8,
0x6F12, 0x5E02,
0x6F12, 0xB3F8,
0x6F12, 0x5C02,
0x6F12, 0xB3F8,
0x6F12, 0x5E12,
0x6F12, 0x4218,
0x6F12, 0x02D0,
0x6F12, 0x8002,
0x6F12, 0xB0FB,
0x6F12, 0xF2F2,
0x6F12, 0x91B2,
0x6F12, 0xDE4A,
0x6F12, 0xA3F8,
0x6F12, 0x6012,
0x6F12, 0xB2F8,
0x6F12, 0x1602,
0x6F12, 0xB2F8,
0x6F12, 0x1422,
0x6F12, 0xA3F8,
0x6F12, 0x9805,
0x6F12, 0xA3F8,
0x6F12, 0x9A25,
0x6F12, 0x8018,
0x6F12, 0x04D0,
0x6F12, 0x9202,
0x6F12, 0xB2FB,
0x6F12, 0xF0F0,
0x6F12, 0xA3F8,
0x6F12, 0x9C05,
0x6F12, 0xB3F8,
0x6F12, 0x9C05,
0x6F12, 0x0A18,
0x6F12, 0x01FB,
0x6F12, 0x1020,
0x6F12, 0x40F3,
0x6F12, 0x9510,
0x6F12, 0x1028,
0x6F12, 0x06DC,
0x6F12, 0x0028,
0x6F12, 0x05DA,
0x6F12, 0x0020,
0x6F12, 0x03E0,
0x6F12, 0xFFE7,
0x6F12, 0x0122,
0x6F12, 0xC5E7,
0x6F12, 0x1020,
0x6F12, 0xCE49,
0x6F12, 0x0880,
0x6F12, 0x2146,
0x6F12, 0x2846,
0x6F12, 0xBDE8,
0x6F12, 0xF041,
0x6F12, 0x0122,
0x6F12, 0x00F0,
0x6F12, 0x4ABA,
0x6F12, 0xF0B5,
0x6F12, 0xCA4C,
0x6F12, 0xDDE9,
0x6F12, 0x0565,
0x6F12, 0x08B1,
0x6F12, 0x2788,
0x6F12, 0x0760,
0x6F12, 0x09B1,
0x6F12, 0x6088,
0x6F12, 0x0860,
0x6F12, 0x12B1,
0x6F12, 0xA088,
0x6F12, 0x401C,
0x6F12, 0x1060,
0x6F12, 0x0BB1,
0x6F12, 0xE088,
0x6F12, 0x1860,
0x6F12, 0x0EB1,
0x6F12, 0xA07B,
0x6F12, 0x3060,
0x6F12, 0x002D,
0x6F12, 0x01D0,
0x6F12, 0xE07B,
0x6F12, 0x2860,
0x6F12, 0xF0BD,
0x6F12, 0x70B5,
0x6F12, 0x0646,
0x6F12, 0xB648,
0x6F12, 0x0022,
0x6F12, 0x8068,
0x6F12, 0x84B2,
0x6F12, 0x050C,
0x6F12, 0x2146,
0x6F12, 0x2846,
0x6F12, 0x00F0,
0x6F12, 0x26FA,
0x6F12, 0x3046,
0x6F12, 0x00F0,
0x6F12, 0x2DFA,
0x6F12, 0xB848,
0x6F12, 0x0368,
0x6F12, 0xB3F8,
0x6F12, 0x7401,
0x6F12, 0x010A,
0x6F12, 0xB648,
0x6F12, 0x4268,
0x6F12, 0x82F8,
0x6F12, 0x5010,
0x6F12, 0x93F8,
0x6F12, 0x7511,
0x6F12, 0x82F8,
0x6F12, 0x5210,
0x6F12, 0xB3F8,
0x6F12, 0x7811,
0x6F12, 0x090A,
0x6F12, 0x82F8,
0x6F12, 0x5810,
0x6F12, 0x93F8,
0x6F12, 0x7911,
0x6F12, 0x82F8,
0x6F12, 0x5A10,
0x6F12, 0x33F8,
0x6F12, 0xF01F,
0x6F12, 0x0068,
0x6F12, 0x090A,
0x6F12, 0x00F8,
0x6F12, 0xCE1F,
0x6F12, 0x5978,
0x6F12, 0x8170,
0x6F12, 0x5988,
0x6F12, 0x090A,
0x6F12, 0x0171,
0x6F12, 0xD978,
0x6F12, 0x8171,
0x6F12, 0x988C,
0x6F12, 0x000A,
0x6F12, 0x9074,
0x6F12, 0x93F8,
0x6F12, 0x2500,
0x6F12, 0x1075,
0x6F12, 0xD88C,
0x6F12, 0x000A,
0x6F12, 0x9075,
0x6F12, 0x93F8,
0x6F12, 0x2700,
0x6F12, 0x1076,
0x6F12, 0xB3F8,
0x6F12, 0xB000,
0x6F12, 0x000A,
0x6F12, 0x82F8,
0x6F12, 0x7E00,
0x6F12, 0x93F8,
0x6F12, 0xB100,
0x6F12, 0x82F8,
0x6F12, 0x8000,
0x6F12, 0x9548,
0x6F12, 0x90F8,
0x6F12, 0xB313,
0x6F12, 0x82F8,
0x6F12, 0x8210,
0x6F12, 0x90F8,
0x6F12, 0xB103,
0x6F12, 0x82F8,
0x6F12, 0x8400,
0x6F12, 0x93F8,
0x6F12, 0xB400,
0x6F12, 0x82F8,
0x6F12, 0x8600,
0x6F12, 0x0020,
0x6F12, 0x82F8,
0x6F12, 0x8800,
0x6F12, 0x93F8,
0x6F12, 0x6211,
0x6F12, 0x82F8,
0x6F12, 0x9610,
0x6F12, 0x93F8,
0x6F12, 0x0112,
0x6F12, 0x82F8,
0x6F12, 0x9E10,
0x6F12, 0x93F8,
0x6F12, 0x0212,
0x6F12, 0x82F8,
0x6F12, 0xA010,
0x6F12, 0x82F8,
0x6F12, 0xA200,
0x6F12, 0x82F8,
0x6F12, 0xA400,
0x6F12, 0x93F8,
0x6F12, 0x0512,
0x6F12, 0x82F8,
0x6F12, 0xA610,
0x6F12, 0x93F8,
0x6F12, 0x0612,
0x6F12, 0x82F8,
0x6F12, 0xA810,
0x6F12, 0x93F8,
0x6F12, 0x0712,
0x6F12, 0x82F8,
0x6F12, 0xAA10,
0x6F12, 0x82F8,
0x6F12, 0xAC00,
0x6F12, 0x5A20,
0x6F12, 0x82F8,
0x6F12, 0xAD00,
0x6F12, 0x93F8,
0x6F12, 0x0902,
0x6F12, 0x82F8,
0x6F12, 0xAE00,
0x6F12, 0x2146,
0x6F12, 0x2846,
0x6F12, 0xBDE8,
0x6F12, 0x7040,
0x6F12, 0x0122,
0x6F12, 0x00F0,
0x6F12, 0xAFB9,
0x6F12, 0x70B5,
0x6F12, 0x7548,
0x6F12, 0x0022,
0x6F12, 0x0169,
0x6F12, 0x0C0C,
0x6F12, 0x8DB2,
0x6F12, 0x2946,
0x6F12, 0x2046,
0x6F12, 0x00F0,
0x6F12, 0xA5F9,
0x6F12, 0x00F0,
0x6F12, 0xB2F9,
0x6F12, 0x7248,
0x6F12, 0x8078,
0x6F12, 0x08B1,
0x6F12, 0x4F22,
0x6F12, 0x00E0,
0x6F12, 0x2522,
0x6F12, 0x7748,
0x6F12, 0x90F8,
0x6F12, 0xE400,
0x6F12, 0x0328,
0x6F12, 0x07D1,
0x6F12, 0x42F0,
0x6F12, 0x8002,
0x6F12, 0x4FF4,
0x6F12, 0x8361,
0x6F12, 0x48F6,
0x6F12, 0x7A20,
0x6F12, 0x00F0,
0x6F12, 0xA4F9,
0x6F12, 0x2946,
0x6F12, 0x2046,
0x6F12, 0xBDE8,
0x6F12, 0x7040,
0x6F12, 0x0122,
0x6F12, 0x00F0,
0x6F12, 0x89B9,
0x6F12, 0x10B5,
0x6F12, 0x0221,
0x6F12, 0x7620,
0x6F12, 0x00F0,
0x6F12, 0x9DF9,
0x6F12, 0x0221,
0x6F12, 0x4420,
0x6F12, 0x00F0,
0x6F12, 0x99F9,
0x6F12, 0x4021,
0x6F12, 0x4520,
0x6F12, 0x00F0,
0x6F12, 0x95F9,
0x6F12, 0x5D49,
0x6F12, 0x0420,
0x6F12, 0xA1F8,
0x6F12, 0x3A06,
0x6F12, 0x10BD,
0x6F12, 0x7047,
0x6F12, 0x7047,
0x6F12, 0x08B5,
0x6F12, 0x5949,
0x6F12, 0x3120,
0x6F12, 0x6A46,
0x6F12, 0x81F8,
0x6F12, 0x4306,
0x6F12, 0x5A20,
0x6F12, 0x8DF8,
0x6F12, 0x0000,
0x6F12, 0x0121,
0x6F12, 0x7520,
0x6F12, 0x00F0,
0x6F12, 0x86F9,
0x6F12, 0x9DF8,
0x6F12, 0x0000,
0x6F12, 0x08BD,
0x6F12, 0x7047,
0x6F12, 0x5248,
0x6F12, 0x10B5,
0x6F12, 0x8078,
0x6F12, 0x18B1,
0x6F12, 0x0021,
0x6F12, 0x4420,
0x6F12, 0x00F0,
0x6F12, 0x75F9,
0x6F12, 0x4D49,
0x6F12, 0x0220,
0x6F12, 0xA1F8,
0x6F12, 0x3A06,
0x6F12, 0x10BD,
0x6F12, 0x5448,
0x6F12, 0x90F8,
0x6F12, 0xE400,
0x6F12, 0x0328,
0x6F12, 0x01D0,
0x6F12, 0x00F0,
0x6F12, 0x73B9,
0x6F12, 0xAFF2,
0x6F12, 0x2B01,
0x6F12, 0x4648,
0x6F12, 0xC0F8,
0x6F12, 0x4C16,
0x6F12, 0x4649,
0x6F12, 0x8978,
0x6F12, 0x11B1,
0x6F12, 0xAFF2,
0x6F12, 0x8501,
0x6F12, 0x01E0,
0x6F12, 0xAFF2,
0x6F12, 0x6501,
0x6F12, 0xC0F8,
0x6F12, 0x4816,
0x6F12, 0xAFF2,
0x6F12, 0x6B01,
0x6F12, 0xC0F8,
0x6F12, 0x4416,
0x6F12, 0xAFF2,
0x6F12, 0x7101,
0x6F12, 0xC0F8,
0x6F12, 0x5016,
0x6F12, 0xAFF2,
0x6F12, 0x5901,
0x6F12, 0xC0F8,
0x6F12, 0x5416,
0x6F12, 0x7047,
0x6F12, 0x2DE9,
0x6F12, 0xF041,
0x6F12, 0x434C,
0x6F12, 0x4249,
0x6F12, 0x0646,
0x6F12, 0xB4F8,
0x6F12, 0x6670,
0x6F12, 0xC989,
0x6F12, 0xB4F8,
0x6F12, 0x7E20,
0x6F12, 0x0020,
0x6F12, 0xC1B1,
0x6F12, 0x2146,
0x6F12, 0xD1F8,
0x6F12, 0x9010,
0x6F12, 0x72B1,
0x6F12, 0x8FB1,
0x6F12, 0x0846,
0x6F12, 0x00F0,
0x6F12, 0x48F9,
0x6F12, 0x0546,
0x6F12, 0xA06F,
0x6F12, 0x00F0,
0x6F12, 0x44F9,
0x6F12, 0x8542,
0x6F12, 0x02D2,
0x6F12, 0xD4F8,
0x6F12, 0x9000,
0x6F12, 0x26E0,
0x6F12, 0xA06F,
0x6F12, 0x24E0,
0x6F12, 0x002F,
0x6F12, 0xFBD1,
0x6F12, 0x002A,
0x6F12, 0x24D0,
0x6F12, 0x0846,
0x6F12, 0x1EE0,
0x6F12, 0x2849,
0x6F12, 0x8D88,
0x6F12, 0x8968,
0x6F12, 0x4B42,
0x6F12, 0x77B1,
0x6F12, 0x2F48,
0x6F12, 0x406F,
0x6F12, 0x10E0,
0x6F12, 0x4242,
0x6F12, 0x00E0,
0x6F12, 0x0246,
0x6F12, 0x0029,
0x6F12, 0x0FDB,
0x6F12, 0x8A42,
0x6F12, 0x0FDD,
0x6F12, 0x3046,
0x6F12, 0xBDE8,
0x6F12, 0xF041,
0x6F12, 0x00F0,
0x6F12, 0x28B9,
0x6F12, 0x002A,
0x6F12, 0x0CD0,
0x6F12, 0x2748,
0x6F12, 0xD0F8,
0x6F12, 0x8800,
0x6F12, 0x25B1,
0x6F12, 0x0028,
0x6F12, 0xEDDA,
0x6F12, 0xEAE7,
0x6F12, 0x1946,
0x6F12, 0xEDE7,
0x6F12, 0x00F0,
0x6F12, 0x20F9,
0x6F12, 0xE060,
0x6F12, 0x0120,
0x6F12, 0x3DE6,
0x6F12, 0x2DE9,
0x6F12, 0xF047,
0x6F12, 0x8146,
0x6F12, 0x0F46,
0x6F12, 0x0846,
0x6F12, 0x00F0,
0x6F12, 0x1BF9,
0x6F12, 0x1B4C,
0x6F12, 0x0026,
0x6F12, 0x608A,
0x6F12, 0x10B1,
0x6F12, 0x00F0,
0x6F12, 0x1AF9,
0x6F12, 0x6682,
0x6F12, 0x194D,
0x6F12, 0x2888,
0x6F12, 0x0128,
0x6F12, 0x60D1,
0x6F12, 0xA08B,
0x6F12, 0x0028,
0x6F12, 0x5DD1,
0x6F12, 0x002F,
0x6F12, 0x5BD1,
0x6F12, 0x104F,
0x6F12, 0x3868,
0x6F12, 0xB0F8,
0x6F12, 0x1403,
0x6F12, 0x38B1,
0x6F12, 0x2889,
0x6F12, 0x401C,
0x6F12, 0x80B2,
0x6F12, 0x2881,
0x6F12, 0xFF28,
0x6F12, 0x01D9,
0x6F12, 0xA08C,
0x6F12, 0x2881,
0x6F12, 0x0F48,
0x6F12, 0xEE60,
0x6F12, 0xB0F8,
0x6F12, 0x5E80,
0x6F12, 0x1BE0,
0x6F12, 0x2000,
0x6F12, 0x4580,
0x6F12, 0x2000,
0x6F12, 0x2E50,
0x6F12, 0x2000,
0x6F12, 0x6200,
0x6F12, 0x4000,
0x6F12, 0x9404,
0x6F12, 0x2000,
0x6F12, 0x38E0,
0x6F12, 0x4000,
0x6F12, 0xD000,
0x6F12, 0x4000,
0x6F12, 0xA410,
0x6F12, 0x2000,
0x6F12, 0x2C66,
0x6F12, 0x2000,
0x6F12, 0x0890,
0x6F12, 0x2000,
0x6F12, 0x3620,
0x6F12, 0x2000,
0x6F12, 0x0DE0,
0x6F12, 0x2000,
0x6F12, 0x2BC0,
0x6F12, 0x2000,
0x6F12, 0x3580,
0x6F12, 0x4000,
0x6F12, 0x7000,
0x6F12, 0x40F2,
0x6F12, 0xFF31,
0x6F12, 0x0B20,
0x6F12, 0x00F0,
0x6F12, 0xE2F8,
0x6F12, 0x3868,
0x6F12, 0xB0F8,
0x6F12, 0x1213,
0x6F12, 0x19B1,
0x6F12, 0x4846,
0x6F12, 0x00F0,
0x6F12, 0xC7F8,
0x6F12, 0x0AE0,
0x6F12, 0xB0F8,
0x6F12, 0x1403,
0x6F12, 0xC0B1,
0x6F12, 0x2889,
0x6F12, 0xB4F9,
0x6F12, 0x2410,
0x6F12, 0x8842,
0x6F12, 0x13DB,
0x6F12, 0x4846,
0x6F12, 0xFFF7,
0x6F12, 0x5AFF,
0x6F12, 0x78B1,
0x6F12, 0x2E81,
0x6F12, 0x00F0,
0x6F12, 0xD0F8,
0x6F12, 0xE868,
0x6F12, 0x2861,
0x6F12, 0x208E,
0x6F12, 0x18B1,
0x6F12, 0x608E,
0x6F12, 0x18B9,
0x6F12, 0x00F0,
0x6F12, 0xCDF8,
0x6F12, 0x608E,
0x6F12, 0x10B1,
0x6F12, 0xE889,
0x6F12, 0x2887,
0x6F12, 0x6686,
0x6F12, 0x4046,
0x6F12, 0xBDE8,
0x6F12, 0xF047,
0x6F12, 0x00F0,
0x6F12, 0xC8B8,
0x6F12, 0xBDE8,
0x6F12, 0xF087,
0x6F12, 0x10B5,
0x6F12, 0x6021,
0x6F12, 0x0B20,
0x6F12, 0x00F0,
0x6F12, 0xC6F8,
0x6F12, 0x8106,
0x6F12, 0x4FEA,
0x6F12, 0x4061,
0x6F12, 0x05D5,
0x6F12, 0x0029,
0x6F12, 0x0BDA,
0x6F12, 0xBDE8,
0x6F12, 0x1040,
0x6F12, 0x00F0,
0x6F12, 0xC1B8,
0x6F12, 0x0029,
0x6F12, 0x03DA,
0x6F12, 0xBDE8,
0x6F12, 0x1040,
0x6F12, 0x00F0,
0x6F12, 0xC0B8,
0x6F12, 0x8006,
0x6F12, 0x03D5,
0x6F12, 0xBDE8,
0x6F12, 0x1040,
0x6F12, 0x00F0,
0x6F12, 0xBFB8,
0x6F12, 0x10BD,
0x6F12, 0x70B5,
0x6F12, 0x1E4C,
0x6F12, 0x0020,
0x6F12, 0x2080,
0x6F12, 0xAFF2,
0x6F12, 0xDF40,
0x6F12, 0x1C4D,
0x6F12, 0x2861,
0x6F12, 0xAFF2,
0x6F12, 0xD940,
0x6F12, 0x0022,
0x6F12, 0xAFF2,
0x6F12, 0xB941,
0x6F12, 0xA861,
0x6F12, 0x1948,
0x6F12, 0x00F0,
0x6F12, 0xB2F8,
0x6F12, 0x0022,
0x6F12, 0xAFF2,
0x6F12, 0xD531,
0x6F12, 0x6060,
0x6F12, 0x1748,
0x6F12, 0x00F0,
0x6F12, 0xABF8,
0x6F12, 0x0022,
0x6F12, 0xAFF2,
0x6F12, 0x1341,
0x6F12, 0xA060,
0x6F12, 0x1448,
0x6F12, 0x00F0,
0x6F12, 0xA4F8,
0x6F12, 0x0022,
0x6F12, 0xAFF2,
0x6F12, 0xED21,
0x6F12, 0xE060,
0x6F12, 0x1248,
0x6F12, 0x00F0,
0x6F12, 0x9DF8,
0x6F12, 0x2061,
0x6F12, 0xAFF2,
0x6F12, 0x4920,
0x6F12, 0x0022,
0x6F12, 0xAFF2,
0x6F12, 0x0B21,
0x6F12, 0xE863,
0x6F12, 0x0E48,
0x6F12, 0x00F0,
0x6F12, 0x93F8,
0x6F12, 0x0022,
0x6F12, 0xAFF2,
0x6F12, 0x8511,
0x6F12, 0x0C48,
0x6F12, 0x00F0,
0x6F12, 0x8DF8,
0x6F12, 0x0022,
0x6F12, 0xAFF2,
0x6F12, 0xA701,
0x6F12, 0xBDE8,
0x6F12, 0x7040,
0x6F12, 0x0948,
0x6F12, 0x00F0,
0x6F12, 0x85B8,
0x6F12, 0x2000,
0x6F12, 0x4580,
0x6F12, 0x2000,
0x6F12, 0x0840,
0x6F12, 0x0001,
0x6F12, 0x020D,
0x6F12, 0x0000,
0x6F12, 0x67CD,
0x6F12, 0x0000,
0x6F12, 0x3AE1,
0x6F12, 0x0000,
0x6F12, 0x72B1,
0x6F12, 0x0000,
0x6F12, 0x56D7,
0x6F12, 0x0000,
0x6F12, 0x5735,
0x6F12, 0x0000,
0x6F12, 0x0631,
0x6F12, 0x45F6,
0x6F12, 0x250C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F6,
0x6F12, 0xF31C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x4AF2,
0x6F12, 0xD74C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x40F2,
0x6F12, 0x0D2C,
0x6F12, 0xC0F2,
0x6F12, 0x010C,
0x6F12, 0x6047,
0x6F12, 0x46F2,
0x6F12, 0xCD7C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x47F2,
0x6F12, 0xB12C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x47F2,
0x6F12, 0x4F2C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x47F6,
0x6F12, 0x017C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x47F6,
0x6F12, 0x636C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x47F2,
0x6F12, 0x0D0C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x4AF2,
0x6F12, 0x5F4C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0xA56C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0x1F5C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0x7F5C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0x312C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x40F2,
0x6F12, 0xAB2C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0xF34C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0x395C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x40F2,
0x6F12, 0x117C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x40F2,
0x6F12, 0xD92C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0x054C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0xAF3C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x45F2,
0x6F12, 0x4B2C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x4AF6,
0x6F12, 0xE75C,
0x6F12, 0xC0F2,
0x6F12, 0x000C,
0x6F12, 0x6047,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x30D5,
0x6F12, 0x0103,
0x6F12, 0x0000,
0x6F12, 0x005E,
0x602A, 0x1662,
0x6F12, 0x1E00,
0x602A, 0x1C9A,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x6F12, 0x0000,
0x602A, 0x0FF2,
0x6F12, 0x0020,
0x602A, 0x0EF6,
0x6F12, 0x0100,
0x602A, 0x23B2,
0x6F12, 0x0001,
0x602A, 0x0FE4,
0x6F12, 0x0107,
0x6F12, 0x07D0,
0x602A, 0x12F8,
0x6F12, 0x3D09,
0x602A, 0x0E18,
0x6F12, 0x0040,
0x602A, 0x1066,
0x6F12, 0x000C,
0x602A, 0x13DE,
0x6F12, 0x0000,
0x602A, 0x12F2,
0x6F12, 0x0F0F,
0x602A, 0x13DC,
0x6F12, 0x806F,
0xF46E, 0x00C3,
0xF46C, 0xBFA0,
0xF44A, 0x0007,
0xF456, 0x000A,
0x6028, 0x2000,
0x602A, 0x12F6,
0x6F12, 0x7008,
0x0BC6, 0x0000,
0x0B36, 0x0001,
0x6028, 0x2000,
0x602A, 0x2BC2,
0x6F12, 0x0020,
0x602A, 0x2BC4,
0x6F12, 0x0020,
0x602A, 0x6204,
0x6F12, 0x0001,
0x602A, 0x6208,
0x6F12, 0x0000,
0x6F12, 0x0030,
0x6028, 0x2000,
0x602A, 0x17C0,
0x6F12, 0x143C,
};

static kal_uint16 addr_data_pair_preview[] = {
0x6214, 0x7971,
0x6218, 0x7150,
0x0344, 0x0070,
0x0346, 0x0044,
0x0348, 0x100F,
0x034A, 0x0BFB,
0x034C, 0x0FA0,
0x034E, 0x0BB8,
0x0340, 0x0CF0,
0x0342, 0x12F0,
0x0900, 0x0011,
0x0380, 0x0001,
0x0382, 0x0001,
0x0384, 0x0001,
0x0386, 0x0001,
0x0402, 0x1010,
0x0404, 0x1000,
0x0350, 0x0000,
0x0352, 0x0000,
0x0136, 0x1800,
0x013E, 0x0000,
0x0300, 0x0008,
0x0302, 0x0001,
0x0304, 0x0006,
0x0306, 0x00F1,
0x0308, 0x0008,
0x030A, 0x0001,
0x030C, 0x0000,
0x030E, 0x0003,
0x0310, 0x005A,
0x0312, 0x0000,
0x0B06, 0x0101,
0x6028, 0x2000,
0x602A, 0x1FF6,
0x6F12, 0x0000,
0x021E, 0x0000,
0x0202, 0x0100,
0x0204, 0x0020,
//0x0D00, 0x0100, normal mode
//0x0D02, 0x0001,
0x0D00, 0x0101, //tail mode
0x0D02, 0x0101,
0x0114, 0x0301,
0x0D06, 0x01F0,
0x0D08, 0x02E0,
0x6028, 0x2000,
0x602A, 0x0F10,
0x6F12, 0x0003,
0x602A, 0x0F12,
0x6F12, 0x0200,
0x602A, 0x2BC0,
0x6F12, 0x0001,
0x0B30, 0x0000,
0x0B32, 0x0000,
0x0B34, 0x0001,
};

static kal_uint16 addr_data_pair_slimvideo[] = {
0x6214, 0x7971,
0x6218, 0x7150,
0x0344, 0x0000,
0x0346, 0x0008,
0x0348, 0x1077,
0x034A, 0x0C37,
0x034C, 0x1070,
0x034E, 0x0C30,
0x0340, 0x0CF2,
0x0342, 0x12F0,
0x0900, 0x0011,
0x0380, 0x0001,
0x0382, 0x0001,
0x0384, 0x0001,
0x0386, 0x0001,
0x0402, 0x1010,
0x0404, 0x1000,
0x0350, 0x0008,
0x0352, 0x0000,
0x0136, 0x1800,
0x013E, 0x0000,
0x0300, 0x0008,
0x0302, 0x0001,
0x0304, 0x0006,
0x0306, 0x00F1,
0x0308, 0x0008,
0x030A, 0x0001,
0x030C, 0x0000,
0x030E, 0x0003,
0x0310, 0x005A,
0x0312, 0x0000,
0x0B06, 0x0101,
0x6028, 0x2000,
0x602A, 0x1FF6,
0x6F12, 0x0000,
0x021E, 0x0000,
0x0202, 0x0100,
0x0204, 0x0020,
//0x0D00, 0x0100, normal mode
//0x0D02, 0x0001,
0x0D00, 0x0101, //tail mode
0x0D02, 0x0101,
0x0114, 0x0301,
0x0D06, 0x0208,
0x0D08, 0x0300,
0x6028, 0x2000,
0x602A, 0x0F10,
0x6F12, 0x0003,
0x602A, 0x0F12,
0x6F12, 0x0200,
0x602A, 0x2BC0,
0x6F12, 0x0001,
0x0B30, 0x0000,
0x0B32, 0x0000,
0x0B34, 0x0001,
};

static void sensor_init(void)
{
	/*Global setting */
	LOG_INF("sensor_init E\n");

	write_cmos_sensor_16_16(0x6028, 0x4000);	 // Page pointer HW
	write_cmos_sensor_16_16(0x0000, 0x0006);	 // Version
	write_cmos_sensor_16_16(0x0000, 0x30D5);
	write_cmos_sensor_16_16(0x6214, 0x7971);	 // Reset
	write_cmos_sensor_16_16(0x6218, 0x7150);	 // Reset
	mdelay(3); // must add 3ms!
	LOG_INF("Before addr_data_pair_globalX\n");
	table_write_cmos_sensor(addr_data_pair_global,
		sizeof(addr_data_pair_global) / sizeof(kal_uint16)); //Global & Functional
	LOG_INF("After addr_data_pair_global\n");

}	/*	sensor_init  */

static void preview_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_preview,
		   sizeof(addr_data_pair_preview) / sizeof(kal_uint16));
}	/*	preview_setting  */

#if 0
/* Pll Setting - VCO = 280Mhz*/
static void capture_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_capture,
		   sizeof(addr_data_pair_capture) / sizeof(kal_uint16));
}

static void normal_video_setting(kal_uint16 currefps)
{

}

static void hs_video_setting(void)
{
	LOG_INF("E\n");
}
#endif

static void slim_video_setting(void)
{
	table_write_cmos_sensor(addr_data_pair_slimvideo,
		sizeof(addr_data_pair_slimvideo) / sizeof(kal_uint16));
}

/*************************************************************************
* FUNCTION
*	get_imgsensor_id
*
* DESCRIPTION
*	This function get the sensor ID
*
* PARAMETERS
*	*sensorID : return the sensor ID
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 get_imgsensor_id(UINT32 *sensor_id)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;

	/*sensor have two i2c address 0x6c 0x6d & 0x21 0x20, we should detect the module used i2c address*/
	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
			*sensor_id = ((read_cmos_sensor_16_8(0x0000) << 8) | read_cmos_sensor_16_8(0x0001)) + 1;
			LOG_INF("read out sensor id 0x%x \n", *sensor_id);
			if (*sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, *sensor_id);
				*sensor_id = S5K3M5SX_SENSOR_ID;
				read_eeprom_CamInfo();
				read_eeprom_SN();
				imgsensor_info.module_id = read_module_id();
				LOG_INF("s5k3m5sx_module_id=%d\n",imgsensor_info.module_id);
				if(deviceInfo_register_value == 0x00){
					register_imgsensor_deviceinfo("Cam_f", DEVICE_VERSION_S5K3M5SX, imgsensor_info.module_id);
					deviceInfo_register_value = 0x01;
				}
				return ERROR_NONE;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
			retry--;
		} while(retry > 0);
		i++;
		retry = 2;
	}
	if (*sensor_id !=  imgsensor_info.sensor_id) {
		/* if Sensor ID is not correct, Must set *sensor_id to 0xFFFFFFFF*/
		*sensor_id = 0xFFFFFFFF;
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 open(void)
{
	kal_uint8 i = 0;
	kal_uint8 retry = 2;
	kal_uint16 sensor_id = 0;
	LOG_INF("S5K3M5SX open start\n");

	LOG_INF("PLATFORM:MT6750,MIPI 4LANE\n");

	while (imgsensor_info.i2c_addr_table[i] != 0xff) {
		spin_lock(&imgsensor_drv_lock);
		imgsensor.i2c_write_id = imgsensor_info.i2c_addr_table[i];
		spin_unlock(&imgsensor_drv_lock);
		do {
				sensor_id = ((read_cmos_sensor_16_8(0x0000) << 8) | read_cmos_sensor_16_8(0x0001)) + 1;
			if (sensor_id == imgsensor_info.sensor_id) {
				LOG_INF("i2c write id: 0x%x, sensor id: 0x%x\n", imgsensor.i2c_write_id, sensor_id);
				break;
			}
			LOG_INF("Read sensor id fail, id: 0x%x\n", imgsensor.i2c_write_id);
			retry--;
		} while(retry > 0);
		i++;
		if (sensor_id == imgsensor_info.sensor_id) {
			break;
		}
		retry = 2;
	}
	if (imgsensor_info.sensor_id !=  sensor_id) {
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	LOG_INF("S5K3M5SX sensor_init start\n");
	/* initail sequence write in  */
	sensor_init();
	LOG_INF("S5K3M5SX sensor_init End\n");
	spin_lock(&imgsensor_drv_lock);

	imgsensor.autoflicker_en= KAL_FALSE;
	imgsensor.sensor_mode = IMGSENSOR_MODE_INIT;
	imgsensor.shutter = 0x3D0;
	imgsensor.gain = 0x100;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.dummy_pixel = 0;
	imgsensor.dummy_line = 0;
	imgsensor.ihdr_mode = 0;
	imgsensor.test_pattern = KAL_FALSE;
	imgsensor.current_fps = imgsensor_info.pre.max_framerate;
	spin_unlock(&imgsensor_drv_lock);
	LOG_INF("S5K3M5SX open End\n");
	return ERROR_NONE;
}	/*	open  */



/*************************************************************************
* FUNCTION
*	close
*
* DESCRIPTION
*
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 close(void)
{
	/* No Need to implement this function */
	streaming_control(KAL_FALSE);
	LOG_INF("close\n");

	return ERROR_NONE;
}	/*	close  */


/*************************************************************************
* FUNCTION
* preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("S5KGD1 preview start\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_PREVIEW;
	imgsensor.pclk = imgsensor_info.pre.pclk;
	imgsensor.line_length = imgsensor_info.pre.linelength;
	imgsensor.frame_length = imgsensor_info.pre.framelength;
	imgsensor.min_frame_length = imgsensor_info.pre.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
//	sensor_init();
	preview_setting();
	set_mirror_flip(imgsensor.mirror);
	LOG_INF("S5KGD1 preview End\n");
	//burst_read_to_check();
	return ERROR_NONE;
}	/*	preview   */

/*************************************************************************
* FUNCTION
*	capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");
	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_CAPTURE;
	if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
	LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n", imgsensor.current_fps, imgsensor_info.cap.max_framerate / 10);
	imgsensor.pclk = imgsensor_info.cap.pclk;
	imgsensor.line_length = imgsensor_info.cap.linelength;
	imgsensor.frame_length = imgsensor_info.cap.framelength;
	imgsensor.min_frame_length = imgsensor_info.cap.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	preview_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/* capture() */
static kal_uint32 normal_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_VIDEO;
	imgsensor.pclk = imgsensor_info.normal_video.pclk;
	imgsensor.line_length = imgsensor_info.normal_video.linelength;
	imgsensor.frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.normal_video.framelength;
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);

	/* normal_video_setting(imgsensor.current_fps);*/
	preview_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	normal_video   */

static kal_uint32 hs_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("Don't use, no setting E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_HIGH_SPEED_VIDEO;
	imgsensor.pclk = imgsensor_info.hs_video.pclk;
	/*imgsensor.video_mode = KAL_TRUE;*/
	imgsensor.line_length = imgsensor_info.hs_video.linelength;
	imgsensor.frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.hs_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/*imgsensor.current_fps = 300;*/
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	preview_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	hs_video   */

static kal_uint32 slim_video(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("E\n");

	spin_lock(&imgsensor_drv_lock);
	imgsensor.sensor_mode = IMGSENSOR_MODE_SLIM_VIDEO;
	imgsensor.pclk = imgsensor_info.slim_video.pclk;
	/*imgsensor.video_mode = KAL_TRUE;*/
	imgsensor.line_length = imgsensor_info.slim_video.linelength;
	imgsensor.frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.min_frame_length = imgsensor_info.slim_video.framelength;
	imgsensor.dummy_line = 0;
	imgsensor.dummy_pixel = 0;
	/*imgsensor.current_fps = 300;*/
	imgsensor.autoflicker_en = KAL_FALSE;
	spin_unlock(&imgsensor_drv_lock);
	slim_video_setting();
	set_mirror_flip(imgsensor.mirror);

	return ERROR_NONE;
}	/*	slim_video	 */



static kal_uint32 get_resolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *sensor_resolution)
{
	LOG_INF("E\n");
	sensor_resolution->SensorFullWidth = imgsensor_info.cap.grabwindow_width;
	sensor_resolution->SensorFullHeight = imgsensor_info.cap.grabwindow_height;

	sensor_resolution->SensorPreviewWidth = imgsensor_info.pre.grabwindow_width;
	sensor_resolution->SensorPreviewHeight = imgsensor_info.pre.grabwindow_height;

	sensor_resolution->SensorVideoWidth = imgsensor_info.normal_video.grabwindow_width;
	sensor_resolution->SensorVideoHeight = imgsensor_info.normal_video.grabwindow_height;


	sensor_resolution->SensorHighSpeedVideoWidth	 = imgsensor_info.hs_video.grabwindow_width;
	sensor_resolution->SensorHighSpeedVideoHeight	 = imgsensor_info.hs_video.grabwindow_height;

	sensor_resolution->SensorSlimVideoWidth	 = imgsensor_info.slim_video.grabwindow_width;
	sensor_resolution->SensorSlimVideoHeight	 = imgsensor_info.slim_video.grabwindow_height;

	return ERROR_NONE;
}	/*	get_resolution	*/

static kal_uint32 get_info(enum MSDK_SCENARIO_ID_ENUM scenario_id,
					  MSDK_SENSOR_INFO_STRUCT *sensor_info,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	sensor_info->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
	sensor_info->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; /* inverse with datasheet*/
	sensor_info->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	sensor_info->SensorInterruptDelayLines = 4; /* not use */
	sensor_info->SensorResetActiveHigh = FALSE; /* not use */
	sensor_info->SensorResetDelayCount = 5; /* not use */

	sensor_info->SensroInterfaceType = imgsensor_info.sensor_interface_type;
	sensor_info->MIPIsensorType = imgsensor_info.mipi_sensor_type;
	sensor_info->SettleDelayMode = imgsensor_info.mipi_settle_delay_mode;
	sensor_info->SensorOutputDataFormat = imgsensor_info.sensor_output_dataformat;

	sensor_info->CaptureDelayFrame = imgsensor_info.cap_delay_frame;
	sensor_info->PreviewDelayFrame = imgsensor_info.pre_delay_frame;
	sensor_info->VideoDelayFrame = imgsensor_info.video_delay_frame;
	sensor_info->HighSpeedVideoDelayFrame = imgsensor_info.hs_video_delay_frame;
	sensor_info->SlimVideoDelayFrame = imgsensor_info.slim_video_delay_frame;

	sensor_info->SensorMasterClockSwitch = 0; /* not use */
	sensor_info->SensorDrivingCurrent = imgsensor_info.isp_driving_current;

	sensor_info->AEShutDelayFrame = imgsensor_info.ae_shut_delay_frame; 	 /* The frame of setting shutter default 0 for TG int */
	sensor_info->AESensorGainDelayFrame = imgsensor_info.ae_sensor_gain_delay_frame;	/* The frame of setting sensor gain */
	sensor_info->AEISPGainDelayFrame = imgsensor_info.ae_ispGain_delay_frame;
	sensor_info->IHDR_Support = imgsensor_info.ihdr_support;
	sensor_info->IHDR_LE_FirstLine = imgsensor_info.ihdr_le_firstline;
	sensor_info->SensorModeNum = imgsensor_info.sensor_mode_num;
	sensor_info->PDAF_Support = 2;
	sensor_info->SensorMIPILaneNumber = imgsensor_info.mipi_lane_num;
	sensor_info->SensorClockFreq = imgsensor_info.mclk;
	sensor_info->SensorClockDividCount = 3; /* not use */
	sensor_info->SensorClockRisingCount = 0;
	sensor_info->SensorClockFallingCount = 2; /* not use */
	sensor_info->SensorPixelClockCount = 3; /* not use */
	sensor_info->SensorDataLatchCount = 2; /* not use */

	sensor_info->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	sensor_info->SensorWidthSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorHightSampling = 0;	/* 0 is default 1x*/
	sensor_info->SensorPacketECCOrder = 1;
	//sensor_info->sensorSecureType = 2;

	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			sensor_info->SensorGrabStartX = imgsensor_info.cap.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.cap.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.cap.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:

			sensor_info->SensorGrabStartX = imgsensor_info.normal_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.normal_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.normal_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.hs_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.hs_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.hs_video.mipi_data_lp2hs_settle_dc;

			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			sensor_info->SensorGrabStartX = imgsensor_info.slim_video.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.slim_video.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.slim_video.mipi_data_lp2hs_settle_dc;

			break;
		default:
			sensor_info->SensorGrabStartX = imgsensor_info.pre.startx;
			sensor_info->SensorGrabStartY = imgsensor_info.pre.starty;

			sensor_info->MIPIDataLowPwr2HighSpeedSettleDelayCount = imgsensor_info.pre.mipi_data_lp2hs_settle_dc;
			break;
	}

	return ERROR_NONE;
}	/*	get_info  */


static kal_uint32 control(enum MSDK_SCENARIO_ID_ENUM scenario_id, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	LOG_INF("scenario_id = %d\n", scenario_id);
	spin_lock(&imgsensor_drv_lock);
	imgsensor.current_scenario_id = scenario_id;
	spin_unlock(&imgsensor_drv_lock);
	switch (scenario_id) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			preview(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			capture(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			normal_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			hs_video(image_window, sensor_config_data);
			break;
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			slim_video(image_window, sensor_config_data);
			break;
		default:
			LOG_INF("Error ScenarioId setting");
			preview(image_window, sensor_config_data);
			return ERROR_INVALID_SCENARIO_ID;
	}

	return ERROR_NONE;
}	/* control() */



static kal_uint32 set_video_mode(UINT16 framerate)
{
	LOG_INF("framerate = %d\n ", framerate);
	/* SetVideoMode Function should fix framerate*/
	if (framerate == 0) {
		/* Dynamic frame rate*/
		return ERROR_NONE;
	}
	spin_lock(&imgsensor_drv_lock);
	if ((framerate == 300) && (imgsensor.autoflicker_en == KAL_TRUE)) {
		imgsensor.current_fps = 296;
	} else if ((framerate == 150) && (imgsensor.autoflicker_en == KAL_TRUE)) {
		imgsensor.current_fps = 146;
	} else {
		imgsensor.current_fps = framerate;
	}
	spin_unlock(&imgsensor_drv_lock);
	set_max_framerate(imgsensor.current_fps, 1);

	return ERROR_NONE;
}

static kal_uint32 set_auto_flicker_mode(kal_bool enable, UINT16 framerate)
{
	LOG_INF("enable = %d, framerate = %d \n", enable, framerate);
	spin_lock(&imgsensor_drv_lock);
	if (enable) {/*enable auto flicker*/
		imgsensor.autoflicker_en = KAL_TRUE;
	} else {/*Cancel Auto flick*/
		imgsensor.autoflicker_en = KAL_FALSE;
	}
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}


static kal_uint32 set_max_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 framerate)
{
	kal_uint32 frame_length;

	LOG_INF("scenario_id = %d, framerate = %d\n", scenario_id, framerate);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		if(framerate == 0)
			return ERROR_NONE;
		frame_length = imgsensor_info.normal_video.pclk / framerate * 10 / imgsensor_info.normal_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.normal_video.framelength) ? (frame_length - imgsensor_info.normal_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.normal_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		if (imgsensor.current_fps != imgsensor_info.cap.max_framerate)
		LOG_INF("Warning: current_fps %d fps is not support, so use cap's setting: %d fps!\n", framerate,imgsensor_info.cap.max_framerate / 10);
		frame_length = imgsensor_info.cap.pclk / framerate * 10 / imgsensor_info.cap.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.cap.framelength) ? (frame_length - imgsensor_info.cap.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.cap.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
	    set_dummy();
	    break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		frame_length = imgsensor_info.hs_video.pclk / framerate * 10 / imgsensor_info.hs_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.hs_video.framelength) ? (frame_length - imgsensor_info.hs_video.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.hs_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		frame_length = imgsensor_info.slim_video.pclk / framerate * 10 / imgsensor_info.slim_video.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.slim_video.framelength) ? (frame_length - imgsensor_info.slim_video.framelength): 0;
		imgsensor.frame_length = imgsensor_info.slim_video.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		break;
	default:  /*coding with  preview scenario by default*/
		frame_length = imgsensor_info.pre.pclk / framerate * 10 / imgsensor_info.pre.linelength;
		spin_lock(&imgsensor_drv_lock);
		imgsensor.dummy_line = (frame_length > imgsensor_info.pre.framelength) ? (frame_length - imgsensor_info.pre.framelength) : 0;
		imgsensor.frame_length = imgsensor_info.pre.framelength + imgsensor.dummy_line;
		imgsensor.min_frame_length = imgsensor.frame_length;
		spin_unlock(&imgsensor_drv_lock);
		set_dummy();
		LOG_INF("error scenario_id = %d, we use preview scenario \n", scenario_id);
		break;
	}
	return ERROR_NONE;
}


static kal_uint32 get_default_framerate_by_scenario(enum MSDK_SCENARIO_ID_ENUM scenario_id, MUINT32 *framerate)
{
	LOG_INF("scenario_id = %d\n", scenario_id);

	switch (scenario_id) {
	case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		*framerate = imgsensor_info.pre.max_framerate;
		break;
	case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		*framerate = imgsensor_info.normal_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		*framerate = imgsensor_info.cap.max_framerate;
		break;
	case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		*framerate = imgsensor_info.hs_video.max_framerate;
		break;
	case MSDK_SCENARIO_ID_SLIM_VIDEO:
		*framerate = imgsensor_info.slim_video.max_framerate;
		break;
	default:
		break;
	}

	return ERROR_NONE;
}

static kal_uint32 set_test_pattern_mode(kal_bool enable)
{
	LOG_INF("enable: %d\n", enable);

	if (enable) {
		/* 0x5E00[8]: 1 enable,  0 disable*/
		/* 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK*/
		write_cmos_sensor_16_16(0x0600, 0x0002);
	} else {
		/* 0x5E00[8]: 1 enable,  0 disable*/
		/* 0x5E00[1:0]; 00 Color bar, 01 Random Data, 10 Square, 11 BLACK*/
		write_cmos_sensor_16_16(0x0600, 0x0000);
	}
	spin_lock(&imgsensor_drv_lock);
	imgsensor.test_pattern = enable;
	spin_unlock(&imgsensor_drv_lock);
	return ERROR_NONE;
}
static kal_uint32 s5k3m5sx_awb_gain(struct SET_SENSOR_AWB_GAIN *pSetSensorAWB)
{
	LOG_INF("s5k3m5sx_awb_gain: 0x100\n");

	write_cmos_sensor_16_16(0x0D82, 0x100);
	write_cmos_sensor_16_16(0x0D84, 0x100);
	write_cmos_sensor_16_16(0x0D86, 0x100);
	/*
	LOG_INF("gd1 [%s] read 0x0D82:0x%x, 0x0D84:0x%x, 0x0D86:0x%x\n",
		__func__,
		read_cmos_sensor_16_16(0x0D82),
		read_cmos_sensor_16_16(0x0D84),
		read_cmos_sensor_16_16(0x0D86));
	*/
	return ERROR_NONE;
}

/*write AWB gain to sensor*/
static void feedback_awbgain(kal_uint32 r_gain, kal_uint32 b_gain)
{
	UINT32 r_gain_int = 0;
	UINT32 b_gain_int = 0;

	r_gain_int = r_gain / 2;
	b_gain_int = b_gain / 2;

	/*write r_gain*/
	write_cmos_sensor_16_16(0x0D82, r_gain_int);
	/*write _gain*/
	write_cmos_sensor_16_16(0x0D86, b_gain_int);
	/*
	LOG_INF("gd1 [%s] write r_gain:%d, r_gain_int:%d, b_gain:%d, b_gain_int:%d\n",
		__func__, r_gain, r_gain_int, b_gain, b_gain_int);

	LOG_INF("gd1 [%s] read 0x0D82:0x%x, 0x0D84:0x%x, 0x0D86:0x%x\n",
		__func__, read_cmos_sensor_16_16(0x0D82),
		read_cmos_sensor_16_16(0x0D84), read_cmos_sensor_16_16(0x0D86));
	*/
}

static kal_uint32 feature_control(MSDK_SENSOR_FEATURE_ENUM feature_id,
							 UINT8 *feature_para,UINT32 *feature_para_len)
{
	UINT16 *feature_return_para_16= (UINT16 *) feature_para;
	UINT16 *feature_data_16= (UINT16 *) feature_para;
	UINT32 *feature_return_para_32= (UINT32 *) feature_para;
	UINT32 *feature_data_32= (UINT32 *) feature_para;
	unsigned long long *feature_data= (unsigned long long *) feature_para;

	struct SET_PD_BLOCK_INFO_T *PDAFinfo;
	struct SENSOR_WINSIZE_INFO_STRUCT *wininfo;
	struct SENSOR_VC_INFO_STRUCT *pvcinfo;

	MSDK_SENSOR_REG_INFO_STRUCT *sensor_reg_data= (MSDK_SENSOR_REG_INFO_STRUCT *) feature_para;

	LOG_INF("feature_id = %d\n", feature_id);
	switch (feature_id) {
	#ifdef VENDOR_EDIT
	/*Henry.Chang@Camera.Driver add for google ARCode Feature verify 20190531*/
	case SENSOR_FEATURE_GET_MODULE_INFO:
		LOG_INF("s5k3m5sx GET_MODULE_CamInfo:%d %d\n", *feature_para_len, *feature_data_32);
		*(feature_data_32 + 1) = (gS5k3m5sx_CamInfo[1] << 24)
					| (gS5k3m5sx_CamInfo[0] << 16)
					| (gS5k3m5sx_CamInfo[3] << 8)
					| (gS5k3m5sx_CamInfo[2] & 0xFF);
		*(feature_data_32 + 2) = (gS5k3m5sx_CamInfo[5] << 24)
					| (gS5k3m5sx_CamInfo[4] << 16)
					| (gS5k3m5sx_CamInfo[7] << 8)
					| (gS5k3m5sx_CamInfo[6] & 0xFF);
		break;
	/*Henry.Chang@Camera.Driver add for 18531 ModuleSN*/
	case SENSOR_FEATURE_GET_MODULE_SN:
		LOG_INF("s5k3m5sx GET_MODULE_SN:%d %d\n", *feature_para_len, *feature_data_32);
		if (*feature_data_32 < CAMERA_MODULE_SN_LENGTH/4)
			*(feature_data_32 + 1) = (gS5k3m5sx_SN[4*(*feature_data_32) + 3] << 24)
						| (gS5k3m5sx_SN[4*(*feature_data_32) + 2] << 16)
						| (gS5k3m5sx_SN[4*(*feature_data_32) + 1] << 8)
						| (gS5k3m5sx_SN[4*(*feature_data_32)] & 0xFF);
		break;
	/*Henry.Chang@camera.driver 20181129, add for sensor Module SET*/
	case SENSOR_FEATURE_SET_SENSOR_OTP:
	{
		kal_int32 ret = IMGSENSOR_RETURN_SUCCESS;
		LOG_INF("SENSOR_FEATURE_SET_SENSOR_OTP length :%d\n", (UINT32)*feature_para_len);
		ret = write_Module_data((ACDK_SENSOR_ENGMODE_STEREO_STRUCT *)(feature_para));
		if (ret == ERROR_NONE)
			return ERROR_NONE;
		else
			return ERROR_MSDK_IS_ACTIVED;
	}
	#endif
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ_BY_SCENARIO:
        switch (*feature_data) {
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = imgsensor_info.cap.pclk;
                break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = imgsensor_info.normal_video.pclk;
                break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = imgsensor_info.hs_video.pclk;
                break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = imgsensor_info.slim_video.pclk;
                break;
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        default:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = imgsensor_info.pre.pclk;
                break;
        }
        break;
	case SENSOR_FEATURE_GET_PERIOD:
		*feature_return_para_16++ = imgsensor.line_length;
		*feature_return_para_16 = imgsensor.frame_length;
		*feature_para_len= 4;
		break;
	case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
		*feature_return_para_32 = imgsensor.pclk;
		*feature_para_len= 4;
		break;
	case SENSOR_FEATURE_SET_ESHUTTER:
	             set_shutter(*feature_data);
		break;
	case SENSOR_FEATURE_SET_NIGHTMODE:
		/*night_mode((BOOL) *feature_data);*/
		break;
	case SENSOR_FEATURE_SET_GAIN:
		set_gain((UINT16) *feature_data);
		break;
	case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
	case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
		break;
	case SENSOR_FEATURE_SET_REGISTER:
		write_cmos_sensor_16_16(sensor_reg_data->RegAddr, sensor_reg_data->RegData);
		break;
	case SENSOR_FEATURE_GET_REGISTER:
		sensor_reg_data->RegData = read_cmos_sensor_16_16(sensor_reg_data->RegAddr);
		break;
	case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/* get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE*/
		/* if EEPROM does not exist in camera module.*/
		*feature_return_para_32= LENS_DRIVER_ID_DO_NOT_CARE;
		*feature_para_len= 4;
		break;
	case SENSOR_FEATURE_SET_VIDEO_MODE:
		set_video_mode(*feature_data);
		break;
	case SENSOR_FEATURE_CHECK_SENSOR_ID:
		get_imgsensor_id(feature_return_para_32);
		break;
	#ifdef VENDOR_EDIT
	/*Caohua.Lin@CAmera, modify for different module 20180723*/
	case SENSOR_FEATURE_CHECK_MODULE_ID:
		*feature_return_para_32 = imgsensor_info.module_id;
		break;
	/*Caohua.Lin@Camera.Driver , 20190318, add for ITS--sensor_fusion*/
	case SENSOR_FEATURE_GET_OFFSET_TO_START_OF_EXPOSURE:
		*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 8000000;
		break;
	#endif
	case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
		set_auto_flicker_mode((BOOL)*feature_data_16,*(feature_data_16+1));
		break;
	case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
		set_max_framerate_by_scenario(
			   (enum MSDK_SCENARIO_ID_ENUM)*feature_data,
			   *(feature_data+1));
		break;
	case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
		get_default_framerate_by_scenario((enum MSDK_SCENARIO_ID_ENUM)*(feature_data),
			(MUINT32 *)(uintptr_t)(*(feature_data + 1)));
		break;
	/*case SENSOR_FEATURE_GET_PDAF_DATA:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_DATA\n");
		read_2L9_eeprom((kal_uint16 )(*feature_data),(char*)(uintptr_t)(*(feature_data+1)),(kal_uint32)(*(feature_data+2)));
		break;*/
	case SENSOR_FEATURE_SET_TEST_PATTERN:
		set_test_pattern_mode((BOOL)*feature_data);
		break;
	case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE: /*for factory mode auto testing*/
		*feature_return_para_32 = imgsensor_info.checksum_value;
		*feature_para_len= 4;
		break;
	case SENSOR_FEATURE_SET_FRAMERATE:
		spin_lock(&imgsensor_drv_lock);
		imgsensor.current_fps = *feature_data_32;
		spin_unlock(&imgsensor_drv_lock);
		break;
    case SENSOR_FEATURE_GET_PERIOD_BY_SCENARIO:
        switch (*feature_data) {
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = (imgsensor_info.cap.framelength << 16)
                                 + imgsensor_info.cap.linelength;
                break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = (imgsensor_info.normal_video.framelength << 16)
                                + imgsensor_info.normal_video.linelength;
                break;
        case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = (imgsensor_info.hs_video.framelength << 16)
                                 + imgsensor_info.hs_video.linelength;
                break;
        case MSDK_SCENARIO_ID_SLIM_VIDEO:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = (imgsensor_info.slim_video.framelength << 16)
                                 + imgsensor_info.slim_video.linelength;
                break;
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        default:
                *(MUINT32 *)(uintptr_t)(*(feature_data + 1))
                        = (imgsensor_info.pre.framelength << 16)
                                 + imgsensor_info.pre.linelength;
                break;
        }
        break;
	case SENSOR_FEATURE_GET_CROP_INFO:
		LOG_INF("SENSOR_FEATURE_GET_CROP_INFO scenarioId:%d\n", (UINT32)*feature_data_32);
		wininfo = (struct SENSOR_WINSIZE_INFO_STRUCT *)(uintptr_t)(*(feature_data + 1));

		switch (*feature_data_32) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[1],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[2],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[3],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[4],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
				break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			default:
				memcpy((void *)wininfo,(void *)&imgsensor_winsize_info[0],sizeof(struct SENSOR_WINSIZE_INFO_STRUCT));
				break;
		}
		break;
	case SENSOR_FEATURE_GET_PDAF_INFO:
		LOG_INF("SENSOR_FEATURE_GET_PDAF_INFO scenarioId:%d\n", (UINT16)*feature_data);
		PDAFinfo= (struct SET_PD_BLOCK_INFO_T *)(uintptr_t)(*(feature_data + 1));

		switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
				memcpy((void *)PDAFinfo,(void *)&imgsensor_pd_info,sizeof(struct SET_PD_BLOCK_INFO_T));
				break;
			default:
				break;
		}
		break;
	case SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY:
		LOG_INF("SENSOR_FEATURE_GET_SENSOR_PDAF_CAPACITY scenarioId:%d\n", (UINT16)*feature_data);
		/*PDAF capacity enable or not, 2p8 only full size support PDAF*/
		switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 1;
				break;
			default:
				*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = 0;
				break;
		}
		break;
	case SENSOR_FEATURE_SET_SHUTTER_FRAME_TIME:
		set_shutter_frame_length((UINT16) *feature_data, (UINT16) *(feature_data + 1));
		break;
	case SENSOR_FEATURE_SET_STREAMING_SUSPEND:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_SUSPEND\n");
		streaming_control(KAL_FALSE);
		break;
	case SENSOR_FEATURE_SET_STREAMING_RESUME:
		LOG_INF("SENSOR_FEATURE_SET_STREAMING_RESUME, shutter:%llu\n", *feature_data);
		if (*feature_data != 0)
			set_shutter(*feature_data);
		streaming_control(KAL_TRUE);
		break;
	case SENSOR_FEATURE_GET_MIPI_PIXEL_RATE:
		{
			kal_uint32 rate;

			switch (*feature_data) {
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
					rate = imgsensor_info.cap.mipi_pixel_rate;
					break;
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					rate = imgsensor_info.normal_video.mipi_pixel_rate;
					break;
			case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
					rate = imgsensor_info.hs_video.mipi_pixel_rate;
					break;
			case MSDK_SCENARIO_ID_SLIM_VIDEO:
					rate = imgsensor_info.slim_video.mipi_pixel_rate;
					break;
			case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
					rate = imgsensor_info.pre.mipi_pixel_rate;
					break;
			default:
					rate = 0;
					break;
			}
			*(MUINT32 *)(uintptr_t)(*(feature_data + 1)) = rate;
		}
		break;
	case SENSOR_FEATURE_GET_VC_INFO:
		LOG_INF("SENSOR_FEATURE_GET_VC_INFO %d\n",
			(UINT16)*feature_data);
		pvcinfo =
		 (struct SENSOR_VC_INFO_STRUCT *)(uintptr_t)(*(feature_data+1));
		switch (*feature_data_32) {
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[1],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		default:
			memcpy((void *)pvcinfo, (void *)&SENSOR_VC_INFO[0],
				sizeof(struct SENSOR_VC_INFO_STRUCT));
			break;
		}
	break;
	case SENSOR_FEATURE_GET_BINNING_TYPE:
		switch (*(feature_data + 1)) {
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
			*feature_return_para_32 = 1; /*BINNING_NONE*/
			break;
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_HIGH_SPEED_VIDEO:
		case MSDK_SCENARIO_ID_SLIM_VIDEO:
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		default:
			*feature_return_para_32 = 2; /*BINNING_AVERAGED*/
			break;
		}

		LOG_INF("SENSOR_FEATURE_GET_BINNING_TYPE AE_binning_type:%d\n",
			(UINT32)*feature_return_para_32);
		break;
	case SENSOR_FEATURE_SET_AWB_GAIN:
		/* modify to separate 3hdr and remosaic */
		if (imgsensor.sensor_mode == IMGSENSOR_MODE_CAPTURE) {
			/*write AWB gain to sensor*/
			feedback_awbgain((UINT32)*(feature_data_32 + 1),
					(UINT32)*(feature_data_32 + 2));
		} else {
			s5k3m5sx_awb_gain(
				(struct SET_SENSOR_AWB_GAIN *) feature_para);
		}
		break;
	case SENSOR_FEATURE_GET_4CELL_DATA:
		{
			int type = (kal_uint16)(*feature_data);
			if (type == FOUR_CELL_CAL_TYPE_ALL) {
				LOG_INF("SENSOR_FEATURE_GET_4CELL_DATA type=%d\n", type);
			} else if (type == FOUR_CELL_CAL_TYPE_GAIN_TBL) {
				LOG_INF("SENSOR_FEATURE_GET_4CELL_DATA type=%d\n", type);
			} else {
				memset((void *)(*(feature_data+1)), 0, 4);
				LOG_INF("No type %d buffer on this sensor\n", type);
			}
			break;
		}
	default:
		break;
	}

	return ERROR_NONE;
}	/*	feature_control()  */

static struct SENSOR_FUNCTION_STRUCT sensor_func = {
	open,
	get_info,
	get_resolution,
	feature_control,
	control,
	close
};

UINT32 S5K3M5SX_MIPI_RAW_SensorInit(struct SENSOR_FUNCTION_STRUCT **pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!= NULL) {
		*pfFunc= &sensor_func;
	}
	return ERROR_NONE;
}	/*	S5K3M5SX_MIPI_RAW_SensorInit	*/
