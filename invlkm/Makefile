TARGET_MODULE := invpcid-module

ifneq ($(KERNELRELEASE),)
	$(TARGET_MODULE)-objs := invpcid.o
	obj-m := $(TARGET_MODULE).o
else
	BUILDSYSTEM_DIR := /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

all: 
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean

load:
	sudo insmod ./$(TARGET_MODULE).ko

unload:
	sudo rmmod ./$(TARGET_MODULE).ko
endif
