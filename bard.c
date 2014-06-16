#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "devices.h"
#include "adk.h"

#define DEBUG_TAG "BARD"

/* table of devices that work with this driver */
static struct usb_device_id adk_devices [] = {
	{ USB_DEVICE(USB_GOO_VENDOR_ID, USB_N10_PRODUCT_ID) },
	{ USB_DEVICE(USB_SAM_VENDOR_ID, USB_GS2_PRODUCT_ID) },
	{ USB_DEVICE(USB_GOO_VENDOR_ID, AOA_ACCESSORY_PID) },
	{ USB_DEVICE(USB_SAM_VENDOR_ID, AOA_ACCESSORY_PID) },
	{ } /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, adk_devices);

static struct usb_driver bard_driver;

static int
print (char *msg)
{
	return printk("%s : %s\n", DEBUG_TAG, msg);
}

static int
bard_probe (struct usb_interface *interface, const struct usb_device_id *id)
{
	struct adk_device *dev;
	int retval = -ENODEV;
	print("Device attached");
	
	dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		retval = -ENOMEM;
		goto exit;
	}

	memset(dev, 0x00, sizeof(*dev));
	
	dev->udev = interface_to_usbdev(interface);
	if (!dev->udev) {
		retval = -ENODEV;
		goto exit;
	}
	dev->interface = interface;
	
	/* Device attached in normal mode */
	if(id->idProduct != AOA_ACCESSORY_PID)
		retval = setup_accessory(
			dev,
			"Nexus-Computing GmbH",
			"Model",
			"Description",
			"VersionName",
			"http://neuxs-computing.ch",
			"SerialNo.");
	/* Device attached in accessory mode */
	else
		print("Attached as an accessory");

exit:
	return retval;
}

static void
bard_disconnect (struct usb_interface *interface){
	print("Device disconnected");
}

static struct usb_driver bard_driver = {
	.name = "bard_driver",
	.id_table = adk_devices,
	.probe = bard_probe,
	.disconnect = bard_disconnect,
};

static int __init
bard_init (void)
{
	int result;
	print("Init called!");

	result = usb_register(&bard_driver);
	if (result)
		print("Registering driver failed!");
	else
		print("Driver registered successfully!");

	return result;
}

static void __exit
bard_exit (void)
{
	usb_deregister(&bard_driver);
	print("Exit called!");
}

module_init(bard_init);
module_exit(bard_exit);

MODULE_AUTHOR("Praveen Kumar");
MODULE_LICENSE("GPL");
