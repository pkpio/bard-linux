udlfb
=====

A fork of the latest udlfb driver.

How to run,

	'lsusb -v'

* Update L13-17 in adk/adk.c with your device values

	'cd adk && make && sudo insmod adk.ko'

* Install bard-droid on your device and attach it to pc

* Your device should now be in adk mode. If not check above steps

	'lsusb -v'

* Update L36-L40 in udlfb.c with your device values when it is in adk mode

* Now detach your device and run,

	'cd .. && make && insmod udlfb.ko'

* Check kernel log to verify udlfb is probed and a new fb device is registered

* Update L4 of 'xorg.conf.fb' file with the value of new fb device created. Generally /dev/fb1 or /dev/fb2

* To start xserver on this frame buffer,

    'FRAMEBUFFER=/dev/fb1 xinit -- /usr/bin/X :1 -config /etc/X11/xorg.conf.fb'
