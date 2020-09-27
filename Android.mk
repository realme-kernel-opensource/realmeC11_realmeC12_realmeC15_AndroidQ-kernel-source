# Copyright (C) 2017 MediaTek Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See http://www.gnu.org/licenses/gpl-2.0.html for more details.

LOCAL_PATH := $(call my-dir)

ifeq ($(notdir $(LOCAL_PATH)),$(strip $(LINUX_KERNEL_VERSION)))
ifneq ($(strip $(TARGET_NO_KERNEL)),true)
include $(LOCAL_PATH)/kenv.mk

#ifdef VENDOR_EDIT
#Yongpei.Yao@PSW.MM.Audio.Machine, 2019/11/4, add for 19551 fm config
ifneq ($(filter oppo6779_18073 oppo6779_18593, $(OPPO_TARGET_DEVICE)),)
$(shell sed -i 's/CONFIG_MTK_FM_CHIP=*.*/CONFIG_MTK_FM_CHIP=\"MT6635_FM\"/g' $(KERNEL_CONFIG_FILE))
else
$(shell sed -i 's/CONFIG_MTK_FM_CHIP=*.*/CONFIG_MTK_FM_CHIP=\"MT6631_FM\"/g' $(KERNEL_CONFIG_FILE))
endif
#endif VENDOR_EDIT

#ifdef VENDOR_EDIT
#Zhijun.ye@PSW.MM.Display.LCD.Stability, 2019/11/16, add for 19301 lcm config
ifneq ($(filter oppo6779_18073 oppo6779_18593 oppo6779_19301 oppo6779_19011, $(OPPO_TARGET_DEVICE)),)
$(shell sed -i 's/CONFIG_LCM_HEIGHT=*.*/CONFIG_LCM_HEIGHT=\"2340\"/g' $(KERNEL_CONFIG_FILE))
else
#ifdef ODM_WT_EDIT
#Wu.weihong@ODM_WT.NW.Config, 2020/03/28, LCD bringup, sync kernel oppo6765_defconfig CONFIG_LCM_HEIGHT
$(shell sed -i 's/CONFIG_LCM_HEIGHT=*.*/CONFIG_LCM_HEIGHT=\"1600\"/g' $(KERNEL_CONFIG_FILE))
#endif
endif
#endif VENDOR_EDIT

#ifdef VENDOR_EDIT
#oujinrong@BSP.Fingerprint.Secure, 2019/12/25, add for widevine compatible
ifneq ($(filter oppo6779_19551 oppo6779_18593 oppo6779_19301, $(OPPO_TARGET_DEVICE)),)
$(shell sed -i 's/CONFIG_MTK_DRM_KEY_MNG_SUPPORT=.*/CONFIG_MTK_DRM_KEY_MNG_SUPPORT=y/g' $(KERNEL_CONFIG_FILE))
$(shell sed -i 's/CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT=.*/CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT=y/g' $(KERNEL_CONFIG_FILE))
else
$(shell sed -i 's/CONFIG_MTK_DRM_KEY_MNG_SUPPORT=.*//g' $(KERNEL_CONFIG_FILE))
$(shell sed -i 's/CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT=.*//g' $(KERNEL_CONFIG_FILE))
endif
#endif VENDOR_EDIT

#ifdef VENDOR_EDIT
#hongxiang.jin@PSW.MM.Audio.Machine, 2019/11/12, add for 18073 soundtrigger config
ifneq ($(filter oppo6779_18073, $(OPPO_TARGET_DEVICE)),)
$(shell sed -i 's/# CONFIG_SND_SOC_DBMDX is not set/CONFIG_SND_SOC_DBMDX=y/g' $(KERNEL_CONFIG_FILE))
else
$(shell sed -i 's/CONFIG_SND_SOC_DBMDX=y/# CONFIG_SND_SOC_DBMDX is not set/g' $(KERNEL_CONFIG_FILE))
endif
#endif VENDOR_EDIT

#ifdef VENDOR_EDIT
#weiriqin@camera,driver, 2019/11/18, add for 18073 camera config
ifneq ($(filter oppo6779_18073 oppo6779_18593, $(OPPO_TARGET_DEVICE)),)
$(shell sed -i 's/CONFIG_CUSTOM_KERNEL_IMGSENSOR=*.*/CONFIG_CUSTOM_KERNEL_IMGSENSOR=\"imx586p2q_mipi_raw s5kgd1sp_mipi_raw gc5035_mipi_raw\"/g' $(KERNEL_CONFIG_FILE))
endif
#endif

#ifdef VENDOR_EDIT
#weiriqin@camera,driver, 2019/11/18, add for 19301 camera config
ifneq ($(filter oppo6779_19301 oppo6779_19011, $(OPPO_TARGET_DEVICE)),)
$(shell sed -i 's/CONFIG_CUSTOM_KERNEL_IMGSENSOR=*.*/CONFIG_CUSTOM_KERNEL_IMGSENSOR=\"imx586p2q2_mipi_raw s5k3p9sp_mipi_raw hi846p2q_mipi_raw ov02a10p2q_mipi_mono gc02m0p2q_mipi_mono ov02a10p2q_mipi_mono1 gc02m0p2q_mipi_mono1\"/g' $(KERNEL_CONFIG_FILE))
endif
#endif

#ifdef VENDOR_EDIT
#ZhengQiong@camera,driver, 2019/12/27, add for 19551 camera config
ifneq ($(filter oppo6779_19551, $(OPPO_TARGET_DEVICE)),)
$(shell sed -i 's/CONFIG_CUSTOM_KERNEL_IMGSENSOR=*.*/CONFIG_CUSTOM_KERNEL_IMGSENSOR=\"imx586_mipi_raw s5kgw1_mipi_raw s5kgh1_mipi_raw s5k3m5_mipi_raw gc8054_mipi_raw gc02m0_mipi_mono gc02m0f_mipi_mono\"/g' $(KERNEL_CONFIG_FILE))
endif
#endif

ifeq ($(wildcard $(TARGET_PREBUILT_KERNEL)),)
KERNEL_MAKE_DEPENDENCIES := $(shell find $(KERNEL_DIR) -name .git -prune -o -type f | sort)
KERNEL_MAKE_DEPENDENCIES := $(filter-out %/.git %/.gitignore %/.gitattributes,$(KERNEL_MAKE_DEPENDENCIES))

$(TARGET_KERNEL_CONFIG): PRIVATE_DIR := $(KERNEL_DIR)
$(TARGET_KERNEL_CONFIG): $(KERNEL_CONFIG_FILE) $(LOCAL_PATH)/Android.mk
$(TARGET_KERNEL_CONFIG): $(KERNEL_MAKE_DEPENDENCIES)
	$(hide) mkdir -p $(dir $@)
	$(PREBUILT_MAKE_PREFIX)$(MAKE) -C $(PRIVATE_DIR) $(KERNEL_MAKE_OPTION) $(KERNEL_DEFCONFIG)

$(BUILT_DTB_OVERLAY_TARGET): $(KERNEL_ZIMAGE_OUT)

.KATI_RESTAT: $(KERNEL_ZIMAGE_OUT)
$(KERNEL_ZIMAGE_OUT): PRIVATE_DIR := $(KERNEL_DIR)
$(KERNEL_ZIMAGE_OUT): $(TARGET_KERNEL_CONFIG) $(KERNEL_MAKE_DEPENDENCIES)
	$(hide) mkdir -p $(dir $@)
	$(PREBUILT_MAKE_PREFIX)$(MAKE) -C $(PRIVATE_DIR) $(KERNEL_MAKE_OPTION)
	$(hide) $(call fixup-kernel-cmd-file,$(KERNEL_OUT)/arch/$(KERNEL_TARGET_ARCH)/boot/compressed/.piggy.xzkern.cmd)
	# check the kernel image size
	python device/mediatek/build/build/tools/check_kernel_size.py $(KERNEL_OUT) $(KERNEL_DIR)
#ifdef VENDOR_EDIT
#Ke.Li@Rom.Security, KernelHotFix, 2019.10.15 Add for HotFix of kernel
	$(KERNEL_DIR)/tools/toSDK.sh
#endif /* VENDOR_EDIT */
ifeq ($(strip $(MTK_HEADER_SUPPORT)), yes)
$(BUILT_KERNEL_TARGET): $(KERNEL_ZIMAGE_OUT) $(TARGET_KERNEL_CONFIG) $(LOCAL_PATH)/Android.mk | $(HOST_OUT_EXECUTABLES)/mkimage$(HOST_EXECUTABLE_SUFFIX)
	$(hide) $(HOST_OUT_EXECUTABLES)/mkimage$(HOST_EXECUTABLE_SUFFIX) $< KERNEL 0xffffffff > $@
else
$(BUILT_KERNEL_TARGET): $(KERNEL_ZIMAGE_OUT) $(TARGET_KERNEL_CONFIG) $(LOCAL_PATH)/Android.mk | $(ACP)
	$(copy-file-to-target)
endif

$(TARGET_PREBUILT_KERNEL): $(BUILT_KERNEL_TARGET) $(LOCAL_PATH)/Android.mk | $(ACP)
	$(copy-file-to-new-target)

endif#TARGET_PREBUILT_KERNEL is empty

$(INSTALLED_KERNEL_TARGET): $(BUILT_KERNEL_TARGET) $(LOCAL_PATH)/Android.mk | $(ACP)
	$(copy-file-to-target)

.PHONY: kernel save-kernel kernel-savedefconfig kernel-menuconfig menuconfig-kernel savedefconfig-kernel clean-kernel
kernel: $(INSTALLED_KERNEL_TARGET)
save-kernel: $(TARGET_PREBUILT_KERNEL)

kernel-savedefconfig: $(TARGET_KERNEL_CONFIG)
	cp $(TARGET_KERNEL_CONFIG) $(KERNEL_CONFIG_FILE)

kernel-menuconfig:
	$(hide) mkdir -p $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) $(KERNEL_MAKE_OPTION) menuconfig

menuconfig-kernel savedefconfig-kernel:
	$(hide) mkdir -p $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) $(KERNEL_MAKE_OPTION) $(patsubst %config-kernel,%config,$@)

clean-kernel:
	$(hide) rm -rf $(KERNEL_OUT) $(KERNEL_MODULES_OUT) $(INSTALLED_KERNEL_TARGET)
	$(hide) rm -f $(INSTALLED_DTB_OVERLAY_TARGET)


.PHONY: check-kernel-config check-kernel-dotconfig check-mtk-config
droid: check-kernel-config check-kernel-dotconfig
check-mtk-config: check-kernel-config check-kernel-dotconfig
check-kernel-config: PRIVATE_COMMAND := $(if $(wildcard device/mediatek/build/build/tools/check_kernel_config.py),$(if $(filter yes,$(DISABLE_MTK_CONFIG_CHECK)),-)python device/mediatek/build/build/tools/check_kernel_config.py -c $(MTK_TARGET_PROJECT_FOLDER)/ProjectConfig.mk -k $(KERNEL_CONFIG_FILE) -p $(MTK_PROJECT_NAME))
check-kernel-config:
	$(PRIVATE_COMMAND)

check-kernel-dotconfig: PRIVATE_COMMAND := $(if $(wildcard device/mediatek/build/build/tools/check_kernel_config.py),$(if $(filter yes,$(DISABLE_MTK_CONFIG_CHECK)),-)python device/mediatek/build/build/tools/check_kernel_config.py -c $(MTK_TARGET_PROJECT_FOLDER)/ProjectConfig.mk -k $(TARGET_KERNEL_CONFIG) -p $(MTK_PROJECT_NAME))
check-kernel-dotconfig: $(TARGET_KERNEL_CONFIG)
	$(PRIVATE_COMMAND)

### DTB
ifdef BOARD_PREBUILT_DTBIMAGE_DIR
INSTALLED_MTK_DTB_TARGET := $(BOARD_PREBUILT_DTBIMAGE_DIR)/mtk_dtb
$(shell if [ ! -f $(INSTALLED_MTK_DTB_TARGET) ]; then mkdir -p $(dir $(INSTALLED_MTK_DTB_TARGET)); touch $(INSTALLED_MTK_DTB_TARGET);fi)
$(INSTALLED_MTK_DTB_TARGET): $(INSTALLED_KERNEL_TARGET)
	@mkdir -p $(dir $@)
	@cp -f $(KERNEL_DTB_FILE) $@
endif

endif#TARGET_NO_KERNEL
endif#LINUX_KERNEL_VERSION
