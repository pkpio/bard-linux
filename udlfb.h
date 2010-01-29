#ifndef UDLFB_H
#define UDLFB_H

/*
 * TODO: Use _IOWR() and fixed size struct
 * To do so, need to update matching X server
 */
#define DLFB_IOCTL_RETURN_EDID	 0xAD
#define DLFB_IOCTL_REPORT_DAMAGE 0xAA

struct urb_node {
	struct list_head entry;
	struct dlfb_data *dev;
	struct urb *urb;
};

struct urb_list {
	struct list_head list;
	spinlock_t lock;
	struct semaphore limit_sem;
	int available;
	int count;
	size_t size;
};

struct dlfb_data {
	struct usb_device *udev;
	struct device *gdev; /* &udev->dev */
	struct fb_info *info;
	struct urb_list urbs;
	struct kref kref;
	char *backing_buffer;
	struct delayed_work deferred_work;
	atomic_t fb_count;
	atomic_t usb_active; /* 0 = update virtual buffer, but no usb traffic */
	atomic_t lost_pixels; /* 1 = a render op failed. Need screen refresh */
	atomic_t defio_off; /* 1 = rely on ioctls and blit/copy/fill rects */
	char edid[128];
	int sku_pixel_limit;
	int base16;
	int base8;
	u32 pseudo_palette[256];
	/* blit-only rendering path metrics, exposed through sysfs */
	atomic_t bytes_rendered; /* raw pixel-bytes driver asked to render */
	atomic_t bytes_identical; /* saved effort with backbuffer comparison */
	atomic_t bytes_sent; /* to usb, after compression including overhead */
	atomic_t cpu_kcycles_used; /* transpired during pixel processing */
	/* interface usage metrics. Clients can call driver via several */
	atomic_t blit_count;
	atomic_t copy_count;
	atomic_t fill_count;
	atomic_t damage_count;
	atomic_t defio_fault_count;
};

#define NR_USB_REQUEST_I2C_SUB_IO 0x02
#define NR_USB_REQUEST_CHANNEL 0x12

/* -BULK_SIZE as per usb-skeleton. Can we get full page and avoid overhead? */
#define BULK_SIZE 512
#define MAX_TRANSFER (PAGE_SIZE*16 - BULK_SIZE)
#define WRITES_IN_FLIGHT (4)

#define GET_URB_TIMEOUT	HZ
#define FREE_URB_TIMEOUT (HZ*2)

#define BPP                     2
#define MAX_CMD_PIXELS		255
#define MIN_RLX_PIX_BYTES       4
#define MIN_RLX_CMD_BYTES	(7 + MIN_RLX_PIX_BYTES)

/* remove these once align.h patch is taken into kernel */
#define DL_ALIGN_UP(x,a) ALIGN(x,a)
#define DL_ALIGN_DOWN(x,a) ALIGN(x-(a-1), a)

/* KERN_ERR level for now - we need easy debug/feedback from all users still */
#define dl_err(format, arg...) \
	dev_err(dev->gdev, "dlfb: " format, ## arg)
#define dl_warn(format, arg...) \
	dev_err(dev->gdev, "dlfb: " format, ## arg)
#define dl_notice(format, arg...) \
	dev_err(dev->gdev, "dlfb: " format, ## arg)
#define dl_info(format, arg...) \
	dev_err(dev->gdev, "dlfb: " format, ## arg)
#endif
