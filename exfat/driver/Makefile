
export ARCH ?= arm

ifneq ($(KERNELRELEASE),)
# Inside the kernel build system

EXTRA_CFLAGS += -I$(KBUILD_EXTMOD)


OBJS := exfat.o exfat_blkdev.o exfat_cache.o exfat_data.o \
		exfat_global.o exfat_nls.o exfat_oal.o exfat_upcase.o exfat_api.o exfat_super.o

SRC := $(OBJS:.o=.c)

MODULE:=exfat_fs.ko

obj-m += $(MODULE:.ko=.o)
$(MODULE:.ko=-y) := $(OBJS)
$(MODULE:.ko=-objs) := $(OBJS)

else

# Outside the kernel build system
#
#

# Define host system directory
KDIR-$(shell uname -m):=/lib/modules/$(shell uname -r)/build

include $(KDIR)/.config

# validate lookup result
ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

ifeq ($(ARCH), arm)
# when compiling for ARM we're cross compiling
export CROSS_COMPILE ?= arm-eabi-
endif

all:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) modules

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) kernelrelease

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) clean

endif
