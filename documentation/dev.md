This is intended to give a quick overview of the project. Read this and if you still have any queries, 
come over to #beagle-gsoc irc channel. You may address me(praveendath92), Vlad(vvu) or Vladimir(av500). 
Please note that this doc is written to cater the interest of even absolute begginers so, expect some 
verbose comments.

Structure
--------------------
The whole pacakge comes in 2 parts.

  1. [Android application][1]
  2. Kernel driver

Headsup Droid enthusiasts, major work here is on the kernel driver - you will have just as much fun however! 
More on fine structural details, later.


Kernel driver
---------------------
It would be a plus if you already have some knowledge about Linux kernel drivers. If you don't, just remember 
to come back and check this [Linux Kernel Driver Tutorial][3]


Communication
--------------------
The comm. between Android and Linux (Bone) is over the USB bus - that's obvious by now, right? We use 
[Android Open Accessory Protocol][1] in our USB communication - the reason behind this choice is to support Android
devices without making changes to the ROM. Since we will be doing USB comm., our kernel driver acts as a USB Driver 
for your Android phone. It would nice if you a brief overview of [Linux USB drivers][4] - it's not as complicated 
as it may sound. Just skim through one of the tutorials.


Graphics
---------------------
We will making any future references to graphics part as framebuffer. Linux kernel offers Framebuffer APIs - 
like the USB APIs, to interact with graphics without having to know too much details about the underlying graphics 
hardware. Take may take a look at the framebuffer drivers structure but that's not necessary at this point. All you
need to know is our driver will be acting as a framebuffer driver and usb driver at the same time. So, when we load
our module into kernel, we register USB and Framebuffer driver. More details in framebuffer section.


Framebuffer
----------------------
For this project we used the kernel framebuffer driver [udlfb], developed by DisplayLink devs. There is a lot of 
code, actually major portion of it, related to the framebuffer tasks. A quick tip: The way it works is, you register 
call backs with Linux kernel and when there are updates to the graphics, linux kernel will trigger 
the call back functions with modified part of the screen as params. Spoiler alert, [the function is this][5]. Current 
image on screen is saved in pages of memory by the kernel and you will be notified with only the updated portions of 
that memory / current image on screen. We are using a fixed resolution of 1024x768 and a eaqch pixel is 2 Bytes. 
A page is 4096 bytes. Do the math, you will find out that, a page has 2 lines of current image 
(line is from left - right). I gave away more than what I should. Just go through that function.


Android Accessory Mode
-------------------------
As mentioned earlier, we use AOA protocol. The way it works is, you attach your device to Linux machine and it should
send a series of commands so that your phone goes into Android Accessory Mode. USB comm. can be done with the app. 
only after the device enters Accessory mode (you will see a persistent notification). To set the device into Accessory
mode (referred ADK mode hence forth), you will be using another small driver in this repo, named [ADK][6]. 
So, to summarize:

  - Linux: 2 drivers
    1. ADK driver - To set device into accessory mode
    2. udlfb - The main driver that transfers UI
  - Andriod app: well, that's it.

Feel free to drop an email or come by IRC. 


[1]: https://github.com/praveendath92/bard-droid
[2]: https://source.android.com/accessories/protocol.html
[3]: http://www.thegeekstuff.com/2013/07/write-linux-kernel-module/
[4]: http://www.linuxjournal.com/article/7353
[5]: https://github.com/praveendath92/bard-linux/blob/master/udlfb.c#L491
[6]: https://github.com/praveendath92/bard-linux/tree/master/adk
