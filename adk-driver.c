#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

/* Define these values to match your devices */
#define USB_VENDOR_ID 0x04e8
#define USB_PRODUCT_ID 0x6860

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

static int testusb_probe (struct usb_interface *interface, const struct usb_device_id *id){
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_skel *dev;
	int buffer_size;
	int i;

	printk("\ntestusb: probe module\n");

	iface_desc = interface->cur_altsetting;
	dev = usb_get_intfdata(interface);

	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (!dev->bulk_in_endpointAddr &&
		(endpoint->bEndpointAddress & USB_DIR_IN) &&
		((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)== USB_ENDPOINT_XFER_BULK)) {
			/* we found a bulk in endpoint */
			buffer_size = endpoint->wMaxPacketSize;
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->bulk_in_buffer) {
				printk("\nCould not allocate bulk_in_buffer\n");
				//goto error;
			}
		}

		if (!dev->bulk_out_endpointAddr &&
		!(endpoint->bEndpointAddress & USB_DIR_IN) &&
		((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
			== USB_ENDPOINT_XFER_BULK)) {
		/* we found a bulk out endpoint */
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
	}

	if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr)) {
		printk("\nCould not find both bulk-in and bulk-out endpoints\n");
		//goto error;
	}

	return 0;
}


static void testusb_disconnect (struct usb_interface *interface){
	printk("\ntestusb: disconnect module\n");
}


/* table of devices that work with this driver */
static struct usb_device_id test_table [] = {
	{ USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
	{ } /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, test_table);

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
