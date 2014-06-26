#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "devices.h"
#include "adk.h"

#define DEBUG_TAG "BARD"
#define BARD_MINOR_BASE	96 /* Base address for the minor number */

/* table of devices that work with this driver */
static struct usb_device_id adk_devices [] = {
	{ USB_DEVICE(USB_GOO_VID, USB_N10_PID) },
	{ USB_DEVICE(USB_SAM_VID, USB_GS2_PID) },
	{ USB_DEVICE(USB_GOO_VID, AOA_ACC_PID) },
	{ USB_DEVICE(USB_SAM_VID, AOA_ACC_PID) },
	{ USB_DEVICE(USB_GOO_VID, USB_N10_DEB_PID) },
	{ USB_DEVICE_AND_INTERFACE_INFO(USB_GOO_VID, AOA_ACC_ADB_PID, 
					0xff, 0xff, 0x00) },
	{ } /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, adk_devices);

static struct usb_driver bard_driver;

static int
print (char *msg)
{
	return printk("%s : %s\n", DEBUG_TAG, msg);
}

static inline void
adk_delete (struct adk_device *dev)
{
	print("adk_delete: deleting dev");
	kfree(dev);
}

static int
adk_open (struct inode *inode, struct file *file)
{
	struct adk_device *dev = NULL;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	print("adk_open: Device opened");
	subminor = iminor(inode);

	interface = usb_find_interface(&bard_driver, subminor);
	if (!interface) {
		printk("Can't find device for minor %d!", subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if (!dev) {
		print("Device NULL!");
		retval = -ENODEV;
		goto exit;
	}

	file->private_data = dev;

exit:
	return retval;
}

static int
adk_release (struct inode *inode, struct file *file)
{
	struct adk_device *dev = NULL;
	int retval = 0;

	print("adk_release: Device released");
	dev = file->private_data;

	if (!dev) {
		print("Dev is NULL!");
		retval = -ENODEV;
		goto exit;
	}

	if (!dev->udev) {
		print("Device unplugged before the file was released!");
		goto exit;
	}

exit:
	return retval;
}

static ssize_t
adk_read (struct file *file, char __user *user_buf,
	 size_t count, loff_t *ppos)
{
	int retval = 0;
	struct adk_device *dev;
	
	print("adk_write: Reading from device");
	
	dev = file->private_data;
	
	/* do a blocking bulk read to get data from the device */
	retval = usb_bulk_msg(dev->udev,
		      usb_rcvbulkpipe(dev->udev, dev->bulk_in_add),
		      dev->bulk_in_buffer,
		      min(dev->bulk_in_size, count),
		      &count, HZ*20);
	
	printk("BARD: Bulk read return code: %d\n", retval);

	/* if the read was successful, copy the data to user space */
	if (!retval) {
	    if (copy_to_user(user_buf, dev->bulk_in_buffer, count))
		retval = -EFAULT;
	    else
		retval = count;
	}
	
	return retval;
}

static ssize_t
adk_write (struct file *file, const char __user *user_buf,
	 size_t count, loff_t *ppos)
{
	struct adk_device *dev;
	int retval = 0;
	u8 buffer[8];
	static int transferred = 0;
	int i = 0;
	unsigned long test = 0;

	dev = file->private_data;
	
	print("adk_write: Writing to device");

	/* Verify that the device wasn't unplugged. */
	if (!dev->udev) {
		retval = -ENODEV;
		printk("No device or device unplugged (%d)", retval);
		goto exit;
	}

	if (count == 0)
		goto exit;
		
	memset(&buffer, 0, sizeof(buffer));
	if(copy_from_user(buffer, user_buf, min(sizeof(buffer), count))){
		retval = -EFAULT;
		goto exit;
	}
		
	/* do a blocking bulk write to the device */	
	retval = usb_bulk_msg(dev->udev,
		      usb_sndbulkpipe(dev->udev, dev->bulk_out_add),
		      buffer, count, &transferred, HZ*5);
	
	
	printk("Bulk transfer return code: %d\n", retval);
	printk("Actual length is: %d\n", transferred);

exit:
	return retval;
}

static struct file_operations adk_fops = {
	.owner =	THIS_MODULE,
	.write =	adk_write,
	.read =		adk_read,
	.open =		adk_open,
	.release =	adk_release,
};

static struct usb_class_driver bard_usb_class = {
	.name = "bard%d",
	.fops = &adk_fops,
	.minor_base = BARD_MINOR_BASE,
};

static void
set_bulk_address (
	struct adk_device *dev,
	struct usb_interface *interface)
{
	struct usb_endpoint_descriptor *endpoint;
	struct usb_host_interface *iface_desc;
	int i;
	
	iface_desc = interface->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;
		
		/* check for bulk endpoint */
		if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) 
			== USB_ENDPOINT_XFER_BULK){
			
			/* bulk in */
			if(endpoint->bEndpointAddress & USB_DIR_IN) {
				dev->bulk_in_add = endpoint->bEndpointAddress;
				dev->bulk_in_size = endpoint->wMaxPacketSize;
				dev->bulk_in_buffer = kmalloc(dev->bulk_in_size,
							 	GFP_KERNEL);
				if (!dev->bulk_in_buffer)
					print("Could not allocate bulk buffer");
			}
			
			/* bulk out */
			else
				dev->bulk_out_add = endpoint->bEndpointAddress;	
		}
	}
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
	
	printk("Product id is: %04x \n", id->idProduct);

	memset(dev, 0x00, sizeof(*dev));
	
	dev->udev = interface_to_usbdev(interface);
	if (!dev->udev) {
		retval = -ENODEV;
		goto exit;
	}
	
	/* Device attached in normal mode */
	if(id->idProduct != AOA_ACC_ADB_PID){
		retval = setup_accessory(dev, ADK_MAN, ADK_MOD, ADK_DES, 
			 		ADK_VER, ADK_URI, ADK_SER);
	}
	/* Device attached in accessory mode */
	else{
		print("Attached as an accessory");
		dev->interface = interface;
		kref_init(&dev->kref);
		
		/* save our data pointer in this interface device. */
		usb_set_intfdata(interface, dev);
		
		kref_init(&dev->kref);
		set_bulk_address(dev, interface);
		printk("Bulk values: %04x, %04x\n", dev->bulk_in_add, dev->bulk_out_add);
		
		/* register the device now */
		retval = usb_register_dev(interface, &bard_usb_class);
		if (retval) {
			usb_set_intfdata(interface, NULL);
			goto error;
		}

		dev->minor = interface->minor;

		printk("New ADK device attached to /dev/bard%d\n",
				interface->minor - BARD_MINOR_BASE);
		
	}

exit:
	return retval;

error:
	adk_delete(dev);
	return retval;
}

static void
bard_disconnect (struct usb_interface *interface){
	struct adk_device *dev;
	int minor=0;

	dev = usb_get_intfdata(interface);
	//usb_set_intfdata(interface, NULL);

	//minor = dev->minor;

	/* Give back our minor. */
	if(dev != NULL)
		usb_deregister_dev(interface, &bard_usb_class);
	else
		printk("bard_disconnect: Dev is null!\n");
	//adk_delete(dev);

	printk("ADK device /dev/adk%d now disconnected\n",
		minor - BARD_MINOR_BASE);
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

MODULE_AUTHOR("Praveen Kumar Pendyala");
MODULE_LICENSE("GPL");
