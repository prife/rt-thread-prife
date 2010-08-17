#ifndef __RT_SERIAL_H__
#define __RT_SERIAL_H__

#include "rtconfig.h"

#ifndef RT_UART_RX_BUFFER_SIZE
	#define RT_UART_RX_BUFFER_SIZE	128
#endif //RT_UART_RX_BUFFER_SIZE

extern rt_err_t rt_hw_serial_init(void);

#endif
