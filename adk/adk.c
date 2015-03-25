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
	{.idVendor = VENDOR_ID_ACER,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_ALLWINNER,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_AMLOGIC,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_ANYDATA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_ARCHOS,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_ASUS,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_BYD,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_COMPAL,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_COMPALCOMM,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_DELL,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_ECS,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_EMERGING_TECH,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_EMERSON,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_FOXCONN,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_FUJITSU,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_FUNAI,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_GARMIN_ASUS,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_GIGABYTE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_GIGASET,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_GIONEE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	/**
	 * Google vendor id clashes with main driver
	 * Currently commented out. Add support to 
	 * Google devices the old way for now. Check 
	 * Nexus 10 example at the end of this list.
	 *
	 *{.idVendor = VENDOR_ID_GOOGLE,
	 * .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	 */
	{.idVendor = VENDOR_ID_HAIER,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HARRIS,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HISENSE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HONEYWELL,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HP,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HTC,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_HUAWEI,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_INQ_MOBILE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_INTEL,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_INTERMEC,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_IRIVER,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_K_TOUCH,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_KT_TECH,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_KOBO,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_KYOCERA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_LAB126,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_LENOVO,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_LENOVOMOBILE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_LGE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_LUMIGON,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_MOTOROLA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_MSI,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_MTK,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_NEC,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_NOOK,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_NVIDIA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_OP,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_OPPO,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_OTGV,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_OUYA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_PANTECH,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_PEGATRON,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_PHILIPS,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_PMC,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_POSITIVO,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_PRESTIGIO,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_QISDA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_QUALCOMM,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_QUANTA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_ROCKCHIP,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_SAMSUNG,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_SHARP,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_SK_TELESYS,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_SMARTISAN,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_SONY,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_SONY_ERICSSON,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_T_AND_A,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_TECHFAITH,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_TELEEPOCH,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_TI,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_TOSHIBA,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_UNOWHY,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_VIZIO,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_WACOM,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_XIAOMI,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_YOTADEVICES,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_YULONG_COOLPAD,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{.idVendor = VENDOR_ID_ZTE,
	 .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	/**
	 * Nexus 10. Google devices should be added like 
	 * this for now.
	 */
	{USB_DEVICE_AND_INTERFACE_INFO(
		VENDOR_ID_GOOGLE, 0x4ee2, 0xff, 0x42, 0x00)},
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
