#include "led.h"
#include "rtthread.h"

static rt_tick_t start_time;

/** 
 * This function will init led on the board
 */
void board_led_init(void)
{
	// Configure LEDs.
	Pin powerLed = PIN_LED_POWER;
	Pin usartLed = PIN_LED_USART;
	PIO_Configure(&powerLed, 1);
	PIO_Configure(&usartLed, 1);

}

// Flash the usart led for a while.
void usart_led_flash(void)
{
	Pin p = PIN_LED_USART;
	PIO_Set(&p);
	start_time = rt_tick_get();
}

// flash a led every 10 ticks.
void led_idle_hook(void)
{
	if( rt_tick_get() - start_time > 10 )
	{
		Pin p = PIN_LED_USART;
		PIO_Clear(&p);
		return;
	}
}
