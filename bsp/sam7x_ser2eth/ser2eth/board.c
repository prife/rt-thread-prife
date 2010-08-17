/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-08-23     Bernard      first implementation
 */

#include <rtthread.h>
#include <rthw.h>

#include <AT91SAM7X256.h>
#include "board.h"
#include "led.h"
#include "dev_dbgu.h"

// PIN 50, PHYADDR0, if connected, low.
static const Pin PINS_USART0 = {1<<22, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_INPUT, PIO_PULLUP};

//#define TCK  1000                           /* Timer Clock  */
#define PIV  (((BOARD_MCK/16)/1000)*(1000/RT_TICK_PER_SECOND))	/* Periodic Interval Value */

/**
 * This is the timer interrupt service routine.
 * @param vector the irq number for timer
 */
void rt_hw_timer_handler(int vector)
{
	if (AT91C_BASE_PITC->PITC_PISR & 0x01)
	{
		/* increase a tick */
		rt_tick_increase();

		/* ack interrupt */
		AT91C_BASE_AIC->AIC_EOICR = AT91C_BASE_PITC->PITC_PIVR;
	}
#ifdef RT_USING_DBGU
	else if( AT91C_BASE_DBGU->DBGU_CSR & 0x01 )
	{
		rt_hw_dbgu_isr(vector);
		//AT91C_BASE_AIC->AIC_EOICR = AT91C_BASE_DBGU->DBGU_RHR;
	}
#endif
}


void rt_hw_console_output(const char* str)
{
#ifdef RT_USING_DBGU
	while (*str)
	{
		if (*str == '\n')
		{
			DBGU_PutChar('\r');
		}
		/* Wait for Empty Tx Buffer and Transmit Character */
		DBGU_PutChar(*str);
		str ++;
	}
#endif
}

static void rt_hw_console_init()
{
#ifdef RT_USING_DBGU
	rt_hw_dbgu_console_tx_init();
#endif
}

/**
 * This function will initial sam7x board.
 */
void rt_hw_board_init()
{
	/* init hardware console */
	rt_hw_console_init();

	/* init led */
	board_led_init();
	
	/* init flash */
	FLASHD_Initialize(BOARD_MCK);

	/* init PITC */
	AT91C_BASE_PITC->PITC_PIMR = (1 << 25) | (1 << 24) | PIV;

	/* install timer handler */
	rt_hw_interrupt_install(AT91C_ID_SYS, rt_hw_timer_handler, RT_NULL);
	AT91C_BASE_AIC->AIC_SMR[AT91C_ID_SYS] = 0;
	rt_hw_interrupt_umask(AT91C_ID_SYS);	

	// init PHYADR0 --> connection status.
	PIO_Configure(&PINS_USART0,1);
}

void idle_hook_function(void)
{
	// reset watchdog timer.
	// watchdog timer triggers every 8 seconds, see start_rvds.s
	// but if task is busy, we feed dog in some tasks also.
	FEED_THE_DOG();
	
	// flash led.
	led_idle_hook();

}

int getLinkStatus(void)
{
	return !PIO_Get(&PINS_USART0);
}

