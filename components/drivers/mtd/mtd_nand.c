/*
 * File      : mtd_core.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-05     Bernard      the first version
 */

/*
 * COPYRIGHT (C) 2012, Shanghai Real Thread
 */

#include <drivers/mtd.h>

#ifdef RT_USING_MTD_NAND

/*
 * RT-Thread Generic Device Interface
 */
static rt_err_t _mtd_init(rt_device_t dev)
{
	return RT_EOK;
}

static rt_err_t _mtd_open(rt_device_t dev, rt_uint16_t oflag)
{
	return RT_EOK;
}

static rt_err_t _mtd_close(rt_device_t dev)
{
	return RT_EOK;
}

static rt_size_t _mtd_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	return size;
}

static rt_size_t _mtd_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
	return size;
}

static rt_err_t _mtd_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
	return RT_EOK;
}

rt_err_t rt_mtd_nand_register_device(const char* name, struct rt_mtd_nand_device* device)
{
	rt_device_t dev;

	dev = RT_DEVICE(device);
	RT_ASSERT(dev != RT_NULL);

	/* set device class and generic device interface */
	dev->type 		= RT_Device_Class_MTD;
	dev->init 		= _mtd_init;
	dev->open 		= _mtd_open;
	dev->read 		= _mtd_read;
	dev->write 		= _mtd_write;
	dev->close      = _mtd_close;
	dev->control 	= _mtd_control;

	dev->rx_indicate = RT_NULL;
	dev->tx_complete = RT_NULL;

	/* register to RT-Thread device system */
	return rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
}
#endif
