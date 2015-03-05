This is intended to give a quick overview of the project. Read this and if you still have any queries, 
come over to #beagle-gsoc irc channel. You may address me(praveendath92), Vlad(vvu) or Vladimir(av500). Please note 
that this doc is written to cater the interest of even absolute begginers so, expect some verbose comments.

Structure
--------------------
The whole pacakge comes in 2 parts.

  1. [Android application][1]
  2. Kernel driver

Headsup Droid enthusiasts, major work here is on the kernel driver - you will have just as much fun however!

More on fine structural details later


Kernel driver
---------------------
It would be a plus if you already have some knowledge about Linux kernel drivers. If you don't, just remember 
to come back and check this [Linux Kernel Driver Tutorial][3]


Communication
--------------------
The comm. between Android and Linux (Bone) is over the USB bus - that's obvious by now, right? We use 
[Android Open Accessory Protocol][1] in our USB communication - the reason behind this choice is to support Android
devices without making changes to the ROM. Since we will be doing USB comm., our kernel driver acts as a USB Driver 
for your Android phone. It would nice if you a brief overview of [Linux USB drivers][4] - it's not as complicated as 
it may sound. Just skim through one of the tutorials.


Graphics
---------------------
We will making any future references to graphics part as framebuffer. Linux kernel offers Framebuffer APIs - just 
like the USB APIs

[1]: https://github.com/praveendath92/bard-droid
[2]: https://source.android.com/accessories/protocol.html
[3]: http://www.thegeekstuff.com/2013/07/write-linux-kernel-module/
