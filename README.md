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


Before setup
-------------------
Note: <br/>

1. This is still in the development phase and not much inputs have been put into simplifying 
the setup process for a new user. So, please be informed that the setup might take sometime 
depending on your prior expertise.

2. There are two drivers currently - adk, udlfb. adk driver is to set your device
into adk mode. udlfb is the framebuffer + usb driver which requires the attached
device to be in adk mode. udlfb is the main driver.


Setup
-------------------

1. You will need to know your Android device's vendor_id, product_id, class, subclass
and protocol values. Use the below command to findout,
   
    ```lsusb -v```

2. You need to update those values in adk-driver. L13-17 in ```adk/adk.c```

3. Now ```make``` and load the adk driver.

    ```cd adk && make && sudo insmod adk.ko```

4. Install ```bard.apk``` on Android and attach it to PC / Bone. Get setup from, <br/>
   <http://files.praveenkumar.co.in/bard.apk> <br/>
or you may build the latest version from source at, <br/>
<https://github.com/praveendath92/bard-droid>

5. Check that your device entered adk mode on attaching. If not, recheck above steps

6. The usb identifiers change in adk mode. So, you need to findout the new vendor_id, 
product_id, class, subclass and protocol values. Use the below command to findout,

    ```lsusb -v```
    
7. You need to update those values in udlfb-driver.Update L36-L40 in udlfb.c Note
that these values should be collected using step 6 when device is in adk mode.

8. Now detach your device and run,

    ```cd .. && make && insmod udlfb.ko```
    
9. Check kernel log to verify udlfb is probed and a new fb device is registered.

10. Update L4 of ```refs/xorg.conf.fb``` file with the value of new fb device created. 
Generally ```/dev/fb1``` or ```/dev/fb2``` and copy that file to ```/etc/X11```


Starting remote display
-----------------------

Note: The value of ```X``` in ```\dev\fbX``` should be replaced with value found
from step 10 of setup.<br/>

To start xserver on the framebuffer, run, <br/>

On PC :<br/>
```FRAMEBUFFER=/dev/fbX xinit -- /usr/bin/X :1 -config /etc/X11/xorg.conf.fb```
    
On BeagleBone :<br/>
```FRAMEBUFFER=/dev/fb1 startx -- /usr/bin/X :1```
    
[1]: http://blog.praveenkumar.co.in/2014/05/gsoc-14-proposal-for-beagleorg
[2]: https://github.com/praveendath92/bard-linux/blob/master/documentation/dev.md
[3]: http://blog.praveenkumar.co.in/2014/05/beagle-gsoc-progress-tracking/
[4]: https://github.com/praveendath92/adk-driver
