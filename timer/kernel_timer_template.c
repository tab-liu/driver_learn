/* xxx device structer */
struct xxx_dev {
    struct cdev cdev;
    ...
    timer_list xxx_timer;  /* timer for device*/
};

/* one function of xxx device*/
xxx_fun1(...)
{
    struct xxx_dev *dev = filp->private_data;
    ...
    /* init the timer */
    init_timer(&dev->xxx_timer);
    dev->xxx_timer.function = &xxx_do_timer;
    dev->xxx_timer.data = (unsigned long)dev;
            /*设备结构体指针作为定时器处理函数的参数*/
    dev->xxx_timer.expires = jiffies + delay;
    /* add or register timer*/
    add_timer(&dev->xxx_timer);
    ...
}

/* one function of xxx device */
xxx_fun2()
{
    ...
    /*delte the timer */
    del_timer(&dev->xxx_timer);
    ...
}

/*timer handler function*/
static void xxx_do_timer(unsigned long arg)
{
    struct xxx_device *dev = (struct xxx_device *)arg;
    ...
    //调度定时器时再执行
    dev->xxx_timer.expires = jiffies + delay;
    add_timer(&dev->xxx_timer);
    ...
}
