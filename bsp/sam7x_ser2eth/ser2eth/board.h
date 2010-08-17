/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-10-08     Bernard      add board.h to this bsp
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <rthw.h>
#include <rtthread.h>

#define BOARD_MCK	29491200


#define BOARD_FLASH_EFC
#define at91sam7x256
#define NOASSERT
#define TRACE_ENABLED
#define AT91C_PHY_ADDR      0x01


void rt_hw_board_init(void);
void idle_hook_function(void);
// if rj45 is connected, return 1. else return 0;
int getLinkStatus(void);
#define FEED_THE_DOG()	do{AT91C_BASE_WDTC->WDTC_WDCR = 0xA5UL<<24 | 1;}while(0)

#endif
