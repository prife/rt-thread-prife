/*
 * File      : serial.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-08-23     Bernard      first version
 * 2009-05-14     Bernard      add RT-THread device interface
 */

#include <rthw.h>
#include <rtthread.h>
#include "board.h"
#include "AT91SAM7X256.h"
#include "at91lib.h"
#include "dev_dbgu.h"

#ifdef RT_USING_DBGU

// DBGU pins definition.
static const Pin PINS_DBGU = {0x18000000, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_PULLUP};


struct rt_at91dbgu
{
	struct rt_device parent;

	//AT91PS_DBGU hw_base;
	//rt_uint16_t peripheral_id;
	rt_uint32_t baudrate;

	/* reception field */
	rt_uint16_t save_index, read_index;
	rt_uint8_t  rx_buffer[RT_DBGU_RX_BUFFER_SIZE];
};

struct rt_at91dbgu dbgu;


void rt_hw_dbgu_isr(int irqno)
{
	rt_base_t level;
	struct rt_device* device;
	struct rt_at91dbgu* serialDbgu = &dbgu;

	device = (rt_device_t)serialDbgu;

	/* This function is not the REAL isr, it will be called in SYS isr */
	if( !(AT91C_BASE_DBGU->DBGU_CSR & 0x01) )
		return;

	/* disable interrupt */
	level = rt_hw_interrupt_disable();

	/* get received character */
	serialDbgu->rx_buffer[serialDbgu->save_index] = DBGU_GetChar();

	/* move to next position */
	serialDbgu->save_index ++;
	if (serialDbgu->save_index >= RT_DBGU_RX_BUFFER_SIZE)
		serialDbgu->save_index = 0;

	/* if the next position is read index, discard this 'read char' */
	if (serialDbgu->save_index == serialDbgu->read_index)
	{
		serialDbgu->read_index ++;
		if (serialDbgu->read_index >= RT_DBGU_RX_BUFFER_SIZE)
			serialDbgu->read_index = 0;
	}

	/* enable interrupt */
	rt_hw_interrupt_enable(level);

	/* indicate to upper layer application */
	if (device->rx_indicate != RT_NULL)
		device->rx_indicate(device, 1);

	/* ack interrupt */
	AT91C_BASE_AIC->AIC_EOICR = 1;
}

static rt_err_t rt_dbgu_init (rt_device_t dev)
{
	struct rt_at91dbgu* serialDbgu = (struct rt_at91dbgu*)dev;
	
	/* First, Enable Clock for USART */
	PMC_EnablePeripheral(AT91C_ID_SYS);
	
	/* Second, Configure dbgu */
	DBGU_Configure(DBGU_STANDARD, serialDbgu->baudrate, BOARD_MCK);

	/* Open Pio ports. */
	PIO_Configure(&PINS_DBGU, 1);

	/* reset rx index */
	serialDbgu->save_index = 0;
	serialDbgu->read_index = 0;

	/* reset rx buffer */
	rt_memset(serialDbgu->rx_buffer, 0, RT_DBGU_RX_BUFFER_SIZE);

	return RT_EOK;
}


void rt_hw_dbgu_console_tx_init(void)
{
	/* First, Enable Clock for USART */
	PMC_EnablePeripheral(AT91C_ID_SYS);

	/* Second, Configure dbgu */
	DBGU_Configure(DBGU_STANDARD, 115200, BOARD_MCK);

	/* Open Pio ports. */
	PIO_Configure(&PINS_DBGU, 1);
}

static rt_err_t rt_dbgu_open(rt_device_t dev, rt_uint16_t oflag)
{
	RT_ASSERT(dev != RT_NULL);

	if (dev->flag & RT_DEVICE_FLAG_INT_RX)
	{
		/* enable UART rx interrupt */
		AT91C_BASE_DBGU->DBGU_IER = 1 << 0; 		/* RxReady interrupt */
		AT91C_BASE_DBGU->DBGU_IMR |= 1 << 0; 		/* umask RxReady interrupt */

		/* install UART handler */
		/* NOTE: DBGU shares irq with the other system peripherals. */
	}

	return RT_EOK;
}

static rt_err_t rt_dbgu_close(rt_device_t dev)
{
	RT_ASSERT(dev != RT_NULL);

	if (dev->flag & RT_DEVICE_FLAG_INT_RX)
	{
		/* disable interrupt */
		AT91C_BASE_DBGU->DBGU_IDR = 1 << 0; 		/* RxReady interrupt */
		AT91C_BASE_DBGU->DBGU_IMR &= ~(1 << 0); 	/* mask RxReady interrupt */
	}

	return RT_EOK;
}

static rt_size_t rt_dbgu_read (rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	rt_uint8_t* ptr;
	struct rt_at91dbgu *serialDbgu = (struct rt_at91dbgu*)dev;
	RT_ASSERT(serialDbgu != RT_NULL);

	/* point to buffer */
	ptr = (rt_uint8_t*) buffer;

	if (dev->flag & RT_DEVICE_FLAG_INT_RX)
	{
		while (size)
		{
			/* interrupt receive */
			rt_base_t level;

			/* disable interrupt */
			level = rt_hw_interrupt_disable();
			if (serialDbgu->read_index != serialDbgu->save_index)
			{
				*ptr = serialDbgu->rx_buffer[serialDbgu->read_index];

				serialDbgu->read_index ++;
				if (serialDbgu->read_index >= RT_DBGU_RX_BUFFER_SIZE)
					serialDbgu->read_index = 0;
			}
			else
			{
				/* no data in rx buffer */

				/* enable interrupt */
				rt_hw_interrupt_enable(level);
				break;
			}

			/* enable interrupt */
			rt_hw_interrupt_enable(level);

			ptr ++;
			size --;
		}

		return (rt_uint32_t)ptr - (rt_uint32_t)buffer;
	}
	else if (dev->flag & RT_DEVICE_FLAG_DMA_RX)
	{
		/* not support right now */
		RT_ASSERT(0);
	}
	else
	{
		/* poll mode */
		while (size)
		{
			*ptr = DBGU_GetChar();
			ptr ++;
			size --;
		}

		return (rt_size_t)ptr - (rt_size_t)buffer;
	}

	return 0;
}

static rt_size_t rt_dbgu_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
	rt_uint8_t* ptr;

	ptr = (rt_uint8_t*) buffer;
	if (dev->open_flag & RT_DEVICE_OFLAG_WRONLY)
	{
		if (dev->flag & RT_DEVICE_FLAG_STREAM)
		{
			/* it's a stream mode device */
			while (size)
			{
				/* stream mode */
				if (*ptr == '\n')
				{
					DBGU_PutChar('\r');
				}

				/* Wait for Empty Tx Buffer and Transmit Character */
				DBGU_PutChar(*ptr);

				ptr ++;
				size --;
			}
		}
		else
		{
			while (size)
			{
				/* Wait for Empty Tx Buffer and Transmit Character */
				DBGU_PutChar(*ptr);
				ptr ++;
				size --;
			}
		}
	}

	return (rt_size_t)ptr - (rt_size_t)buffer;
}

static rt_err_t rt_dbgu_control (rt_device_t dev, rt_uint8_t cmd, void *args)
{
	return RT_EOK;
}

rt_err_t rt_hw_dbgu_init(void)
{
	rt_device_t device;

	device = (rt_device_t) &dbgu;

	/* init serial device private data */
	dbgu.baudrate		= 115200;

	/* set device virtual interface */
	device->init 	= rt_dbgu_init;
	device->open 	= rt_dbgu_open;
	device->close 	= rt_dbgu_close;
	device->read 	= rt_dbgu_read;
	device->write 	= rt_dbgu_write;
	device->control = rt_dbgu_control;

	/* register uart1 on device subsystem */
	rt_device_register(device, "dbgu", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);

	return RT_EOK;
}
#endif //RT_USING_DBGU

/*@}*/
