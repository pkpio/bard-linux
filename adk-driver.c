#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>

/* Define these values to match your devices */
/* Values I found out for my phone */
#define USB_VENDOR_ID 0x04e8
#define USB_PRODUCT_ID 0x6860

/* table of devices that work with this driver */
static struct usb_device_id test_table [] = {
  { USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID) },
  { } /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, test_table);


static int testusb_probe (struct usb_interface *interface, const struct usb_device_id *id)
{
  printk("testusb: probe module\n");
  return 0;
}


static void testusb_disconnect (struct usb_interface *interface)
{
  printk("testusb: disconnect module\n");
}


static struct usb_driver testusb_driver = {
  .name = "testusb",
  .id_table = test_table,
  .probe = testusb_probe,
  .disconnect = testusb_disconnect,
};

static int __init testusb_init(void)
{
  int result;
  printk("testusb: Init called");

  result = usb_register(&testusb_driver);
  if (result) {
    printk("testusb: registering driver failed");
  }
  else {
     printk("testusb: driver registered successfully");
  }

  return result;
}


static void __exit testusb_exit(void)
{
   usb_deregister(&testusb_driver);
   printk("testusb: module deregistered");
}

module_init(testusb_init);
module_exit(testusb_exit);

MODULE_AUTHOR("Praveen Kumar");
MODULE_LICENSE("GPL");
