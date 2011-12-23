ifndef KERNELRELEASE

PWD := $(shell pwd)
all:
	$(MAKE) -C /lib/modules/`uname -r`/build SUBDIRS=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.* .*.cmd Module.symvers Module.markers modules.order
	rm -rf .tmp_versions

install:
	$(MAKE) -C /lib/modules/`uname -r`/build SUBDIRS=$(PWD) modules_install
	cp /lib/modules/`uname -r`/extra/udlfb.ko /lib/modules/`uname -r`/updates/udlfb.ko

else
     obj-m := udlfb.o
endif

