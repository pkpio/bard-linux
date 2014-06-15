#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "devices.h"

/* table of devices that work with this driver */
static struct usb_device_id test_table [] = {
	{ USB_DEVICE(USB_GOO_VENDOR_ID, USB_N10_PRODUCT_ID) },
	{ USB_DEVICE(USB_SAM_VENDOR_ID, USB_GS2_PRODUCT_ID) },
	{ } /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, test_table);

/* Structure to hold all of our device specific stuff */
struct usb_skel {
	struct usb_device *	udev;			/* the usb device for this device */
	struct usb_interface *	interface;		/* the interface for this device */
	unsigned char *		bulk_in_buffer;		/* the buffer to receive data */
	size_t			bulk_in_size;		/* the size of the receive buffer */
	__u8			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
	struct kref		kref;
};

static int setup_accessory(
	const struct usb_skel *dev,
	const char *manufacturer,
	const char *modelName,
	const char *description,
	const char *version,
	const char *uri,
	const char *serialNumber);

static int testusb_probe (struct usb_interface *interface, const struct usb_device_id *id){
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_skel *dev;
	int buffer_size;
	int i;
	unsigned int test;
	unsigned char ioBuffer[2];

	printk("\ntestusb: probe module\n");

	dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if(dev == NULL)
		return -1;
		
	memset(dev, 0x00, sizeof(*dev));
	kref_init(&dev->kref);

	dev->udev = interface_to_usbdev(interface);
	dev->interface = interface;

	/* setup into accessory */
	i = setup_accessory(
		dev,
		"Nexus-Computing GmbH",
		"Model",
		"Description",
		"VersionName",
		"http://neuxs-computing.ch",
		"SerialNo.");
	
	printk("\nADK-probe: accessory setup response: %d\n", i);

	return 0;
}

static int setup_accessory(
	const struct usb_skel *dev,
	const char *manufacturer,
	const char *modelName,
	const char *description,
	const char *version,
	const char *uri,
	const char *serialNumber) {

	unsigned char ioBuffer[2];
	int devVersion;
	int retval;
	int tries = 15;

	printk("\nAccessory-Setup: accessory setup started\n");
	
	/* send accessory setup sequence */
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		51, 0xc0, 0x00, 0x00, ioBuffer, 2, HZ*5);
	if (retval < 0) 
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		52, 0x40, 0, 0, (char*)manufacturer, strlen(manufacturer), HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		52, 0x40, 0, 1, (char*)modelName, strlen(modelName)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		52, 0x40, 0, 2, (char*)description, strlen(description)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		52, 0x40, 0, 3, (char*)version, strlen(version)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		52, 0x40, 0, 4, (char*)uri, strlen(uri)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		52, 0x40, 0, 5, (char*)serialNumber, strlen(serialNumber)+1, HZ*5);
	if (retval < 0)
		goto exit;
		
	printk("\nAccessory identification sent. Attempting accessory mode.\n");
	
	
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		53, 0x40, 0, 0, NULL, 0, HZ*5);
	if (retval < 0)
		goto exit;
		
	
	return retval;	
	
exit:
	printk("usb_control_msg failed (%d)", retval);
	return retval;

}

static void testusb_disconnect (struct usb_interface *interface){
	printk("\ntestusb: disconnect module\n");
}

static struct usb_driver testusb_driver = {
	.name = "adk-driver",
	.id_table = test_table,
	.probe = testusb_probe,
	.disconnect = testusb_disconnect,
};

static int __init testusb_init(void){
	int result;
	printk("\ntestusb: Init called\n");

	result = usb_register(&testusb_driver);
	if (result)
		printk("\ntestusb: registering driver failed\n");
	else
		printk("\ntestusb: driver registered successfully\n");

	return result;
}


static void __exit testusb_exit(void){
	usb_deregister(&testusb_driver);
	printk("\ntestusb: module deregistered\n");
}

module_init(testusb_init);
module_exit(testusb_exit);

MODULE_AUTHOR("Praveen Kumar");
MODULE_LICENSE("GPL");
