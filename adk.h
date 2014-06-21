#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* ADK identification params */
#define ADK_MAN "BeagleBone"			/* Board manufacturer */
#define ADK_MOD "BeagleBone Black"		/* Board model */
#define ADK_DES "Development platform"		/* Board description */
#define ADK_VER "1.0"				/* Board version */
#define ADK_URI "http://beagleboard.org/"	/* Board url */
#define ADK_SER "42"				/* Board serial */

/* AOA protocol ids */
#define AOA_GET_PROTOCOL		51
#define AOA_SEND_IDENT			52
#define AOA_START_ACCESSORY		53
#define AOA_REGISTER_HID		54
#define AOA_UNREGISTER_HID		55
#define AOA_SET_HID_REPORT_DESC		56
#define AOA_SEND_HID_EVENT		57
#define AOA_AUDIO_SUPPORT		58

/* AOA String IDs */
#define AOA_STRING_MAN_ID		0
#define AOA_STRING_MOD_ID		1
#define AOA_STRING_DSC_ID		2
#define AOA_STRING_VER_ID		3
#define AOA_STRING_URL_ID		4
#define AOA_STRING_SER_ID		5

/* AOA Product IDs */
#define AOA_ACC_PID		0x2d00	/* accessory */
#define AOA_ACC_ADB_PID		0x2d01	/* accessory + adb */
#define AOA_AUD_PID		0x2D02	/* audio */
#define AOA_AUD_ADB_PID		0x2D03	/* audio + adb */
#define AOA_ACC_AUD_PID		0x2D04	/* accessory + audio */
#define AOA_ACC_AUD_ADB_PID	0x2D05	/* accessory + audio + adb */

/* Structure to hold all of our device specific stuff */
struct adk_device {
	struct usb_device 	*udev;		/* usb device for this device */
	struct usb_interface 	*interface;	/* interface for this device */
	unsigned char 		minor;		/* minor value */
	unsigned char *		bulk_in_buffer;	/* the buffer to in data */
	size_t			bulk_in_size;	/* the size of the in buffer */
	__u8			bulk_in_add;	/* bulk in endpoint address */
	__u8			bulk_out_add;	/* bulk out endpoint address */
	struct kref		kref;		/* module references counter */
};

char* utf8(const char *str)
{
	char *utf8;
	utf8 = kmalloc(1+(2*strlen(str)), GFP_KERNEL);

	if (utf8) {
		char *c = utf8;
		for (; *str; ++str) {
			if (*str & 0x80) {
				*c++ = *str;
			} else {
				*c++ = (char) (0xc0 | (unsigned) *str >> 6);
				*c++ = (char) (0x80 | (*str & 0x3f));
			}
		}
		*c++ = '\0';
	}
	return utf8;
}

static int setup_accessory(
	const struct adk_device *dev,
	const char *manufacturer,
	const char *modelName,
	const char *description,
	const char *version,
	const char *uri,
	const char *serialNumber) {

	unsigned char ioBuffer[2];
	int retval;
	
	printk("\nAccessory-Setup: accessory setup started\n");
	
	/* send accessory setup sequence */
	
	/* Read device version */
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		AOA_GET_PROTOCOL, 0xc0, 0, 0, ioBuffer, 2, HZ*5);
	if (retval < 0) 
		goto exit;
		
	/* Manufacturer */
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				AOA_SEND_IDENT, 0x40, 0, AOA_STRING_MAN_ID, 
				utf8((char *)manufacturer), 
				strlen(utf8((char *)manufacturer)), HZ*5);
	if (retval < 0)
		goto exit;
		
	/* Modelname */
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				AOA_SEND_IDENT, 0x40, 0, AOA_STRING_MOD_ID, 
				utf8((char*)modelName), 
				strlen(utf8((char*)modelName))+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	/* Description */
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				AOA_SEND_IDENT, 0x40, 0, AOA_STRING_DSC_ID, 
				utf8((char*)description), 
				strlen(utf8((char*)description))+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	/* Version */
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				AOA_SEND_IDENT, 0x40, 0, AOA_STRING_VER_ID, 
				utf8((char*)version), 
				strlen(utf8((char*)version))+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	/* URI */
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				AOA_SEND_IDENT, 0x40, 0, AOA_STRING_URL_ID, 
				utf8((char*)uri), 
				strlen(utf8((char*)uri))+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	/* Serialnumber */
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				AOA_SEND_IDENT, 0x40, 0, AOA_STRING_SER_ID, 
				utf8((char*)serialNumber), 
				strlen(utf8((char*)serialNumber))+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	printk("\nAccessory identification sent. Attempting accessory mode.\n");
	
	
	/* Start accessory */
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
				AOA_START_ACCESSORY, 0x40, 0, 0, NULL, 0, HZ*5);
	if (retval < 0)
		goto exit;
		
	
	return retval;	
	
exit:
	printk("usb_control_msg failed (%d)", retval);
	return retval;

}
