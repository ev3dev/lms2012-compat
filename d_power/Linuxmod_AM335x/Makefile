ifneq ($(KERNELRELEASE),)

# kbuild part of makefile
obj-m += d_power.o

else

# normal makefile

TARGET = d_power.ko

CONF = Linuxmod
ARCH = AM335x

include ../../open_first/rules.mk

endif
