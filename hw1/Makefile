obj-m:=hw1_driver.o
KERNEL_DIR	:=/lib/modules/$(shell uname -r)/build
PWD		:=$(shell pwd)

all:
	$(MAKE)	-C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
	gcc -o provider provider.c
	gcc -o consumer consumer.c
	gcc -o cleaner cleaner.c
clean:
	$(MAKE)	-C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
	rm provider consumer cleaner
