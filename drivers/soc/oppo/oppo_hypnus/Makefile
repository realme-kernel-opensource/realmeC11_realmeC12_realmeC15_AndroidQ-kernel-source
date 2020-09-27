ccflags-$(CONFIG_ARCH_QCOM) += -I$(srctree)/drivers/gpu/msm/
ccflags-$(CONFIG_MACH_MT6779) += -I$(srctree)/drivers/misc/mediatek/include/ -I$(srctree)/drivers/misc/mediatek/base/power/include/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/mcdi/include -I$(srctree)/drivers/misc/mediatek/sched/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/mcdi/mt6779/ -I$(srctree)/drivers/misc/mediatek/base/power/mt6779/        \
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt6779/include/	\
	-I$(srctree)/drivers/misc/mediatek/vpu/3.0/	\
	-I$(srctree)/drivers/misc/mediatek/vpu/mt6779/	\
	-I$(srctree)/drivers/misc/mediatek/video/include/ \
	-I$(srctree)/drivers/misc/mediatek/mdla/1.0/

ccflags-$(CONFIG_MACH_MT6765) += -I$(srctree)/drivers/misc/mediatek/include/ -I$(srctree)/drivers/misc/mediatek/base/power/include/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/mcdi/include -I$(srctree)/drivers/misc/mediatek/sched/	\
	-I$(srctree)/drivers/misc/mediatek/base/power/mcdi/mt6765/ -I$(srctree)/drivers/misc/mediatek/base/power/mt6765/        \
	-I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt6765/include/	\
	-I$(srctree)/drivers/misc/mediatek/vpu/3.0/	\
	-I$(srctree)/drivers/misc/mediatek/vpu/mt6765/	\
	-I$(srctree)/drivers/misc/mediatek/video/include/ \
	-I$(srctree)/drivers/misc/mediatek/mdla/2.0/ \
	-I$(srctree)/drivers/misc/mediatek/mdla/1.0/

ifdef CONFIG_ARCH_QCOM
hypnus_ops-y := hypnus_msm_op.o
else
hypnus_ops-y := hypnus_mtk_op.o
endif

oppo_hypnus-objs := hypnus_main.o hypnus_dev.o hypnus_sysfs.o hypnus_display.o
oppo_hypnus-objs += $(hypnus_ops-y)

obj-$(CONFIG_OPPO_HYPNUS) += oppo_hypnus.o
