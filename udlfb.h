#ifndef UDLFB_H
#define UDLFB_H

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

/* USB dlfb-specific helper functions */
static int dlfb_sync_bulk_msg(struct dlfb_data *dev, void *buf, int len);
static void dlfb_urb_completion(struct urb *urb);
static struct urb* dlfb_get_urb(struct dlfb_data *dev);
static int dlfb_submit_urb(struct dlfb_data *dev, struct urb * urb, size_t len);
static int dlfb_alloc_urb_list(struct dlfb_data *dev, int count, size_t size);
static void dlfb_free_urb_list(struct dlfb_data *dev);

/* Framebuffer deferred IO functions */
static void dlfb_dpy_deferred_io(struct fb_info *info,
				 struct list_head *pagelist);

/* Internal drawing functions */
static int trim_hline(const u8* bback, const u8 **bfront, int *width_bytes);
static void render_hline(
	const uint16_t* *pixel_start_ptr,
	const uint16_t*	const pixel_end,
	uint32_t *device_address_ptr,
	uint8_t **command_buffer_ptr,
	const uint8_t* const cmd_buffer_end);
static void dlfb_render_hline(struct dlfb_data *dev, struct urb **urb_ptr,
			      const char *front, char **urb_buf_ptr,
			      u32 byte_offset, u32 byte_width,
			      int *ident_ptr, int *sent_ptr);

static void dlfb_get_edid(struct dlfb_data *dev_info)
{
	int i;
	int ret;
	char rbuf[2];

	for (i = 0; i < 128; i++) {
		ret =
		    usb_control_msg(dev_info->udev,
				    usb_rcvctrlpipe(dev_info->udev, 0), (0x02),
				    (0x80 | (0x02 << 5)), i << 8, 0xA1, rbuf, 2,
				    0);
		/*printk("ret control msg edid %d: %d [%d]\n",i, ret, rbuf[1]); */
		dev_info->edid[i] = rbuf[1];
	}

}

#define dlfb_set_register insert_command

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
