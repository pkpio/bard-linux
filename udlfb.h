#ifndef UDLFB_H
#define UDLFB_H

struct dlfb_data {
	struct usb_device *udev;
	struct fb_info *info;
	struct semaphore limit_sem;
	struct completion write_completion_routine;
	struct kref kref;
	char *backing_buffer;
	atomic_t fb_count;
	atomic_t usb_active; /* 0 = update virtual buffer, but no usb traffic */
	atomic_t lost_pixels; /* 1 = a render op failed. Need screen refresh */
	char edid[128];
	int sku_pixel_limit;
	int base16;
	int base16d;
	int base8;
	int base8d;
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

#endif
