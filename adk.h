#include <linux/usb.h>
#include <linux/slab.h>

#include "aoa.h"

/* ADK identification params */
#define ADK_MAN "BeagleBone"			/* Board manufacturer */
#define ADK_MOD "BeagleBone Black"		/* Board model */
#define ADK_DES "Development platform"		/* Board description */
#define ADK_VER "1.0"				/* Board version */
#define ADK_URI "http://beagleboard.org/"	/* Board url */
#define ADK_SER "42"				/* Board serial */

/* Structure to hold all of our device specific stuff */
struct adk_device {
	struct usb_device 	*udev;		/* usb device for this device */
	struct usb_interface 	*interface;	/* interface for this device */
	unsigned char 		minor;
};

static int setup_accessory(
	const struct adk_device *dev,
	const char *manufacturer,
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
		(char*)manufacturer, strlen(manufacturer), HZ*5);
	printk("Data length: %d\n", retval);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_MOD_ID, 
		(char*)modelName, strlen(modelName)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_DSC_ID, 
		(char*)description, strlen(description)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_VER_ID, 
		(char*)version, strlen(version)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		AOA_SEND_IDENT, 0x40, 0, AOA_STRING_URL_ID, 
		(char*)uri, strlen(uri)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
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
