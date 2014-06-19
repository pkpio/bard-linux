#include <linux/usb.h>
#include <linux/slab.h>

/* ADK identification params */
#define ADK_MAN "BeagleBone"			/* Board manufacturer */
#define ADK_MOD "BeagleBone Black"		/* Board model */
#define ADK_DES "Development platform"		/* Board description */
#define ADK_VER "1.0"				/* Board version */
#define ADK_URI "http://beagleboard.org/\0"	/* Board url */
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
#define AOA_ACCESSORY_PID		0x2d00	/* accessory */
#define AOA_ACCESSORY_ADB_PID		0x2d01	/* accessory + adb */
#define AOA_AUDIO_PID			0x2D02	/* audio */
#define AOA_AUDIO_ADB_PID		0x2D03	/* audio + adb */
#define AOA_ACCESSORY_AUDIO_PID		0x2D04	/* accessory + audio */
#define AOA_ACCESSORY_AUDIO_ADB_PID	0x2D05	/* accessory + audio + adb */

/* Structure to hold all of our device specific stuff */
struct adk_device {
	struct usb_device 	*udev;		/* usb device for this device */
	struct usb_interface 	*interface;	/* interface for this device */
	unsigned char 		minor;
};

static int setup_accessory(
	const struct adk_device *dev,
	unsigned char *manufacturer,
	const char *modelName,
	const char *description,
	const char *version,
	const char *uri,
	const char *serialNumber) {

	unsigned char ioBuffer[2];
	int devVersion;
	int retval;
	
	printk((char*)manufacturer);
	printk("\nAccessory-Setup: accessory setup started\n");
	
	/* send accessory setup sequence */
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		AOA_GET_PROTOCOL, 0xc0, 0, 0, ioBuffer, 2, HZ*5);
	if (retval < 0) 
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_MAN_ID, 
		manufacturer, strlen(manufacturer), HZ*5);
	printk("Data length: %d\n", retval);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_MOD_ID, 
		(char*)modelName, strlen(modelName)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_DSC_ID, 
		(char*)description, strlen(description)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_VER_ID, 
		(char*)version, strlen(version)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_URL_ID, 
		(char*)uri, strlen(uri)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_SER_ID, 
		(char*)serialNumber, strlen(serialNumber)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	printk("\nAccessory identification sent. Attempting accessory mode.\n");
	
	
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		AOA_START_ACCESSORY, 0x40, 0, 0, NULL, 0, HZ*5);
	if (retval < 0)
		goto exit;
		
	
	return retval;	
	
exit:
	printk("usb_control_msg failed (%d)", retval);
	return retval;

}
