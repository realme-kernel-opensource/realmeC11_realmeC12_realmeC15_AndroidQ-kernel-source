#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

obj-$(CONFIG_MTK_LEDS) := mtk_leds.o

#ifdef VENDOR_EDIT
#Bin.Liu@BSP.Fingerprint.Secure 2019/04/12, Add for led ic ktd2026
obj-$(CONFIG_OPPO_BACKLIGHT_KTD2026) += oppo_ktd2026/
#obj-$(CONFIG_OPPO_BACKLIGHT_KTD2026) += oppo_ktd2026b/
#Bin.Liu@BSP.Fingerprint.Secure 2019/04/12, Add for led ic aw2026
obj-$(CONFIG_AW2026_SLED) += oppo_aw2026/
#endif
ccflags-y += -I$(srctree)/drivers/misc/mediatek/video/include
