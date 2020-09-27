#include $(srctree)/drivers/misc/mediatek/Makefile.custom
#ifdef VENDOR_EDIT
#wen.luo@BSP.Kernel.Stability, 2019/2/21,  for enable KASAN, use GCC complile tool, add flag format
ifeq ($(CONFIG_KASAN),y)
subdir-ccflags-y += -Wno-format
endif
#endif /*VENDOR_EDIT*/
obj-y += s5kgd1spmipiraw_Sensor.o

