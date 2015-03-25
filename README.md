bard-linux
=========================
Android remote display for BeagleBoard. This is a proposal 
for Google Summer of Code 2014 under Beagle.org. Check the [complete proposal][1].

Developers / GSoC aspirants
-----------------------------
Please start by [reading this][2]


Weekly reports
-------------------
Weekly reports can be found at [here][3]


Framebuffer
-------------------
Developed upon udlfb. udlfb is DisplayLinks's kernel framebuffer + USB driver. 
Most of the framebuffer work in this driver is taken directly from udlfb.


Usb communication
-------------------
USB communication is done using Android Open Accessory (AOA) protocal with the
Android device setup into Accessory Development Kit (ADK). Read about ADK on 
Android developers webpage. 

A bare-bone implementation of the ADK driver, with only usb communication, can 
be found [here][4]


Setup
-------------------

1. ```make``` and ```insmod``` both drivers udlfb and adk

    ```cd adk && make && sudo insmod adk.ko && cd .. && make && insmod udlfb.ko```

2. Install Android app from [Alpha testing][6] or [build from source][7]
3. Attach your device - app opens popus up showing a green screen

Note: You may check the list of [supported OEMs here][8]


Starting remote display
-----------------------

Note: The value of ```X``` in ```\dev\fbX``` should be replaced with value new framebuffer device - appears after setup.<br/>

To start xserver on the framebuffer, run, <br/>

On PC :<br/>
```FRAMEBUFFER=/dev/fbX xinit -- /usr/bin/X :1 -config /etc/X11/xorg.conf.fb```
    
On BeagleBone :<br/>
```FRAMEBUFFER=/dev/fb1 startx -- /usr/bin/X :1```
    
[1]: http://blog.praveenkumar.co.in/2014/05/gsoc-14-proposal-for-beagleorg
[2]: https://github.com/praveendath92/bard-linux/blob/master/documentation/dev.md
[3]: http://blog.praveenkumar.co.in/2014/05/beagle-gsoc-progress-tracking/
[4]: https://github.com/praveendath92/adk-driver
[5]: https://github.com/praveendath92/bard-linux/blob/master/udlfb.c#L83
[6]: https://plus.google.com/communities/112465954313348402923
[7]: https://github.com/praveendath92/bard-droid
[8]: https://github.com/praveendath92/bard-linux/blob/master/adk/devices.h
