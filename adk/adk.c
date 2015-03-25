#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "adk.h"
#include "devices.h"

#define DEBUG_TAG "ADK"

/* table of devices that work with this driver */
static struct usb_device_id adk_devices [] = {
	{.idVendor = VENDOR_ID_GOOGLE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_INTEL,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HTC,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_SAMSUNG,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_MOTOROLA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_LGE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HUAWEI,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_OP,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_OPPO,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{ } /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, adk_devices);

static struct usb_driver adk_driver;

static int
print (char *msg)
{
	return printk("%s : %s\n", DEBUG_TAG, msg);
}


static int
adk_probe (struct usb_interface *interface, const struct usb_device_id *id)
{
	struct adk_device *dev;
	int retval = -ENODEV;
	print("Device attached");
	
	dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		retval = -ENOMEM;
		goto exit;
	}
	
	printk("Product id is: %04x \n", id->idProduct);

	memset(dev, 0x00, sizeof(*dev));
	
	dev->udev = interface_to_usbdev(interface);
	if (!dev->udev) {
		retval = -ENODEV;
	}

	retval = setup_accessory(dev, ADK_MAN, ADK_MOD, ADK_DES, 
			 		ADK_VER, ADK_URI, ADK_SER);
			 		
exit:
	return retval;
}

static void
adk_disconnect (struct usb_interface *interface){
	print("Device detached");
}

static struct usb_driver adk_driver = {
	.name = "adk_driver",
	.id_table = adk_devices,
	.probe = adk_probe,
	.disconnect = adk_disconnect,
};

static int __init
adk_init (void)
{
	int result;
	print("Init called!");

	result = usb_register(&adk_driver);
	if (result)
		print("Registering driver failed!");
	else
		print("Driver registered successfully!");

	return result;
}

static void __exit
adk_exit (void)
{
	usb_deregister(&adk_driver);
	print("Exit called!");
}

module_init(adk_init);
module_exit(adk_exit);

MODULE_AUTHOR("Praveen Kumar Pendyala");
MODULE_LICENSE("GPL");
