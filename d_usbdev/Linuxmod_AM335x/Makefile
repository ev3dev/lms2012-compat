ifneq ($(KERNELRELEASE),)

# kbuild part of makefile
obj-m += d_usbdev.o

else

# normal makefile

TARGET = d_usbdev.ko

CONF = Linuxmod
ARCH = AM335x

include ../../open_first/rules.mk

endif
