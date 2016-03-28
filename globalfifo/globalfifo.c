#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALFIFO_SIZE 0x1000
#define FIFO_CLEAR 0x1
#define GLOBALFIFO_MAJOR 241
#define DEVICE_NUM 10

static int globalfifo_major = GLOBALFIFO_MAJOR;
module_param(globalfifo_major, int, S_IRUGO);

struct globalfifo_dev {
	struct cdev cdev;
	unsigned char fifo[GLOBALFIFO_SIZE];
     struct mutex mutex;
};
struct globalfifo_dev *globalfifo_devp;

static int globalfifo_open(struct inode *inode, struct file *filp)
{
	struct globalfifo_dev *dev = container_of(inode->i_cdev, struct globalfifo_dev, cdev);
	filp->private_data = dev;
	return 0;
}

static int globalfifo_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long globalfifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct globalfifo_dev *dev = filp->private_data;
	switch (cmd) {
	case FIFO_CLEAR:
      mutex_lock(&dev->mutex);
      memset(dev->fifo, 0, GLOBALFIFO_SIZE);
      mutex_unlock(&dev->mutex);

	printk(KERN_INFO "globalfifo is set to zero\n");
	break;
	default:
		return -EINVAL;
	}
	return 0;
}

static ssize_t globalfifo_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned count = size;
	int ret = 0;

	struct globalfifo_dev *dev = filp->private_data;

	if (p > GLOBALFIFO_SIZE)
		return 0;
	if (count > GLOBALFIFO_SIZE - p)
		count = GLOBALFIFO_SIZE -p;

    mutex_lock(&dev->mutex);

	if (copy_to_user(buf, dev->fifo + p, count)) {
		return -EINVAL;
	} else {
		*ppos += count;
		ret = count;
		printk(KERN_INFO "read %u byte(s) from %lu\n", count, p);
	}
    
    mutex_unlock(&dev->mutex);

	return ret;
}

static ssize_t globalfifo_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalfifo_dev *dev = filp->private_data;

	if (p > GLOBALFIFO_SIZE)
		return 0;
	if (count > GLOBALFIFO_SIZE -p) 
		count = GLOBALFIFO_SIZE -p;

    mutex_lock(&dev->mutex);

    if (copy_from_user(dev->fifo + p, buf, count)) {
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;
		printk(KERN_INFO "written %u byte(s) from %lu\n", count, p);
	}

    mutex_unlock(&dev->mutex);

	return ret;
}

static loff_t globalfifo_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret = 0;
	switch (orig) {
	case 0:
		if (offset < 0 || (unsigned int)offset > GLOBALFIFO_SIZE) {
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1:
		if ((filp->f_pos + offset) > GLOBALFIFO_SIZE || (filp->f_pos +offset) < 0) {
			ret = -EINVAL;
			break;
		}
		filp->f_pos += offset;
		ret = filp->f_pos;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}


static const struct file_operations globalfifo_fops = {
	.owner = THIS_MODULE,
	.open = globalfifo_open,
	.release = globalfifo_release,
	.unlocked_ioctl = globalfifo_ioctl,
	.read = globalfifo_read,
	.write = globalfifo_write,
	.llseek = globalfifo_llseek,
};

static void globalfifo_setup_cdev (struct globalfifo_dev *dev, int index)
{
	int err, devno = MKDEV(globalfifo_major, index);
	
	cdev_init(&dev->cdev, &globalfifo_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error: %d adding globalfifo %d", err, index);
}

static int __init globalfifo_init (void)
{
	int ret, i;
	dev_t devno = MKDEV(globalfifo_major, 0);

	if (globalfifo_major)
		ret = register_chrdev_region(devno, DEVICE_NUM, "globalfifo");
	else {
		ret = alloc_chrdev_region(&devno, 0, DEVICE_NUM, "globalfifo");
		globalfifo_major = MAJOR(devno);
	}
	if (ret < 0)
		return ret;
	
	globalfifo_devp = kzalloc(sizeof(struct globalfifo_dev)*DEVICE_NUM, GFP_KERNEL);
	if (!globalfifo_devp) {
		unregister_chrdev_region(devno, DEVICE_NUM);
		return -ENOMEM;
	}

    mutex_init(&globalfifo_devp->mutex);
	for (i = 0; i < DEVICE_NUM; i++)
		globalfifo_setup_cdev(globalfifo_devp + i, i);
	return 0;
}

static void __exit globalfifo_exit (void)
{
	int i;
	for (i = 0; i < DEVICE_NUM; i++)
		cdev_del(&(globalfifo_devp + i)->cdev);
	kfree(globalfifo_devp);
	unregister_chrdev_region(MKDEV(globalfifo_major, 0), DEVICE_NUM);
}

module_init(globalfifo_init);
module_exit(globalfifo_exit);

MODULE_AUTHOR("Tab Liu @ <dearhange@126.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("My first globalfifo driver program.");

