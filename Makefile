ifndef KERNELRELEASE

PWD := $(shell pwd)
MOD := /lib/modules/`uname -r`

all:
	$(MAKE) -C $(MOD)/build SUBDIRS=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.* .*.cmd Module.symvers Module.markers modules.order
	rm -rf .tmp_versions

install:
	$(MAKE) -C $(MOD)/build SUBDIRS=$(PWD) modules_install
	@if test ! -d $(MOD)/updates; \
		then mkdir $(MOD)/updates; \
	fi
	@if test -f $(MOD)/extra/udlfb.ko; \
	then \
		cp $(MOD)/extra/udlfb.ko $(MOD)/updates/udlfb.ko ; \
	fi

else
     obj-m := udlfb.o
endif

