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
#include "dev_serial.h"
#include "firmupdate.h"

/// USART0 pins definition.
static const Pin PINS_USART0 = {0x03, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_PULLUP};
static const Pin PINS_USART0_CLOSE = {0x3, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_INPUT, PIO_PULLUP};
/// USART1 pins definition.
static const Pin PINS_USART1 = {0x60, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_PULLUP};
static const Pin PINS_USART1_CLOSE = {0x60, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_INPUT, PIO_PULLUP};


/**
 * @addtogroup AT91SAM7X256
 */
/*@{*/

struct rt_at91serial
{
	struct rt_device parent;

	AT91PS_USART hw_base;
	rt_uint16_t peripheral_id;
	rt_uint32_t baudrate;

	/* reception field */
	rt_uint16_t recv_tail, recv_head;
	rt_uint8_t  rx_buffer[RT_UART_RX_BUFFER_SIZE];
	// tx
	//rt_uint8_t  send_tail, send_head;
	//rt_uint8_t  tx_buffer[RT_UART_TX_BUFFER_SIZE];
};
rt_uint16_t inc_index(int bufsize,rt_uint16_t index)
{
	index++;
	if( index >= bufsize )
		index = 0;
	return index;
}

#ifdef RT_USING_UART1
struct rt_at91serial serial1;
#endif
#ifdef RT_USING_UART2
struct rt_at91serial serial2;
#endif

static void rt_hw_serial_isr(int irqno)
{
	unsigned int status;
	rt_base_t level;
	struct rt_device* device;
	struct rt_at91serial* serial = RT_NULL;

#ifdef RT_USING_UART1
	if (irqno == AT91C_ID_US0)
	{
		/* serial 1 */
		serial = &serial1;
	}
#endif

#ifdef RT_USING_UART2
	if (irqno == AT91C_ID_US1)
	{
		/* serial 2 */
		serial = &serial2;
	}
#endif
	RT_ASSERT(serial != RT_NULL);

	/* get generic device object */
	device = (rt_device_t)serial;

	/* disable interrupt */
	level = rt_hw_interrupt_disable();
	
	status = serial->hw_base->US_CSR;

	if( status & AT91C_US_RXRDY )
	{
		//if( serial->hw_base->US_CSR & AT91C_US_OVRE )
		//	rt_kprintf("AT91C_US_OVRE\n");
		if( inc_index(RT_UART_RX_BUFFER_SIZE,serial->recv_head) == serial->recv_tail )
		{
			// drop this char.
			static char i=0;
			volatile rt_uint8_t dummy = serial->hw_base->US_RHR;
			if( i==0 )
			{
				rt_kprintf("ERROR! Serial input buffer overflow!\n");
				i=1;
			}
		}
		else
		{
			serial->recv_head = inc_index(RT_UART_RX_BUFFER_SIZE,serial->recv_head);
			serial->rx_buffer[serial->recv_head] = serial->hw_base->US_RHR;
			checkFirmUpdateMagic(serial->rx_buffer[serial->recv_head]);
		}

		/* indicate to upper layer application */
		if (device->rx_indicate != RT_NULL)
			device->rx_indicate(device, 1);
	}
	else if( status & AT91C_US_ENDTX )
	{
		//rt_kprintf("ENDTX\n");
		serial->hw_base->US_IDR = AT91C_US_ENDTX;
		if( device->tx_complete != RT_NULL )
			device->tx_complete(device,RT_NULL);
	}

	/* enable interrupt */
	rt_hw_interrupt_enable(level);

	/* ack interrupt */
	// see rt_hw_trap_irq()
	//AT91C_BASE_AIC->AIC_EOICR = 1;
}

static rt_err_t rt_serial_init (rt_device_t dev)
{
    unsigned int mode = AT91C_US_USMODE_NORMAL
                        | AT91C_US_CLKS_CLOCK
                        | AT91C_US_CHRL_8_BITS
                        | AT91C_US_PAR_NONE
                        | AT91C_US_NBSTOP_1_BIT
                        | AT91C_US_CHMODE_NORMAL;

	struct rt_at91serial* serial = (struct rt_at91serial*) dev;

	RT_ASSERT(serial != RT_NULL);
	/* must be US0 or US1 */
	RT_ASSERT((serial->peripheral_id == AT91C_ID_US0) ||
			  (serial->peripheral_id == AT91C_ID_US1));

	/* First, Enable Clock for USART */
	PMC_EnablePeripheral(serial->peripheral_id);
	
	/* Second, Configure usart */
	USART_Configure(serial->hw_base, mode, serial->baudrate, BOARD_MCK);

	/* Note, Pio ports should not be enabled here, rt_serial_open() is a better place. */

	/* reset rx index */
	serial->recv_tail = serial->recv_head = 0;
	rt_memset(serial->rx_buffer, 0, RT_UART_RX_BUFFER_SIZE);

	return RT_EOK;
}

static rt_err_t rt_serial_open(rt_device_t dev, rt_uint16_t oflag)
{
	struct rt_at91serial *serial = (struct rt_at91serial*)dev;
	RT_ASSERT(serial != RT_NULL);
	
	/* Enable transmitter and receiver */
	USART_SetTransmitterEnabled(serial->hw_base, 1);
	USART_SetReceiverEnabled(serial->hw_base, 1);
	
	/* Enable pins */
	if( serial->peripheral_id == AT91C_ID_US0 )
	{
		PIO_Configure(&PINS_USART0, 1);
	}
	else if ( serial->peripheral_id == AT91C_ID_US1 )
	{
		PIO_Configure(&PINS_USART1, 1);
	}
	
	if (dev->flag & RT_DEVICE_FLAG_INT_RX)
	{
		/* enable UART rx tx interrupt */
		serial->hw_base->US_IER = AT91C_US_RXRDY;		/* RxReady interrupt */

		/* install UART handler */
		rt_hw_interrupt_install(serial->peripheral_id, rt_hw_serial_isr, RT_NULL);
		AT91C_BASE_AIC->AIC_SMR[serial->peripheral_id] = 5;
		rt_hw_interrupt_umask(serial->peripheral_id);
	}

	return RT_EOK;
}

static rt_err_t rt_serial_close(rt_device_t dev)
{
	struct rt_at91serial *serial = (struct rt_at91serial*)dev;
	RT_ASSERT(serial != RT_NULL);

	if ((dev->flag & RT_DEVICE_FLAG_INT_RX))
	{
		/* disable interrupt */
		serial->hw_base->US_IDR = AT91C_US_RXRDY; 		/* RxReady interrupt */
	}

	serial->hw_base->US_CR = AT91C_US_RSTRX	    | 	/* Reset Receiver      */
							 AT91C_US_RSTTX		|	/* Reset Transmitter   */
							 AT91C_US_RXDIS		|	/* Receiver Disable    */
							 AT91C_US_TXDIS;		/* Transmitter Disable */

	/* Close Pins */
	if( serial->peripheral_id == AT91C_ID_US0 )
	{
		PIO_Configure(&PINS_USART0_CLOSE, 1);
	}
	else if ( serial->peripheral_id == AT91C_ID_US1 )
	{
		PIO_Configure(&PINS_USART1_CLOSE, 1);
	}

	return RT_EOK;
}

static rt_size_t rt_serial_read (rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	rt_uint8_t* ptr;
	struct rt_at91serial *serial = (struct rt_at91serial*)dev;
	RT_ASSERT(serial != RT_NULL);

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
			
			if( serial->recv_tail == serial->recv_head )
			{
				// recv completed.
				rt_hw_interrupt_enable(level);
				break;
			}
			else
			{
				serial->recv_tail = inc_index(RT_UART_RX_BUFFER_SIZE,serial->recv_tail);
				*ptr = serial->rx_buffer[serial->recv_tail];
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
			/* Read Character */
			*ptr = USART_Read(serial->hw_base,0);
			ptr ++;
			size --;
		}

		return (rt_size_t)ptr - (rt_size_t)buffer;
	}
	return 0;
	
}

static rt_size_t rt_serial_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
	rt_uint8_t* ptr;
	struct rt_at91serial *serial = (struct rt_at91serial*)dev;
	RT_ASSERT(serial != RT_NULL);

	ptr = ((rt_uint8_t*) buffer)+pos;

	serial->hw_base->US_PTCR = AT91C_PDC_TXTDIS;
	// we should poll PTSR to wait for TXTDIS.
	while(serial->hw_base->US_PTSR & AT91C_PDC_TXTEN);
    serial->hw_base->US_TPR = (unsigned int) ptr;
    serial->hw_base->US_TCR = size;
    serial->hw_base->US_PTCR = AT91C_PDC_TXTEN;

	serial->hw_base->US_IER = AT91C_US_ENDTX;
	return size;
}

static rt_err_t rt_serial_control (rt_device_t dev, rt_uint8_t cmd, void *args)
{
	return RT_EOK;
}

rt_err_t rt_hw_serial_init()
{
	rt_device_t device;

#ifdef RT_USING_UART1
	device = (rt_device_t) &serial1;

	/* init serial device private data */
	serial1.hw_base 		= AT91C_BASE_US0;
	serial1.peripheral_id 	= AT91C_ID_US0;
	serial1.baudrate		= 115200;

	/* set device virtual interface */
	device->init 	= rt_serial_init;
	device->open 	= rt_serial_open;
	device->close 	= rt_serial_close;
	device->read 	= rt_serial_read;
	device->write 	= rt_serial_write;
	device->control = rt_serial_control;

	/* register uart1 on device subsystem */
	rt_device_register(device, "uart1", RT_DEVICE_FLAG_RDWR
						| RT_DEVICE_FLAG_INT_RX
						| RT_DEVICE_FLAG_DMA_TX);
#endif

#ifdef RT_USING_UART2
	device = (rt_device_t) &serial2;

	serial2.hw_base 		= AT91C_BASE_US1;
	serial2.peripheral_id 	= AT91C_ID_US1;
	serial2.baudrate		= 115200;

	/* set device virtual interface */
	device->init 	= rt_serial_init;
	device->open 	= rt_serial_open;
	device->close 	= rt_serial_close;
	device->read 	= rt_serial_read;
	device->write 	= rt_serial_write;
	device->control = rt_serial_control;

	/* register uart2 on device subsystem */
	rt_device_register(device, "uart2", RT_DEVICE_FLAG_RDWR
						| RT_DEVICE_FLAG_INT_RX
						| RT_DEVICE_FLAG_DMA_TX);
#endif

	return RT_EOK;
}

/*@}*/
