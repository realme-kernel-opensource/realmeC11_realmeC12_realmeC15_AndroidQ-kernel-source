subdir-ccflags-y += -Werror
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
# In case the platform does NOT support this type of sensors
obj-y   +=  mtk_tpd.o
obj-y   +=  tpd_button.o
obj-y   +=  tpd_calibrate.o
obj-y   +=  tpd_debug.o
obj-y   +=  tpd_default.o
obj-y   +=  tpd_misc.o
obj-y   +=  tpd_setting.o
#ifdef ODM_WT_EDIT
# Yize.Zhang@ODM_WT.BSP.Touchscreen.Base,2020/03/27,bringup add for xl/hlt/txd touchscreen module.
obj-y   += tp_usb_notifier.o
obj-y   += update_tpfw_notifier.o
obj-y   += headset_notifier.o
obj-$(CONFIG_TOUCHSCREEN_HIMAX_CHIPSET)	+= hxchipset/
obj-$(CONFIG_TOUCHSCREEN_ILI9881H)	 += ili9881h/
obj-$(CONFIG_TOUCHSCREEN_NT36xxx_HOSTDL_SPI)	 += NT36525B/
#endif
obj-$(CONFIG_TOUCHSCREEN_MTK_SOLOMON)	+=  SOLOMON/
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)	+=  focaltech_touch/
#obj-$(CONFIG_TOUCHSCREEN_MTK_GT1151)	+=  GT1151/
obj-$(CONFIG_TOUCHSCREEN_MTK_GT911)	+=  GT911/
obj-$(CONFIG_TOUCHSCREEN_MTK_GSLX680)	+=  gslX680/
obj-$(CONFIG_TOUCHSCREEN_MTK_NT36xxx)	+=  NT36xxx/
obj-$(CONFIG_TOUCHSCREEN_MTK_NT36572_COMMON)   +=  nt36572_common/
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_HD)	+=  focaltech_hd_touch/
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_FHD)	+=  focaltech_fhd_touch/
