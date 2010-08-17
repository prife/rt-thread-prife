#ifndef LED_H
#define LED_H

#include "at91lib.h"

// LED_USART: PB20	LED_POWER: PB21
#define PIN_LED_USART {(1<<20), AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_OUTPUT_0, PIO_PULLUP}
#define PIN_LED_POWER {(1<<21), AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_OUTPUT_1, PIO_PULLUP}

void board_led_init(void);
void usart_led_flash(void);
void led_idle_hook(void);


#endif //LED_H

