#
#		Makefile for Arm Kernel Module 
#		Copyright (C) 1984-2015 Core lab. <djshin.core.kaist.ac.kr>
#

MODULES = linkedlist_skel          # Your c file name 
# You should modify this line with your DIR name
BB_KERNEL = /root/bb-kernel

KERNEL_SOURCE = $(BB_KERNEL)/KERNEL
ARCH = arm
CURDIR = $(shell pwd)
CROSS_COMPILE = $(BB_KERNEL)/dl/gcc-linaro-4.9-2015.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
CFLAGS =

default: all
obj-m += $(MODULES:%=%.o)
BUILD = $(MODULES:%=%.ko)

all:: $(BUILD)
	
clean:
	rm -f $(BUILD) *.o *.ko *.mod.c *.mod.o *~ .*.cmdModule.symvers
	rm -rf .tmp_versions .*.cmd
	rm -f .*.cmdModule
	rm -f modules.order Module.symvers 

$(MODULES:%=%.ko):*c
	$(MAKE) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) -C $(KERNEL_SOURCE) SUBDIRS=$(CURDIR) M=$(CURDIR) modules
