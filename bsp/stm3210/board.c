/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-08-23     Bernard      first implementation
 */

#include <rthw.h>
#include <rtthread.h>

#include "stm32f10x.h"
#include "board.h"

static void rt_hw_console_init(void);

/**
 * @addtogroup STM32
 */

/*@{*/

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
	/* Set the Vector Table base location at 0x20000000 */
	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
	/* Set the Vector Table base location at 0x08000000 */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif
}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Configures the SysTick for OS tick.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void  SysTick_Configuration(void)
{
	RCC_ClocksTypeDef  rcc_clocks;
	rt_uint32_t         cnts;

	RCC_GetClocksFreq(&rcc_clocks);

	cnts = (rt_uint32_t)rcc_clocks.HCLK_Frequency / RT_TICK_PER_SECOND;

	SysTick_Config(cnts);
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
}

/**
 * This is the timer interrupt service routine.
 *
 */
void rt_hw_timer_handler(void)
{
	/* enter interrupt */
	rt_interrupt_enter();

	rt_tick_increase();

	/* leave interrupt */
	rt_interrupt_leave();
}

/**
 * This function will initial STM32 board.
 */
void rt_hw_board_init()
{
	/* NVIC Configuration */
	NVIC_Configuration();

	/* Configure the SysTick */
	SysTick_Configuration();

	rt_hw_console_init();
}

#if STM32_CONSOLE_USART == 1
#define CONSOLE_RX_PIN	    GPIO_Pin_9
#define CONSOLE_TX_PIN	    GPIO_Pin_10
#define CONSOLE_GPIO	    GPIOA
#define CONSOLE_USART	    USART1
#define CONSOLE_RCC         RCC_APB2Periph_USART1
#define CONSOLE_RCC_GPIO    RCC_APB2Periph_GPIOA
#elif STM32_CONSOLE_USART == 2

#if defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_CL)
#define CONSOLE_RX_PIN	    GPIO_Pin_6
#define CONSOLE_TX_PIN	    GPIO_Pin_5
#define CONSOLE_GPIO	    GPIOD
#define CONSOLE_RCC         RCC_APB1Periph_USART2
#define CONSOLE_RCC_GPIO    RCC_APB2Periph_GPIOD
#elif defined(STM32F10X_HD)
#define CONSOLE_RX_PIN	    GPIO_Pin_3
#define CONSOLE_TX_PIN	    GPIO_Pin_2
#define CONSOLE_GPIO	    GPIOA
#define CONSOLE_RCC         RCC_APB1Periph_USART2
#define CONSOLE_RCC_GPIO    RCC_APB2Periph_GPIOA
#endif

#define CONSOLE_USART	USART2
#elif STM32_CONSOLE_USART == 2
#define CONSOLE_RX_PIN	    GPIO_Pin_11
#define CONSOLE_TX_PIN	    GPIO_Pin_10
#define CONSOLE_GPIO	    GPIOB
#define CONSOLE_USART	    USART3
#define CONSOLE_RCC         RCC_APB1Periph_USART3
#define CONSOLE_RCC_GPIO    RCC_APB2Periph_GPIOB
#endif

/* init console to support rt_kprintf */
static void rt_hw_console_init()
{
#if STM32_CONSOLE_USART == 0
#else
    /* Enable GPIOx clock */
    RCC_APB2PeriphClockCmd(CONSOLE_RCC_GPIO, ENABLE);

#if STM32_CONSOLE_USART == 1
	/* Enable USART1 and GPIOA clocks */
	RCC_APB2PeriphClockCmd(CONSOLE_RCC, ENABLE);
#else
    RCC_APB1PeriphClockCmd(CONSOLE_RCC, ENABLE);
#endif

#if (STM32_CONSOLE_USART == 2)  && (defined(STM32F10X_LD) || defined(STM32F10X_MD) || defined(STM32F10X_CL))
    /* Enable AFIO clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* Enable the USART2 Pins Software Remapping */
    GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
#endif

	/* GPIO configuration */
	{
		GPIO_InitTypeDef GPIO_InitStructure;

		/* Configure USART1 Tx (PA.09) as alternate function push-pull */
		GPIO_InitStructure.GPIO_Pin = CONSOLE_RX_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(CONSOLE_GPIO, &GPIO_InitStructure);

		/* Configure USART1 Rx (PA.10) as input floating */
		GPIO_InitStructure.GPIO_Pin = CONSOLE_TX_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(CONSOLE_GPIO, &GPIO_InitStructure);
	}

	/* USART configuration */
	{
		USART_InitTypeDef USART_InitStructure;

		/* USART configured as follow:
		- BaudRate = 115200 baud
		- Word Length = 8 Bits
		- One Stop Bit
		- No parity
		- Hardware flow control disabled (RTS and CTS signals)
		- Receive and transmit enabled
		- USART Clock disabled
		- USART CPOL: Clock is active low
		- USART CPHA: Data is captured on the middle
		- USART LastBit: The clock pulse of the last data bit is not output to
		the SCLK pin
		*/
		USART_InitStructure.USART_BaudRate = 115200;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_Init(CONSOLE_USART, &USART_InitStructure);
		/* Enable USART1 */
		USART_Cmd(CONSOLE_USART, ENABLE);
	}
#endif
}

/* write one character to serial, must not trigger interrupt */
static void rt_hw_console_putc(const char c)
{
	/*
		to be polite with serial console add a line feed
		to the carriage return character
	*/
	if (c=='\n')rt_hw_console_putc('\r');

	while (!(CONSOLE_USART->SR & USART_FLAG_TXE));
	CONSOLE_USART->DR = (c & 0x1FF);
}

/**
 * This function is used by rt_kprintf to display a string on console.
 *
 * @param str the displayed string
 */
void rt_hw_console_output(const char* str)
{
#if STM32_CONSOLE_USART == 0
	/* no console */
#else
	while (*str)
	{
		rt_hw_console_putc (*str++);
	}
#endif
}

/*@}*/
