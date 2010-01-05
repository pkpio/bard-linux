ifndef KERNELRELEASE

PWD := $(shell pwd)
all:
	@echo '************************************************************'
	@echo For those building outside of the kernel build environment:
	@echo The udlfb module requires FB_DEFERRED_IO, CONFIG_FB_VIRTUAL
	@echo and FB_MODE_HELPERS support to be configured in-kernel.
	@echo '"Unknown symbol sys_fillrect" or "error no member fbdefio"'
	@echo errors mean kernel needs recompile with those options enabled
	@echo '************************************************************'
	$(MAKE) -C /lib/modules/`uname -r`/build SUBDIRS=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.* .*.cmd Module.symvers
	rm -rf .tmp_versions

install:
	$(MAKE) -C /lib/modules/`uname -r`/build SUBDIRS=$(PWD) modules_install

else
     obj-m := udlfb.o
endif

