SHELL := /bin/bash
SRC := src/lkm-hook.o src/lkm-main.o src/lkm-intercept-io.o src/lkm-intercept.o src/lkm-monitor.o src/lkm-cc-ss.o src/lkm-file.o src/lkm-netlink.o
TARGET := cr2-km

KERNELDIR=/lib/modules/$(shell uname -r)/build
PWD=$(shell pwd)

obj-m += $(TARGET).o
$(TARGET)-objs += $(SRC)

all:
	@if \
	make -s -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules; \
	then echo -e "[\e[34;1mMLKM\e[m] \e[33mCompile cr2 loadable kernel module \e[m \e[36m->\e[m \e[32;1m$(TARGET).ko\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33mCompile cr2 loadable kernel module \e[m \e[36m->\e[m \e[32;1m$(TARGET).ko\e[m"; exit -1; fi;

clean:
	@if \
	make -s -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean; \
	then echo -e "[\e[34;1mCLEAN\e[m] \e[33mcr2 loadable kernel module \e[m"; \
	else "[\e[31mFAIL\e[m] \e[33mclean cr2 loadable kernel module\e[m "; exit -1; fi;
	
