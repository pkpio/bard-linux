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

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	iface_desc = interface->cur_altsetting;
	printk("\nADK-probe: Number endpoints found: %d\n", iface_desc->desc.bNumEndpoints);
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		/* for INTERRUPT endpoint */
		if((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)== USB_ENDPOINT_XFER_INT)
			printk("\nADK-probe: Found an interrupt endpoint\n");
			
		/* for CONTROL endpoint */
		if((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)== USB_ENDPOINT_XFER_CONTROL)
			printk("\nADK-probe: Found a control endpoint\n");
		
		/* for BULK IN endpoint */
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
			} else{
				printk("\nADK-probe: Found a bulk in interface of size: %d\n", buffer_size);
			}
		}

		/* for BULK OUT endpoint */
		if (!dev->bulk_out_endpointAddr &&
		!(endpoint->bEndpointAddress & USB_DIR_IN) &&
		((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
			== USB_ENDPOINT_XFER_BULK)) {
		/* we found a bulk out endpoint */
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
			printk("\nADK-probe: Found a bulk out interface of address %x\n", dev->bulk_out_endpointAddr);
		}
	}

	if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr)) {
		printk("\nCould not find both bulk-in and bulk-out endpoints\n");
		//goto error;
	}

	/* setup into accessory */
	/*i = setup_accessory(
		dev,
		"Nexus-Computing GmbH",
		"Model",
		"Description",
		"VersionName",
		"http://neuxs-computing.ch",
		"SerialNo.");
	*/
	test = usb_rcvbulkpipe(dev->udev, 0);
	printk("\nADK-probe: bulkin number:%u\n", test);
	
	test = usb_sndbulkpipe(dev->udev, 0);
	printk("\nADK-probe: bulkout number:%u\n", test);
	
	test = usb_sndctrlpipe(dev->udev, 0);
	printk("\nADK-probe: ctrlpipe number:%u\n", test);
	
	test = usb_sndctrlpipe(dev->udev, 1);
	printk("\nADK-probe: ctrlpipe number:%u\n", test);
	
	test = usb_sndctrlpipe(dev->udev, 2);
	printk("\nADK-probe: ctrlpipe number:%u\n", test);
	
	i = usb_control_msg(
		dev->udev, //usb_device pointer
		usb_sndctrlpipe(dev->udev, 0), //pipe
		51, //request
		0xc0, //requesttype
		0x00, //value
		0x00, //index
		ioBuffer, //data
		2, //length
		10 //timeout
		);
	printk("\nADK-probe: accessory retup response: %d\n", i);

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
	int response;
	int tries = 15;

	printk("\nAccessory-Setup: accessory setup started\n");
	/* send control message */
	response = usb_control_msg(
		dev->udev, //usb_device pointer
		usb_sndctrlpipe(dev->udev, 0), //pipe
		51, //request
		0xC0, //requesttype
		0x00, //value
		0x00, //index
		ioBuffer, //data
		2, //length
		0 //timeout
		);
	printk("\nAccessory-Setup: accessory retup response: %d\n", response);

	return response;

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
