#include "stm32f10x.h"

#include "board.h"

#include <rtthread.h>
#include <rtgui/event.h>
#include <rtgui/kbddef.h>
#include <rtgui/rtgui_server.h>
#include <rtgui/rtgui_system.h>

#if (LCD_VERSION == 2)
/*
MISO PA6
MOSI PA7
CLK  PA5
CS   PC4
*/

#define   CS_0()          GPIO_ResetBits(GPIOC,GPIO_Pin_4)
#define   CS_1()          GPIO_SetBits(GPIOC,GPIO_Pin_4)

/*
7  6 - 4  3      2     1-0
s  A2-A0 MODE SER/DFR PD1-PD0
*/
#define TOUCH_MSR_Y  0x90   //读X轴坐标指令 addr:1
#define TOUCH_MSR_X  0xD0   //读Y轴坐标指令 addr:3

struct rtgui_touch_device
{
	struct rt_device parent;

	rt_timer_t poll_timer;
	rt_uint16_t x, y;
};
static struct rtgui_touch_device *touch = RT_NULL;

extern unsigned char SPI_WriteByte(unsigned char data);
rt_inline void EXTI_Enable(rt_uint32_t enable);

//SPI写数据
static void WriteDataTo7843(unsigned char num)
{
    SPI_WriteByte(num);
}

#define X_MIN 	1870
#define X_MAX 	115
#define X_WIDTH 240

#define Y_MIN 	180
#define Y_MAX 	1935
#define Y_WIDTH 320

static void rtgui_touch_calculate()
{
	if (touch != RT_NULL)
	{
		CS_0();
		WriteDataTo7843(TOUCH_MSR_X | 1);                		/* 发送读X坐标命令并关闭中断 */
		touch->x = SPI_WriteByte(0x00)<<4;                      /* 读取第一字节MSB */
		touch->x |= ((SPI_WriteByte(TOUCH_MSR_Y | 1)>>4)&0x0F );/* 读取第二字节 同时发送读Y轴坐标命令行*/
		touch->y = SPI_WriteByte(0x00)<<4;                      /* 读取第一字节MSB */
		touch->y |= ((SPI_WriteByte(1<<7)>>4)&0x0F );           /* 读取第二字节并重新打开中断 */
		CS_1();

		// rt_kprintf("touch(%d, %d)\n", touch->x, touch->y);

		/* 校正 */
		if (X_MAX > X_MIN)
		{
			touch->x = (touch->x - X_MIN) * X_WIDTH/(X_MAX - X_MIN);
		}
		else
		{
			touch->x = (X_MIN - touch->x) * X_WIDTH/(X_MIN - X_MAX);
		}

		if (Y_MAX > Y_MIN)
		{
			touch->y = (touch->y - Y_MIN) * Y_WIDTH /(Y_MAX - Y_MIN);
		}
		else
		{
			touch->y = (Y_MIN - touch->y) * Y_WIDTH /(Y_MIN - Y_MAX);
		}
	}
}

void touch_timeout(void* parameter)
{
    struct rtgui_event_mouse emouse;

	if (GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1) != 0)
	{
		EXTI_Enable(1);
		emouse.parent.type = RTGUI_EVENT_MOUSE_BUTTON;
		emouse.button = (RTGUI_MOUSE_BUTTON_LEFT |RTGUI_MOUSE_BUTTON_UP);

		/* use old value */
		emouse.x = touch->x;
		emouse.y = touch->y;

		/* stop timer */
		rt_timer_stop(touch->poll_timer);
		rt_kprintf("touch up: (%d, %d)\n", emouse.x, emouse.y);
	}
	else
	{
		/* send mouse event */
		emouse.parent.type = RTGUI_EVENT_MOUSE_MOTION;
		emouse.parent.sender = RT_NULL;

		/* calculation */
		rtgui_touch_calculate();

		emouse.x = touch->x;
		emouse.y = touch->y;

		/* init mouse button */
		emouse.button = 0;
		rt_kprintf("touch motion: (%d, %d)\n", emouse.x, emouse.y);
	}

	/* send event to server */
	rtgui_server_post_event(&emouse.parent, sizeof(struct rtgui_event_mouse));
}

static void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable the EXTI0 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

rt_inline void EXTI_Enable(rt_uint32_t enable)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /* Configure  EXTI  */
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//Falling下降沿 Rising上升

	if (enable)
	{
		/* enable */
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	}
	else
	{
		/* disable */
		EXTI_InitStructure.EXTI_LineCmd = DISABLE;
	}

    EXTI_Init(&EXTI_InitStructure);
    EXTI_ClearITPendingBit(EXTI_Line1);
}

static void EXTI_Configuration(void)
{
    /* PB1 touch INT */
    {
        GPIO_InitTypeDef GPIO_InitStructure;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOB,&GPIO_InitStructure);
    }

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

    /* Configure  EXTI  */
    EXTI_Enable(1);
}

/* RT-Thread Device Interface */
static rt_err_t rtgui_touch_init (rt_device_t dev)
{
    SPI_InitTypeDef SPI_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);

    /* Enable SPI_MASTER */
    SPI_Cmd(SPI1, DISABLE);

    /*------------------------ SPI1 configuration ------------------------*/
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;//SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS  = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;/* 72M/64=1.125M */
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_I2S_DeInit(SPI1);
    SPI_Init(SPI1, &SPI_InitStructure);

    /* Enable SPI_MASTER */
    SPI_Cmd(SPI1, ENABLE);

    NVIC_Configuration();
    EXTI_Configuration();

    /* PC4 touch CS */
    {
        GPIO_InitTypeDef GPIO_InitStructure;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);

        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
        GPIO_Init(GPIOC,&GPIO_InitStructure);
        CS_1();
    }

    CS_0();
    WriteDataTo7843( 1<<7 ); /* 打开中断 */
    CS_1();

    return RT_EOK;
}

static rt_err_t rtgui_touch_control (rt_device_t dev, rt_uint8_t cmd, void *args)
{
	return RT_EOK;
}

void EXTI1_IRQHandler(void)
{
    struct rtgui_event_mouse emouse;

    /* calculation */
    rtgui_touch_calculate();

    /* send mouse event */
	emouse.parent.type = RTGUI_EVENT_MOUSE_BUTTON;
	emouse.parent.sender = RT_NULL;

	emouse.x = touch->x;
	emouse.y = touch->y;

	/* init mouse button */
	emouse.button = (RTGUI_MOUSE_BUTTON_LEFT |RTGUI_MOUSE_BUTTON_DOWN);

	rt_kprintf("touch down: (%d, %d)\n", emouse.x, emouse.y);

	/* send event to server */
	rtgui_server_post_event(&emouse.parent, sizeof(struct rtgui_event_mouse));

	/* disable interrupt */
	EXTI_Enable(0);
    EXTI_ClearITPendingBit(EXTI_Line1);

	/* start timer */
	rt_timer_start(touch->poll_timer);
}
#endif

void rtgui_touch_hw_init()
{
#if (LCD_VERSION == 2)
	touch = (struct rtgui_touch_device*)rt_malloc (sizeof(struct rtgui_touch_device));
	if (touch == RT_NULL) return; /* no memory yet */

	/* clear device structure */
	rt_memset(&(touch->parent), 0, sizeof(struct rt_device));

	/* init device structure */
	touch->parent.type = RT_Device_Class_Unknown;
	touch->parent.init = rtgui_touch_init;
	touch->parent.control = rtgui_touch_control;
	touch->parent.private = RT_NULL;

	/* create 0.5 second timer */
	touch->poll_timer = rt_timer_create("touch", touch_timeout, RT_NULL,
		RT_TICK_PER_SECOND/8, RT_TIMER_FLAG_PERIODIC);

	/* register touch device to RT-Thread */
	rt_device_register(&(touch->parent), "touch", RT_DEVICE_FLAG_RDWR);
#endif
}
