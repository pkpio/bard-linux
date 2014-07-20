udlfb
=====

A fork of the latest udlfb driver.

How to run,

    'lsusb -v'

Update L13-17 in adk/adk.c with your device values

    'cd adk && make && sudo insmod adk.ko'

Install bard-droid on your device and attach it to pc

Your device should now be in adk mode. If not check above steps

    'lsusb -v'

Update L36-L40 in udlfb.c with your device values when it is in adk mode

Now detach your device and run,

    'cd .. && make && insmod udlfb.ko'

Check kernel log to verify udlfb is probed and a new fb device is registered

Update L4 of 'xorg.conf.fb' file with the value of new fb device created. Generally /dev/fb1 or /dev/fb2

To start xserver on this frame buffer,

    'FRAMEBUFFER=/dev/fb1 xinit -- /usr/bin/X :1 -config /etc/X11/xorg.conf.fb'
    
 
Regarding the raw frame data on device,

Received a repeating stream of E637 on to device - confirms it is infact 2BPP (RGB565)
E637 - should be interpreted as 37E6 and taking bits wise gives, (as R-5bits, G-6bits,B-5bits)
	R - 00110  -> 49
	G - 111111 -> 255
	B - 00110  -> 49
	
This translates to the full green color on the screen we see on setup success.

Also, after printks on the raw data, we got 767 hline calls for initial setup. and 1312 after xserver and 98 printk callbacks supressed message at end (means somthing here?)





1. The endianess of the data has to be taken care while handeling it.
2. A line of frame data can be sent at a time because our buffer size is 16k bytes and not 256 - the displaylink limit.
3. Look at some compress algo. RLE?
4. Try to encode more lines in a transfer. And encoding that there are more than 1 line.
