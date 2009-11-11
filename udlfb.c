/*****************************************************************************
 *                          DLFB Kernel Driver                               *
 *                            Version 0.4 (udlfb)                            *
 *             (C) 2009 Roberto De Ioris <roberto@unbit.it>                  *
 *                                                                           *
 *     This file is licensed under the GPLv2. See COPYING in the package.    *
 * Based on the amazing work of Florian Echtler and libdlo 0.1               *
 *                                                                           *
 *                                                                           *
 * xx.11.09 release 0.4 merge 0.3 work back into udlfb for kernel tree       *
 * 24.06.09 release 0.3 as displylink-mod (resolution manager, new ioctls)   *
 * 10.06.09 release 0.2.3 (edid ioctl, fallback for unsupported modes)       *
 * 05.06.09 release 0.2.2 (real screen blanking, rle compression, double buffer) *
 * 31.05.09 release 0.2                                                      *
 * 22.05.09 First public (ugly) release                                      *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#include <drm/drm_edid.h>
#else
#include "drm_edid.h"
#endif

#include "udlfb.h"

#define DRIVER_VERSION "DLFB 0.4"

/* memory functions taken from vfb */

static void *rvmalloc(unsigned long size)
{
	void *mem;
	unsigned long adr;

	size = PAGE_ALIGN(size);
	mem = vmalloc_32(size);
	if (!mem)
		return NULL;

	memset(mem, 0, size);	/* Clear the ram out, no junk to the user */
	adr = (unsigned long)mem;
	while (size > 0) {
		SetPageReserved(vmalloc_to_page((void *)adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return mem;
}

static void rvfree(void *mem, unsigned long size)
{
	unsigned long adr;

	if (!mem)
		return;

	adr = (unsigned long)mem;
	while ((long)size > 0) {
		ClearPageReserved(vmalloc_to_page((void *)adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	vfree(mem);
}

static int dlfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned long start = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long page, pos;

	printk("MMAP: %lu %u\n", offset + size, info->fix.smem_len);

	if (offset + size > info->fix.smem_len)
		return -EINVAL;

	pos = (unsigned long)info->fix.smem_start + offset;

	while (size > 0) {
		page = vmalloc_to_pfn((void *)pos);
		if (remap_pfn_range(vma, start, page, PAGE_SIZE, PAGE_SHARED))
			return -EAGAIN;

		start += PAGE_SIZE;
		pos += PAGE_SIZE;
		if (size > PAGE_SIZE)
			size -= PAGE_SIZE;
		else
			size = 0;
	}

	vma->vm_flags |= VM_RESERVED;	/* avoid to swap out this VMA */
	return 0;

}

/* ioctl structure */

struct dlores {
	int w,h;
	int freq;
};

struct dloarea {
	int x, y;
	int w, h;
	int x2, y2;
};

/*
static struct usb_device_id id_table [] = {
	{ USB_DEVICE(0x17e9, 0x023d) },
	{ }
};
*/

static struct usb_device_id id_table[] = {
	{.idVendor = 0x17e9, .match_flags = USB_DEVICE_ID_MATCH_VENDOR,},
	{},
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver dlfb_driver;

// thanks to Henrik Bjerregaard Pedersen for this function
static char *rle_compress16(uint16_t * src, char *dst, int rem)
{

	int rl;
	uint16_t pix0;
	char *end_if_raw = dst + 6 + 2 * rem;

	dst += 6;		// header will be filled in if RLE is worth it

	while (rem && dst < end_if_raw) {
		char *start = (char *)src;

		pix0 = *src++;
		rl = 1;
		rem--;
		while (rem && *src == pix0)
			rem--, rl++, src++;
		*dst++ = rl;
		*dst++ = start[1];
		*dst++ = start[0];
	}

	return dst;
}

/*
Thanks to Henrik Bjerregaard Pedersen for rle implementation and code refactoring.
Next step is huffman compression.
*/

static int
image_blit(struct dlfb_data *dev_info, int x, int y, int width, int height,
	   char *data)
{

	int i, j, base;
	int rem = width;
	int ret;

	int firstdiff, thistime;

	char *bufptr;

	if (dev_info->udev == NULL) {
		return 0;
	}

	if (x + width > dev_info->info->var.xres)
		return -EINVAL;

	if (y + height > dev_info->info->var.yres)
		return -EINVAL;

	mutex_lock(&dev_info->bulk_mutex);

	base =
	    dev_info->base16 + ((dev_info->info->var.xres * 2 * y) + (x * 2));

	data += (dev_info->info->var.xres * 2 * y) + (x * 2);

	/* printk("IMAGE_BLIT\n"); */

	bufptr = dev_info->buf;

	for (i = y; i < y + height; i++) {

		if (dev_info->bufend - bufptr < BUF_HIGH_WATER_MARK) {
			ret = dlfb_bulk_msg(dev_info, bufptr - dev_info->buf);
			bufptr = dev_info->buf;
		}

		rem = width;

		/* printk("WRITING LINE %d\n", i); */

		while (rem) {

			if (dev_info->bufend - bufptr < BUF_HIGH_WATER_MARK) {
				ret =
				    dlfb_bulk_msg(dev_info,
						  bufptr - dev_info->buf);
				bufptr = dev_info->buf;
			}
			// number of pixels to consider this time
			thistime = rem;
			if (thistime > 255)
				thistime = 255;

			// find position of first pixel that has changed
			firstdiff = -1;
			for (j = 0; j < thistime * 2; j++) {
				if (dev_info->backing_buffer
				    [base - dev_info->base16 + j] != data[j]) {
					firstdiff = j / 2;
					break;
				}
			}

			if (firstdiff >= 0) {
				char *end_of_rle;

				end_of_rle =
				    rle_compress16((uint16_t *) (data +
								 firstdiff * 2),
						   bufptr,
						   thistime - firstdiff);

				if (end_of_rle <
				    bufptr + 6 + 2 * (thistime - firstdiff)) {
					bufptr[0] = 0xAF;
					bufptr[1] = 0x69;

					bufptr[2] =
					    (char)((base +
						    firstdiff * 2) >> 16);
					bufptr[3] =
					    (char)((base + firstdiff * 2) >> 8);
					bufptr[4] =
					    (char)(base + firstdiff * 2);
					bufptr[5] = thistime - firstdiff;

					bufptr = end_of_rle;

				} else {
					// fallback to raw (or some other encoding?)
					*bufptr++ = 0xAF;
					*bufptr++ = 0x68;

					*bufptr++ =
					    (char)((base +
						    firstdiff * 2) >> 16);
					*bufptr++ =
					    (char)((base + firstdiff * 2) >> 8);
					*bufptr++ =
					    (char)(base + firstdiff * 2);
					*bufptr++ = thistime - firstdiff;
					// PUT COMPRESSION HERE
					for (j = firstdiff * 2;
					     j < thistime * 2; j += 2) {
						*bufptr++ = data[j + 1];
						*bufptr++ = data[j];
					}
				}
			}

			base += thistime * 2;
			data += thistime * 2;
			rem -= thistime;
		}

		memcpy(dev_info->backing_buffer + (base - dev_info->base16) -
		       (width * 2), data - (width * 2), width * 2);

		base += (dev_info->info->var.xres * 2) - (width * 2);
		data += (dev_info->info->var.xres * 2) - (width * 2);

	}

	if (bufptr > dev_info->buf) {
		dlfb_bulk_msg(dev_info, bufptr - dev_info->buf);
	}

	mutex_unlock(&dev_info->bulk_mutex);

	return base;

}

static int
draw_rect(struct dlfb_data *dev_info, int x, int y, int width, int height,
	  unsigned char red, unsigned char green, unsigned char blue)
{

	int i, j, base;
	int ret;
	unsigned short col =
	    (((((red) & 0xF8) | ((green) >> 5)) & 0xFF) << 8) +
	    (((((green) & 0x1C) << 3) | ((blue) >> 3)) & 0xFF);
	int rem = width;

	char *bufptr;

	if (x + width > dev_info->info->var.xres)
		return -EINVAL;

	if (y + height > dev_info->info->var.yres)
		return -EINVAL;

	mutex_lock(&dev_info->bulk_mutex);

	base = dev_info->base16 + (dev_info->info->var.xres * 2 * y) + (x * 2);

	bufptr = dev_info->buf;

	for (i = y; i < y + height; i++) {

		for (j = 0; j < width * 2; j += 2) {
			dev_info->backing_buffer[base - dev_info->base16 + j] =
			    (char)(col >> 8);
			dev_info->backing_buffer[base - dev_info->base16 + j +
						 1] = (char)(col);
		}
		if (dev_info->bufend - bufptr < BUF_HIGH_WATER_MARK) {
			ret = dlfb_bulk_msg(dev_info, bufptr - dev_info->buf);
			bufptr = dev_info->buf;
		}

		rem = width;

		while (rem) {

			if (dev_info->bufend - bufptr < BUF_HIGH_WATER_MARK) {
				ret =
				    dlfb_bulk_msg(dev_info,
						  bufptr - dev_info->buf);
				bufptr = dev_info->buf;
			}

			*bufptr++ = 0xAF;
			*bufptr++ = 0x69;

			*bufptr++ = (char)(base >> 16);
			*bufptr++ = (char)(base >> 8);
			*bufptr++ = (char)(base);

			if (rem > 255) {
				*bufptr++ = 255;
				*bufptr++ = 255;
				rem -= 255;
				base += 255 * 2;
			} else {
				*bufptr++ = rem;
				*bufptr++ = rem;
				base += rem * 2;
				rem = 0;
			}

			*bufptr++ = (char)(col >> 8);
			*bufptr++ = (char)(col);

		}

		base += (dev_info->info->var.xres * 2) - (width * 2);

	}

	if (bufptr > dev_info->buf)
		ret = dlfb_bulk_msg(dev_info, bufptr - dev_info->buf);

	mutex_unlock(&dev_info->bulk_mutex);

	return 1;
}

static int
copyarea(struct dlfb_data *dev_info, int dx, int dy, int sx, int sy,
	 int width, int height)
{
	int base;
	int source;
	int rem;
	int i, ret;

	char *bufptr;

	if (dx + width > dev_info->info->var.xres)
		return -EINVAL;

	if (dy + height > dev_info->info->var.yres)
		return -EINVAL;

	mutex_lock(&dev_info->bulk_mutex);

	base =
	    dev_info->base16 + (dev_info->info->var.xres * 2 * dy) + (dx * 2);
	source = (dev_info->info->var.xres * 2 * sy) + (sx * 2);

	bufptr = dev_info->buf;

	for (i = sy; i < sy + height; i++) {

		memcpy(dev_info->backing_buffer + base - dev_info->base16,
		       dev_info->backing_buffer + source, width * 2);

		if (dev_info->bufend - bufptr < BUF_HIGH_WATER_MARK) {
			ret = dlfb_bulk_msg(dev_info, bufptr - dev_info->buf);
			bufptr = dev_info->buf;
		}

		rem = width;

		while (rem) {

			if (dev_info->bufend - bufptr < BUF_HIGH_WATER_MARK) {
				ret =
				    dlfb_bulk_msg(dev_info,
						  bufptr - dev_info->buf);
				bufptr = dev_info->buf;
			}

			*bufptr++ = 0xAF;
			*bufptr++ = 0x6A;

			*bufptr++ = (char)(base >> 16);
			*bufptr++ = (char)(base >> 8);
			*bufptr++ = (char)(base);

			if (rem > 255) {
				*bufptr++ = 255;
				*bufptr++ = (char)(source >> 16);
				*bufptr++ = (char)(source >> 8);
				*bufptr++ = (char)(source);

				rem -= 255;
				base += 255 * 2;
				source += 255 * 2;

			} else {
				*bufptr++ = rem;
				*bufptr++ = (char)(source >> 16);
				*bufptr++ = (char)(source >> 8);
				*bufptr++ = (char)(source);

				base += rem * 2;
				source += rem * 2;
				rem = 0;
			}
		}

		base += (dev_info->info->var.xres * 2) - (width * 2);
		source += (dev_info->info->var.xres * 2) - (width * 2);
	}

	if (bufptr > dev_info->buf)
		ret = dlfb_bulk_msg(dev_info, bufptr - dev_info->buf);

	mutex_unlock(&dev_info->bulk_mutex);

	return 1;
}

static void dlfb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{

	struct dlfb_data *dev = info->par;

	mutex_lock(&dev->fb_mutex);

	dev = info->par;

	if (dev->udev == NULL)
		return;

	copyarea(dev, area->dx, area->dy, area->sx, area->sy, area->width,
		 area->height);

	mutex_unlock(&dev->fb_mutex);

	/* printk("COPY AREA %d %d %d %d %d %d !!!\n", area->dx, area->dy, area->sx, area->sy, area->width, area->height); */

}

static void dlfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	struct dlfb_data *dev = info->par;

	mutex_lock(&dev->fb_mutex);

	dev = info->par;

	if (dev->udev == NULL)
		return;

	/* printk("IMAGE BLIT (1) %d %d %d %d DEPTH %d {%p}!!!\n", image->dx, image->dy, image->width, image->height, image->depth, dev->udev); */

	cfb_imageblit(info, image);

	image_blit(dev, image->dx, image->dy, image->width, image->height,
		       info->screen_base);

	mutex_unlock(&dev->fb_mutex);

	/* printk("IMAGE BLIT (2) %d %d %d %d DEPTH %d {%p} %d!!!\n", image->dx, image->dy, image->width, image->height, image->depth, dev->udev, ret); */
}

static void dlfb_fillrect(struct fb_info *info,
			  const struct fb_fillrect *region)
{

	unsigned char red, green, blue;
	struct dlfb_data *dev = info->par;

	mutex_lock(&dev->fb_mutex);

	dev = info->par;

	if (dev->udev == NULL)
		return;

	memcpy(&red, &region->color, 1);
	memcpy(&green, &region->color + 1, 1);
	memcpy(&blue, &region->color + 2, 1);
	draw_rect(dev, region->dx, region->dy, region->width, region->height,
		  red, green, blue);

	mutex_unlock(&dev->fb_mutex);

	/* printk("FILL RECT %d %d !!!\n", region->dx, region->dy); */

}

static int dlfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{

	struct dlfb_data *dev_info = info->par;
	struct dloarea *area = NULL;
	struct dlores *res = NULL;
	char *name;

	if (dev_info->udev == NULL) {
		return -EINVAL;
	}

	if (cmd == 0xAD) {
		char *edid = (char *)arg;
		dlfb_edid(dev_info);
		if (copy_to_user(edid, dev_info->edid, 128)) {
			return -EFAULT;
		}
		return 0;
	}

	if (cmd == 0xAA || cmd == 0xAB || cmd == 0xAC) {

		area = (struct dloarea *)arg;

		if (area->x < 0)
			area->x = 0;

		if (area->x > info->var.xres)
			area->x = info->var.xres;

		if (area->y < 0)
			area->y = 0;

		if (area->y > info->var.yres)
			area->y = info->var.yres;
	}

	if (cmd == 0xAA) {
		image_blit(dev_info, area->x, area->y, area->w, area->h,
			   info->screen_base);
	} else if (cmd == 0xAB) {

		if (area->x2 < 0)
			area->x2 = 0;

		if (area->y2 < 0)
			area->y2 = 0;

		copyarea(dev_info,
			 area->x2, area->y2, area->x, area->y, area->w,
			 area->h);
	}
	else if (cmd == 0xAE) {
		res = (struct dlores *) arg;
		dlfb_set_video_mode(dev_info,0, res->w, res->h, res->freq);

	}
	else if (cmd == 0xAF) {
		name = (char *) arg;
		if (copy_to_user(name, dev_info->name, 64)) {
			return -EFAULT;
		}
		return 0;
	}
	else if (cmd == 0xB0) {
		name = (char *) arg;
		if (copy_to_user(name, "displaylink", 11)) {
			return -EFAULT;
		}
	}
	return 0;
}

/* taken from vesafb */

static int
dlfb_setcolreg(unsigned regno, unsigned red, unsigned green,
	       unsigned blue, unsigned transp, struct fb_info *info)
{
	int err = 0;

	if (regno >= info->cmap.len)
		return 1;

	if (regno < 16) {
		if (info->var.red.offset == 10) {
			/* 1:5:5:5 */
			((u32 *) (info->pseudo_palette))[regno] =
			    ((red & 0xf800) >> 1) |
			    ((green & 0xf800) >> 6) | ((blue & 0xf800) >> 11);
		} else {
			/* 0:5:6:5 */
			((u32 *) (info->pseudo_palette))[regno] =
			    ((red & 0xf800)) |
			    ((green & 0xfc00) >> 5) | ((blue & 0xf800) >> 11);
		}
	}

	return err;
}

static int dlfb_release(struct fb_info *info, int user)
{
	struct dlfb_data *dev_info = info->par;

	BUG_ON(dev_info == NULL);

	/* fbcon control */
	if (user == 0) {
		return 0;
	}
		
	//printk("releasing displaylink framebuffer...\n");
	mutex_lock(&dev_info->fb_mutex);

	atomic_dec(&dev_info->fb_count);

	//printk("release fb count: %d\n", atomic_read(&dev_info->fb_count)); 

	if (atomic_read(&dev_info->fb_count) == 0 && dev_info->udev == NULL) {
		dlfb_destroy_framebuffer(dev_info);
		mutex_unlock(&dev_info->fb_mutex);
		kfree(dev_info);
		return 0 ;	
	}

	image_blit(dev_info, 0, 0, info->var.xres, info->var.yres,
		   info->screen_base);

	mutex_unlock(&dev_info->fb_mutex);

	return 0;
}

static int dlfb_blank(int blank_mode, struct fb_info *info)
{
	struct dlfb_data *dev_info = info->par;
	char *bufptr = dev_info->buf;

	bufptr = dlfb_set_register(bufptr, 0xFF, 0x00);
	if (blank_mode != FB_BLANK_UNBLANK) {
		bufptr = dlfb_set_register(bufptr, 0x1F, 0x01);
	} else {
		bufptr = dlfb_set_register(bufptr, 0x1F, 0x00);
	}
	bufptr = dlfb_set_register(bufptr, 0xFF, 0xFF);

	dlfb_bulk_msg(dev_info, bufptr - dev_info->buf);

	return 0;
}



static int dlfb_open(struct fb_info *info, int user) {

	struct dlfb_data *dev = info->par;

	BUG_ON(dev == NULL);

	/* fbcon can survive disconnection, no refcount needed */
	if (user == 0) {
		return 0;
	}


	//printk("opening displaylink framebuffer...\n");
	mutex_lock(&dev->fb_mutex);

	//printk("application %s %d is trying to open displaylink device\n", current->comm, user);

	if (dev->udev == NULL) {
		mutex_unlock(&dev->fb_mutex);
		return -1;
	}

	atomic_inc(&dev->fb_count);

	//printk("fb count: %d\n", atomic_read(&dev->fb_count)); 

	mutex_unlock(&dev->fb_mutex);

	return 0;
	
}



static int dlfb_setpar(struct fb_info *info) {
	
	struct dlfb_data *dev = info->par;
	
	BUG_ON(dev == NULL);

	if (dev->udev == NULL)
                return -EINVAL;

	printk("setting hardware to %d %d\n", info->var.xres, info->var.yres);

	dlfb_set_video_mode(dev, 0, info->var.xres, info->var.yres, 0);

	info->fix.line_length = dev->line_length;

	return 0;
}



static int dlfb_checkvar(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct dlfb_data *dev = info->par;
	struct edid *edid ;
	struct detailed_timing *best_edid ;
	struct std_timing *std_edid ;

	int i ;

	BUG_ON(dev == NULL);

	if (dev->udev == NULL)
		return -EINVAL;

	edid = (struct edid *) dev->edid ;

	printk("checking for resolution %d %d\n", var->xres, var->yres);

	for(i=0;i<4;i++) {
		best_edid = (struct detailed_timing *) &edid->detailed_timings[i];
		if (EDID_GET_WIDTH(best_edid) == 0)
			break;
		printk("edid %dX%d\n", EDID_GET_WIDTH(best_edid), EDID_GET_HEIGHT(best_edid));
		if (EDID_GET_WIDTH(best_edid) == var->xres && EDID_GET_HEIGHT(best_edid) == var->yres) {
			printk("found valid resolution for displaylink device\n");
			return 0;
		}
	}

	for(i=0;i<8;i++) {
		std_edid = (struct std_timing *) &edid->standard_timings[i];
		if ((std_edid->hsize*8)+248 < 320) {
			break;
		}
		printk("edid (std) %d %d %d %d\n", (std_edid->hsize*8)+248, (((std_edid->hsize*8)+248)/4)*3, std_edid->vfreq+60, std_edid->aspect_ratio);
		if ((std_edid->hsize*8)+248 == var->xres && (((std_edid->hsize*8)+248)/4)*3 == var->yres) {
			printk("found valid resolution for displaylink device\n");
			return 0;
		}
	}
	
	
	return -EINVAL;
	
}

static struct fb_ops dlfb_ops = {
	.fb_setcolreg = dlfb_setcolreg,
	.fb_fillrect = dlfb_fillrect,
	.fb_copyarea = dlfb_copyarea,
	.fb_imageblit = dlfb_imageblit,
	.fb_mmap = dlfb_mmap,
	.fb_ioctl = dlfb_ioctl,
	.fb_release = dlfb_release,
	.fb_blank = dlfb_blank,
	.fb_open = dlfb_open,
	.fb_check_var = dlfb_checkvar,
	.fb_set_par = dlfb_setpar,
};

static int
dlfb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct dlfb_data *dev;

	int ret;
	int mode = 0;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		printk("cannot allocate dev structure.\n");
		return -ENOMEM;
	}

	mutex_init(&dev->bulk_mutex);
	mutex_init(&dev->fb_mutex);

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	printk("DisplayLink device attached\n");

	/* add framebuffer info to usb interface */
	usb_set_intfdata(interface, dev);

	dev->buf = kmalloc(BUF_SIZE, GFP_KERNEL);
	/* usb_buffer_alloc(dev->udev, BUF_SIZE , GFP_KERNEL, &dev->tx_urb->transfer_dma); */

	if (dev->buf == NULL) {
		printk("unable to allocate memory for dlfb commands\n");
		goto out;
	}
	dev->bufend = dev->buf + BUF_SIZE;

	dev->tx_urb = usb_alloc_urb(0, GFP_KERNEL);
	usb_fill_bulk_urb(dev->tx_urb, dev->udev,
			  usb_sndbulkpipe(dev->udev, 1), dev->buf, 0,
			  dlfb_bulk_callback, dev);

	if (strlen(dev->udev->product) > 63) {
                memcpy(dev->name, dev->udev->product, 63);
        }
        else {
                memcpy(dev->name, dev->udev->product, strlen(dev->udev->product));
        }

	dlfb_edid(dev);
	ret = dlfb_setup(dev);


	dlfb_set_video_mode(dev, 0, 0, 0, 0);

	dev->backing_buffer = kzalloc(dev->screen_size, GFP_KERNEL);

	if (!dev->backing_buffer) {
		printk("non posso allocare il backing buffer\n");
		goto out;
	}

	ret = dlfb_activate_framebuffer(dev, mode);

	if (ret !=0) {
		printk("unable to allocate framebuffer\n");
		goto out;
	}

	// put the green screen
	draw_rect(dev, 0, 0, dev->info->var.xres,
		  dev->info->var.yres, 0x30, 0xff, 0x30);

	return 0;

 out:
	usb_set_intfdata(interface, NULL);
	usb_put_dev(dev->udev);
	kfree(dev);
	return -ENOMEM;

}

/* taken from libdlo */
static uint16_t lfsr16(uint16_t v)
{
  uint32_t _v   = 0xFFFF;

	v = cpu_to_le16(v);

  while (v--) {
    _v = ((_v << 1) | (((_v >> 15) ^ (_v >> 4) ^ (_v >> 2) ^ (_v >> 1)) & 1)) & 0xFFFF;
  }
  return (uint16_t) _v;
}



/* displaylink functions */
void dlfb_bulk_callback(struct urb *urb)
{

	struct dlfb_data *dev = urb->context;
	complete(&dev->done);
	//printk("displaylink BULK transfer complete %d\n", urb->actual_length);

}

void dlfb_edid(struct dlfb_data *dev)
{
	int i;
	int ret;
	char rbuf[2];

	for (i = 0; i < 128; i++) {
                ret =
                    usb_control_msg(dev->udev,
                                    usb_rcvctrlpipe(dev->udev, 0), (0x02),
                                    (0x80 | (0x02 << 5)), i << 8, 0xA1, rbuf, 2,
                                    0);
                //printk("ret control msg edid %d: %d [%d]\n",i, ret, rbuf[1]);
                dev->edid[i] = rbuf[1];
        }

}

void dlfb_get_best_edid(struct dlfb_data *dev) {
	return;
}


int dlfb_bulk_msg(struct dlfb_data *dev, int len)
{

	int ret;

	init_completion(&dev->done);

	dev->tx_urb->actual_length = 0;
	dev->tx_urb->transfer_buffer_length = len;

	ret = usb_submit_urb(dev->tx_urb, GFP_KERNEL);
	if (!wait_for_completion_timeout(&dev->done, 1000)) {
		usb_kill_urb(dev->tx_urb);
		printk("usb timeout !!!\n");
	}

	return dev->tx_urb->actual_length;

}

static char *dlfb_set_register_16(char *bufptr, uint8_t reg, uint16_t val)
{

	bufptr = dlfb_set_register(bufptr, reg, val >> 8);
	bufptr = dlfb_set_register(bufptr, reg+1, val & 0xFF);

	return bufptr;
}

static char *dlfb_set_register_le16(char *bufptr, uint8_t reg, uint16_t val)
{

	bufptr = dlfb_set_register(bufptr, reg, val & 0xFF);
	bufptr = dlfb_set_register(bufptr, reg+1, val >> 8);

	return bufptr;
}

char *dlfb_edid_to_reg(struct detailed_timing *edid, char *bufptr, int width, int height, int freq) {

	uint16_t edid_w  ;
	uint16_t edid_h ;
	uint16_t edid_x_ds ;
	uint16_t edid_x_de ;
	uint16_t edid_y_ds ;
	uint16_t edid_y_de ;
	uint16_t edid_x_ec ;
	uint16_t edid_h_se ;
	uint16_t edid_y_ec ;
	uint16_t edid_v_se ;
	uint16_t edid_pclock ;

	/* display width */
	edid_w = EDID_GET_WIDTH(edid) ;
	if (width!= 0) {
		edid_w = width;	
	}

	/* display height */
	edid_h  = EDID_GET_HEIGHT(edid) ;
	if (height!= 0) {
		edid_h = height;	
	}

	/* display x start/end */
	edid_x_ds = (EDID_GET_HBLANK(edid) - EDID_GET_HSYNC(edid)) ;
	edid_x_de = (edid_x_ds + edid_w) ;

	/* display y start/end */
	edid_y_ds = (EDID_GET_VBLANK(edid) - EDID_GET_VSYNC(edid));
	edid_y_de = (edid_y_ds + edid_h);

	/* x end count */
	edid_x_ec = (edid_w + EDID_GET_HBLANK(edid) - 1);
	edid_h_se = (EDID_GET_HPULSE(edid) + 1) ;

	/* y end count */
	edid_y_ec = (edid_h + EDID_GET_VBLANK(edid)) ;
	edid_v_se = (EDID_GET_VPULSE(edid)) ;


	/* pixel clock */
	edid_pclock = edid->pixel_clock*2;

	if (freq != 0) {
		/* calc new pixel clock based on freq */
	}


	bufptr = dlfb_set_register_16(bufptr, 0x01, lfsr16(edid_x_ds)) ;
	bufptr = dlfb_set_register_16(bufptr, 0x03, lfsr16(edid_x_de)) ;
	bufptr = dlfb_set_register_16(bufptr, 0x05, lfsr16(edid_y_ds)) ;
	bufptr = dlfb_set_register_16(bufptr, 0x07, lfsr16(edid_y_de)) ;

	bufptr = dlfb_set_register_16(bufptr, 0x09, lfsr16(edid_x_ec)) ;

	bufptr = dlfb_set_register_16(bufptr, 0x0B, lfsr16(1)) ;
	bufptr = dlfb_set_register_16(bufptr, 0x0D, lfsr16(edid_h_se)) ;

	bufptr = dlfb_set_register_16(bufptr, 0x0F, edid_w) ;

	bufptr = dlfb_set_register_16(bufptr, 0x11, lfsr16(edid_y_ec)) ;

	bufptr = dlfb_set_register_16(bufptr, 0x13, lfsr16(0)) ;
	bufptr = dlfb_set_register_16(bufptr, 0x15, lfsr16(edid_v_se)) ;

	bufptr = dlfb_set_register_16(bufptr, 0x17, edid_h) ;

	bufptr = dlfb_set_register_le16(bufptr, 0x1B, edid_pclock) ;

	return bufptr;
}

char *dlfb_set_register(char *bufptr, uint8_t reg, uint8_t val)
{

	*bufptr++ = 0xAF;
	*bufptr++ = 0x20;
	*bufptr++ = reg;
	*bufptr++ = val;

	return bufptr;

}

int dlfb_set_video_mode(struct dlfb_data *dev, int mode, int width, int height, int freq)
{

	char *bufptr;
	int ret;

	struct edid *edid = (struct edid *) dev->edid;
	struct detailed_timing *best_edid =  &edid->detailed_timings[mode];	

	if (dev->udev == NULL)
		return 0;

	dev->base16 = 0;
	dev->base8 = dev->screen_size;

	bufptr = dev->buf;

	mutex_lock(&dev->bulk_mutex);

	// set registers
	bufptr = dlfb_set_register(bufptr, 0xFF, 0x00);

	// set color depth
	bufptr = dlfb_set_register(bufptr, 0x00, 0x00);

	// set addresses
	bufptr =
		dlfb_set_register(bufptr, 0x20,
		(char)(dev->base16 >> 16));
	bufptr =
		dlfb_set_register(bufptr, 0x21,
		(char)(dev->base16 >> 8));
	bufptr =
		dlfb_set_register(bufptr, 0x22,
		(char)(dev->base16));

	bufptr =
		dlfb_set_register(bufptr, 0x26,
		(char)(dev->base8 >> 16));

	bufptr =
		dlfb_set_register(bufptr, 0x27,
		(char)(dev->base8 >> 8));

	bufptr =
		dlfb_set_register(bufptr, 0x28,
		(char)(dev->base8));

	if (width != 0) {
		printk("displaylink setting resolution to %dx%d\n", width, height);
	}
	
	bufptr = dlfb_edid_to_reg(best_edid, bufptr, width, height, freq);


	// blank
	bufptr = dlfb_set_register(bufptr, 0x1F, 0x00);

	// end registers
	bufptr = dlfb_set_register(bufptr, 0xFF, 0xFF);

	// send
	ret = dlfb_bulk_msg(dev, bufptr - dev->buf);
	printk("set video mode bulk message: %d %d\n", ret,
			       bufptr - dev->buf);

	// flush
	ret = dlfb_bulk_msg(dev, 0);
	printk("displaylink register flush: %d\n", ret);

	if (width == 0) {
		dev->line_length = EDID_GET_WIDTH(best_edid) * (FB_BPP/8) ;
	}
	else {
		dev->line_length = width * (FB_BPP/8) ;
	}

	mutex_unlock(&dev->bulk_mutex);

	return 0;

}

int dlfb_setup(struct dlfb_data *dev) 
{

	int ret;
	unsigned char buf[4];

	struct edid *edid = (struct edid *) dev->edid;

	struct detailed_timing *best_edid =  &edid->detailed_timings[0] ;

	ret = usb_control_msg(dev->udev,
                                    usb_rcvctrlpipe(dev->udev, 0), (0x02),
                                    (0x80 | (0x02 << 5)), 0, 0, buf, 4,
                                    5000);

	if (ret != 4) {
		return -1 ;
	}

	switch ((buf[3] >> 4) & 0xF) {
		case DL_CHIP_TYPE_BASE:
			strcpy(dev->chiptype, "base");
			break;
		case DL_CHIP_TYPE_ALEX:
			strcpy(dev->chiptype, "alex");
			break;
		case DL_CHIP_TYPE_OLLIE:
			strcpy(dev->chiptype, "ollie");
			break;
		default:
			if (buf[3] == DL_CHIP_TYPE_OLLIE)
				strcpy(dev->chiptype, "ollie");
			else
				strcpy(dev->chiptype, "unknown");
	}

	printk("found DisplayLink Chip %s\n", dev->chiptype);

	// set encryption key (null)
        memcpy(dev->buf, STD_CHANNEL, 16);
        ret = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		0x12, (0x02 << 5), 0, 0,
		dev->buf, 16, 0);

	printk("sent encryption null key: %d\n", ret);

        dev->line_length = EDID_GET_WIDTH(best_edid) * (FB_BPP / 8);

	printk("displaylink monitor info: W(%d) H(%d) clock(%d)\n",
		EDID_GET_WIDTH(best_edid),
		EDID_GET_HEIGHT(best_edid),
		best_edid->pixel_clock
		);

	dev->screen_size = EDID_GET_WIDTH(best_edid) * 
			EDID_GET_HEIGHT(best_edid) * (FB_BPP / 8);

	return 0;
}

int dlfb_activate_framebuffer(struct dlfb_data *dev, int mode)
{

	struct fb_info *info;

	info = framebuffer_alloc(sizeof(u32) * 256, &dev->udev->dev);

	if (!info) {
		printk("unable to allocate displaylink virtual framebuffer");
		return -ENOMEM;
	}

	dev->info = info;

	info->pseudo_palette = info->par;
	info->par = dev;

	info->flags =
	    FBINFO_DEFAULT | FBINFO_READS_FAST | FBINFO_HWACCEL_IMAGEBLIT |
	    FBINFO_HWACCEL_COPYAREA | FBINFO_HWACCEL_FILLRECT;
	info->fbops = &dlfb_ops;
	info->screen_base = rvmalloc(dev->screen_size);

	if (info->screen_base == NULL) {
		printk
		    ("cannot allocate framebuffer virtual memory of %d bytes\n",
		     dev->screen_size);
		goto out0;
	}

	fb_parse_edid(dev->edid, &info->var);

	info->var.bits_per_pixel = 16;
	info->var.activate = FB_ACTIVATE_TEST;
	info->var.vmode = FB_VMODE_NONINTERLACED;

	info->var.red.offset = 11;
	info->var.red.length = 5;
	info->var.red.msb_right = 0;

	info->var.green.offset = 5;
	info->var.green.length = 6;
	info->var.green.msb_right = 0;

	info->var.blue.offset = 0;
	info->var.blue.length = 5;
	info->var.blue.msb_right = 0;

	info->fix.smem_start = (unsigned long)info->screen_base;
	info->fix.smem_len = PAGE_ALIGN(dev->screen_size);
	
	if (strlen(dev->udev->product) > 15) {
		memcpy(info->fix.id, dev->udev->product, 15);
	}
	else {
		memcpy(info->fix.id, dev->udev->product, strlen(dev->udev->product));
	}
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.accel = info->flags;
	info->fix.line_length = dev->line_length;


	if (fb_alloc_cmap(&info->cmap, 256, 0) < 0)
		goto out1;

	printk("colormap allocated\n");
	if (register_framebuffer(info) < 0)
		goto out2;

	return 0;

 out2:
	fb_dealloc_cmap(&info->cmap);
 out1:
	rvfree(info->screen_base, dev->screen_size);
 out0:
	framebuffer_release(info);

	return -1 ;

}

void dlfb_destroy_framebuffer(struct dlfb_data *dev) 
{

	printk("destroying framebuffer device...\n");
	unregister_framebuffer(dev->info);
	printk("unregistering...\n");
	fb_dealloc_cmap(&dev->info->cmap);
	printk("deallocating cmap...\n");
	rvfree(dev->info->screen_base, dev->screen_size);
	printk("deallocating screen\n");
	framebuffer_release(dev->info);
	printk("...done\n");
}



static void dlfb_disconnect(struct usb_interface *interface)
{
	struct dlfb_data *dev = usb_get_intfdata(interface);
	struct dlfb_orphaned_dev *odev;

	mutex_unlock(&dev->bulk_mutex);

	usb_kill_urb(dev->tx_urb);
	usb_free_urb(dev->tx_urb);
	usb_set_intfdata(interface, NULL);
	usb_put_dev(dev->udev);

	mutex_lock(&dev->fb_mutex);

	printk("fb count: %d\n", atomic_read(&dev->fb_count)); 

	if (atomic_read(&dev->fb_count) == 0) {
		dlfb_destroy_framebuffer(dev);
	}
	else {
		printk("the framebuffer associated to this displaylink device is still in use. postponing deallocation...\n");
		// mark the framebuffer for destruction
		odev = kzalloc(sizeof(*odev), GFP_KERNEL);
		atomic_set(&odev->fb_count, atomic_read(&dev->fb_count) );
		odev->udev = NULL;
		mutex_init(&odev->fb_mutex);
		odev->info = dev->info;
		odev->info->par = odev;
		odev->screen_size = dev->screen_size;
		odev->line_length = dev->line_length;
		printk("%d clients are still connected to this framebuffer device\n", atomic_read(&odev->fb_count));
	}

	mutex_unlock(&dev->fb_mutex);

	kfree(dev);

	printk("DisplayLink device disconnected\n");
}

static struct usb_driver dlfb_driver = {
	.name = "udlfb",
	.probe = dlfb_probe,
	.disconnect = dlfb_disconnect,
	.id_table = id_table,
};

static int __init dlfb_init(void)
{
	int res;

	res = usb_register(&dlfb_driver);
	if (res)
		err("usb_register failed. Error number %d", res);

	printk("VMODES initialized\n");

	return res;
}

static void __exit dlfb_exit(void)
{
	usb_deregister(&dlfb_driver);
}

module_init(dlfb_init);
module_exit(dlfb_exit);

MODULE_AUTHOR("Roberto De Ioris <roberto@unbit.it>");
MODULE_DESCRIPTION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
