#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>

/* Define these values to match your devices */
#define USB_VENDOR_ID 0x04e8
#define USB_PRODUCT_ID 0x6860


static int testusb_probe (struct usb_interface *interface, const struct usb_device_id *id){
  printk("\ntestusb: probe module\n");
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
  int result = 1;
  printk("\ntestusb: Init called\n");

  result = usb_register(&testusb_driver);
  if (result) {
    printk("\ntestusb: registering driver failed\n");
  }
  else {
     printk("\ntestusb: driver registered successfully\n");
  }

  printk("\nResult is: " + result);

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
