ARCH = AM335x

BASE = ..

KERNEL = $(BASE)/../../board-support/linux-3.12.10-ti2013.12.01
UBOOT = $(BASE)/../../board-support/u-boot-2013.10-ti2013.12.01


KERNEL_CONF = lego.config

default: help

include rules.mk




#
# lms2012 program.
#

LIBS = c_com c_input c_memory c_output c_sound c_ui c_dynload

almostall: lms2012 programs kernel modules

lms2012:
	$(MAKE) -C $(BASE)/lms2012/Linux_$(ARCH)

lms2012.clean: $(LIBS:%=%.clean)
	$(MAKE) -C $(BASE)/lms2012/Linux_$(ARCH) clean uninstall

$(LIBS:%=%.clean): %.clean:
	$(MAKE) -C $(BASE)/$*/Linuxlib_$(ARCH) clean uninstall

.PHONY: lms2012 lms2012.clean $(LIBS:%=%.clean)




#
# lms2012 modules.
#

MODULES = d_analog d_bt d_iic d_power d_pwm d_sound d_uart d_ui d_usbdev \
	  d_usbhost

modules: $(MODULES)

$(MODULES):
	$(MAKE) -C $(BASE)/$@/Linuxmod_$(ARCH)

modules.clean: $(MODULES:%=%.clean)

$(MODULES:%=%.clean): %.clean:
	$(MAKE) -C $(BASE)/$*/Linuxmod_$(ARCH) clean uninstall

.PHONY: modules $(MODULES) modules.clean $(MODULES:%=%.clean)



#
# Programs.
#

#programs:
#	$(MAKE) -C $(BASE)/lmssrc

#programs.clean:
#	$(MAKE) -C $(BASE)/lmssrc clean uninstall




#
# Kernel.
#

kernel: zImage

$(KERNEL)/.config: $(KERNEL_CONF)
	cp $< $@

kernel.prepare: $(KERNEL)/include/generated/bounds.h
$(KERNEL)/include/generated/bounds.h: $(KERNEL)/.config
	$(MAKE) -C $(KERNEL) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) oldconfig modules_prepare

$(KERNEL)/arch/arm/boot/zImage: kernel.prepare | $(MKIMAGE_CHECK)
	$(MAKE) -C $(KERNEL) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) zImage

zImage: $(KERNEL)/arch/arm/boot/zImage
	cp $< $@

kernel.clean:
	$(MAKE) -C $(KERNEL) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) distclean

.PHONY: kernel kernel.prepare kernel.clean




#
# U-Boot.
#

u-boot: u-boot.img

$(UBOOT)/am335x_evm/MLO:
	$(MAKE) -C $(UBOOT) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm O=am335x_evm am335x_evm
#	$(MAKE) -C $(UBOOT) CROSS_COMPILE=$(CROSS_COMPILE) all

u-boot.img: MLO
	cp $(UBOOT)/am335x_evm/u-boot.img $@

MLO: $(UBOOT)/am335x_evm/MLO
	cp $< $@

u-boot.clean:
	$(MAKE) -C $(UBOOT) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm distclean
	rm -r $(UBOOT)/am335x_evm

.PHONY: u-boot u-boot.clean




#
# Misc.
#

help:
	@echo 'Execute "make TARGET" where TARGET is:'
	@echo ' lms2012: to build the lms2012 program and its libraries'
	@echo ' modules: to build lms2012 kernel modules'
	@echo ' programs: to build bytecode programs and their data files'
	@echo ' kernel: to build Linux kernel'
	@echo ' u-boot: to build u-Boot'
	@echo ' doc: to build documentation'
	@echo
	@echo 'Execute "make doc.view" to view the documentation in a web'
	@echo 'browser'
	@echo 
	@echo 'Execute "make TARGET.clean" to clean an individual target'
	@echo 'or "make clean" to clean everything'
	@echo
	@echo 'Add the ARCH=X86 argument to run for X86 platform (some targets'
	@echo 'are then meaningless).'

clean: lms2012.clean modules.clean programs.clean kernel.clean u-boot.clean \
	doc.clean

.PHONY: help clean
