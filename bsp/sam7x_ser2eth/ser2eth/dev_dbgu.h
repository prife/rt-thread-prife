#ifndef DEV_DBGU_H
#define DEV_DBGU_H


#include "rtconfig.h"

#ifndef RT_DBGU_RX_BUFFER_SIZE
	#define RT_DBGU_RX_BUFFER_SIZE	32
#endif //RT_UART_RX_BUFFER_SIZE

extern rt_err_t rt_hw_dbgu_init(void);
extern void rt_hw_dbgu_isr(int irqno);
extern void rt_hw_dbgu_console_tx_init(void);

#endif //DEV_DBGU_H

