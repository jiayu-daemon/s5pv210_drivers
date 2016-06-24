#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gfp.h>
#include <linux/slab.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/dma.h>

#define RAMDISK_SIZE (1024*1024)

static DEFINE_SPINLOCK(ramdisk_lock);

static const struct block_device_operations ramdisk_fops = {
	.owner	= THIS_MODULE,
};

static struct gendisk *ramblock_disk;
static struct request_queue *ramblock_queue;
static unsigned char *ram_buff;
static int major;

static void ramblock_do_request(struct request_queue * q)
{
	struct request *req;
	printk("do:ramblock_do_request\n");
	req = blk_fetch_request(q);
	while (req) {
		/*源或目的*/
		unsigned long offset = blk_rq_pos(req) * 512;

		/*目的或源*/
		//req->buffer
		
		/*长度*/
		unsigned long len  = blk_rq_cur_bytes(req);
		if (rq_data_dir(req) == READ)
			memcpy(req->buffer, ram_buff+offset, len);
		else
			memcpy(ram_buff+offset, req->buffer, len);

		/* wrap up, 0 = success, -errno = fail */
		if (!__blk_end_request_cur(req, 0))
			req = blk_fetch_request(q);
	}

}

static int ramblock_init(void)
{
	/*获得主设备号*/
	major = register_blkdev(major, "ramdisk");
	/*1.分配一个gendisk结构体*/
	ramblock_disk = alloc_disk(16);
	
	/*2.设置*/
	/*2.1设置一个请求队列*/
	ramblock_queue = blk_init_queue(ramblock_do_request, &ramdisk_lock);
	ramblock_disk->queue = ramblock_queue;
	/*2.2设置其他信息*/
	ramblock_disk->major = major;
	ramblock_disk->first_minor = 0;
	sprintf(ramblock_disk->disk_name, "ramdisk");
	ramblock_disk->fops = &ramdisk_fops;
	set_capacity(ramblock_disk, RAMDISK_SIZE);
	/*3.硬件相关*/
	ram_buff = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
	/*4.注册*/
	add_disk(ramblock_disk);
	return 0;
}

static void ramblock_exit(void)
{
	unregister_blkdev(major, "ramdisk");
	del_gendisk(ramblock_disk);
	put_disk(ramblock_disk);
	blk_cleanup_queue(ramblock_queue);
	kfree(ram_buff);
}

module_init(ramblock_init);
module_exit(ramblock_exit);

MODULE_LICENSE("GPL");

