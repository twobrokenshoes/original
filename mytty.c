#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/param.h>
#include <linux/workqueue.h>

#define TAGS	"mytty"
#define	MY_TTY_MINOR	4
#define	MY_TTY_MAJOR	240

struct tty_driver *pmytty = NULL;

struct mytty_info {
	struct delayed_work work;
	int open_count;
	struct tty_struct *tty;
};

static void print_work(struct work_struct *data)
{
	unsigned char *text = "hello,vivo!\n";
	struct mytty_info *pinfo = (struct mytty_info *)data;
//	printk("%s: %s open_count = %d\n",TAGS, text, pinfo->open_count);
	tty_insert_flip_string(pinfo->tty, text, strlen(text));
	tty_flip_buffer_push(pinfo->tty);
//	tty_kref_put(pinfo->tty);
	
	schedule_delayed_work(&pinfo->work, 5 * HZ );
}

static int mytty_open(struct tty_struct *tty,struct file *pfile)
{
	struct mytty_info *pinfo = NULL;
	struct tty_driver *pmytty;
	int line;
	if(!tty)
	{
		printk("%s: failed to alloc for mytty info\n",TAGS);
		return -EINVAL;
	}
	if(!tty->driver_data)
	{
		pinfo = kzalloc(sizeof(struct mytty_info), GFP_KERNEL);
		if(!pinfo)
		{
			printk("%s: tty is NULL\n",TAGS);
			return -ENOMEM;
		}
		pmytty = tty->driver;
		line = tty->index;
		tty_port_tty_set(pmytty->ports[line],tty);
		
		INIT_DELAYED_WORK(&pinfo->work, print_work);
		schedule_delayed_work(&pinfo->work, 5 * HZ);
		tty->driver_data = pinfo;
		
		pinfo->tty = tty;
	}
	pinfo = tty->driver_data;
	pinfo->open_count += 1;
	printk("%s: mytty open successfull!open count is %d\n", TAGS, pinfo->open_count);
	
	return 0;
}

static void mytty_close(struct tty_struct *tty,struct file *pfile)
{
	struct mytty_info *pinfo = tty->driver_data;
	if(!pinfo)
	{
		printk("%s: tty->driver_data is NULL\n",TAGS);
		return;
	}
	if(pinfo->open_count <= 0)
	{
		printk("%s: tty->open_count is error\n",TAGS);
		return;
	}
	pinfo->open_count -= 1;
	if(!pinfo->open_count)
	{
		cancel_delayed_work(&pinfo->work);
		flush_scheduled_work();
		kfree(pinfo);
	}
	printk("%s: mytty close successfull!open count is %d\n", TAGS, pinfo->open_count);
	return;
}

static int mytty_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
	int i;
	struct mytty_info *pinfo = tty->driver_data;
	if(!pinfo)
	{
		printk("%s: tty->driver_data is NULL\n",TAGS);
		return -ENODEV;
	}
	if(pinfo->open_count <= 0)
	{
		printk("%s: tty->open_count is error\n",TAGS);
		return -EINVAL;
	}
//	printk("%s: ",TAGS);
	for(i = 0;i < count; i++ )
	{
/*		if(buf[i] == '\n')
		{
			printk("\r");
		}*/
		printk("%c",buf[i]);
	}
//	printk("\n");
	return count;
}

static int mytty_write_room(struct tty_struct *tty)
{
	struct mytty_info *pinfo = tty->driver_data;
	if(!pinfo)
	{
		printk("%s: tty->driver_data is NULL\n",TAGS);
		return -ENODEV;
	}
	if(pinfo->open_count <= 0)
	{
		printk("%s: tty->open_count is error\n",TAGS);
		return -EINVAL;
	}
	return 255;
}
static struct tty_operations mytty_ops = {
	.open = mytty_open,
	.close = mytty_close,
	.write = mytty_write,
	.write_room = mytty_write_room,
};

static int mytty_probe(struct platform_device *dev)
{
	int ret = 0;

	int i = 0;

	pmytty = alloc_tty_driver(MY_TTY_MINOR);
	if(!pmytty)
	{
		printk("%s: failed to alloc for mytty\n",TAGS);
		return -ENOMEM;
	}

	pmytty->owner = THIS_MODULE;
	pmytty->driver_name = "mytty";
	pmytty->name = "ttyL";
	pmytty->major = MY_TTY_MAJOR;
	pmytty->num = 4;
	pmytty->type = TTY_DRIVER_TYPE_SERIAL;
	pmytty->subtype = SERIAL_TYPE_NORMAL;
	pmytty->flags = TTY_DRIVER_REAL_RAW;//|TTY_DRIVER_NO_DEVFS;
	pmytty->init_termios = tty_std_termios;
	pmytty->init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	tty_set_operations(pmytty,&mytty_ops);
	
	pmytty->ports = kzalloc(pmytty->num * (sizeof(struct tty_port*)), GFP_KERNEL);
	if(!pmytty->ports)
	{
		printk("%s: failed to malloc for pmytty->ports\n",TAGS);
		goto malloc_port_err;
	}
	pmytty->ports[0] = kzalloc(pmytty->num * (sizeof(struct tty_port)), GFP_KERNEL);
	if(!pmytty->ports[0])
	{
		printk("%s: failed to malloc for pmytty->ports[0]\n",TAGS);
		goto malloc_port0_err;
	}
	for(i = 0;i < pmytty->num; i++ )
	{
		pmytty->ports[i] = pmytty->ports[0] + i;
		tty_port_init(pmytty->ports[i]);
	}
	
	ret = tty_register_driver(pmytty);
	if(ret)
	{
		printk("%s: failed to register mytty driver\n",TAGS);
		goto tty_reg_err;
	}
	printk("%s: %s run successfully\n", TAGS, __func__ );
	return 0;
tty_reg_err:
	for(i = 0;i < pmytty->num; i++ )
	{
		kfree(pmytty->ports[i]);
	}
malloc_port0_err:	
	kfree(pmytty->ports);
malloc_port_err:
	put_tty_driver(pmytty);
	return ret;
}

static int mytty_remove(struct platform_device *dev)
{
	int i;
	if(!pmytty)
	{
		printk("%s: pmytty is NULL before unregister tty_driver\n",TAGS);
		return -1;
	}
	tty_unregister_driver(pmytty);
	if(!pmytty)
	{
		printk("%s: pmytty is NULL after unregister tty_driver\n",TAGS);
		return -1;
	}
	
	for(i = 0;i < pmytty->num; i++ )
	{
		kfree(pmytty->ports[i]);
	}
	kfree(pmytty->ports);
	
	put_tty_driver(pmytty);
	pmytty = NULL;
	printk("%s: %s run successfully\n", TAGS, __func__ );
	return 0;
}

static void mytty_device_release(struct device *pdev)
{
	return;
}

static struct platform_device mytty_device = {
	.name = "tty_test",
	.id = -1,
	.dev = {
		.release = mytty_device_release,
	},
};

static struct platform_driver mytty_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "tty_test",
	},
	.probe = mytty_probe,
	.remove = mytty_remove,
};

int __init mytty_init(void)
{
	int ret = 0;
	ret = platform_device_register(&mytty_device);
	if(ret)
	{
		printk("%s: failed to register mytty_device \n",TAGS);
		return ret;
	}
	ret = platform_driver_register(&mytty_driver);
	if(ret)
	{
		printk("%s: failed to register mytty_driver \n",TAGS);
		goto register_driver_err;
	}
	return 0;
register_driver_err:
	platform_device_unregister(&mytty_device);
	return ret;
}

void __exit mytty_exit(void)
{
	platform_driver_unregister(&mytty_driver);
	platform_device_unregister(&mytty_device);
}

module_init(mytty_init);
module_exit(mytty_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liukangfei");
