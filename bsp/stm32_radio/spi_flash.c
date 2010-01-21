#include <stm32f10x.h>
//#include "spi_flash.h"
#include   <string.h>

extern unsigned char SPI_WriteByte(unsigned char data);

/********************** hardware *************************************/
/* SPI_FLASH_CS   PA4 */
/* SPI_FLASH_RST  PA3 */
#define FLASH_RST_0()    GPIO_ResetBits(GPIOA,GPIO_Pin_3)
#define FLASH_RST_1()    GPIO_SetBits(GPIOA,GPIO_Pin_3)

#define FLASH_CS_0()     GPIO_ResetBits(GPIOA,GPIO_Pin_4)
#define FLASH_CS_1()     GPIO_SetBits(GPIOA,GPIO_Pin_4)
/********************** hardware *************************************/

static void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    FLASH_RST_0(); // RESET
    FLASH_RST_1();
}

static unsigned char SPI_HostReadByte(void)
{
    return SPI_WriteByte(0x00);
}

static void SPI_HostWriteByte(unsigned char wByte)
{
    SPI_WriteByte(wByte);
}

/******************************************************************************/
/*Status Register Format:                                   */
/*   ----------------------------------------------------------------------- */
/* | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 | */
/* |--------|--------|--------|--------|--------|--------|--------|--------| */
/* |RDY/BUSY| COMP |   0   |   1   |   1   |   1   |   X   |   X   | */
/*   ----------------------------------------------------------------------- */
/* bit7 - 忙标记，0为忙1为不忙。                               */
/*       当Status Register的位0移出之后，接下来的时钟脉冲序列将使SPI器件继续*/
/*       将最新的状态字节送出。                               */
/* bit6 - 标记最近一次Main Memory Page和Buffer的比较结果，0相同，1不同。   */
/* bit5                                               */
/* bit4                                               */
/* bit3                                               */
/* bit2 - 这4位用来标记器件密度，对于AT45DB041B，这4位应该是0111，一共能标记 */
/*       16种不同密度的器件。                               */
/* bit1                                               */
/* bit0 - 这2位暂时无效                                     */
/******************************************************************************/
static unsigned char AT45DB_StatusRegisterRead(void)
{
    unsigned char i;

    FLASH_CS_0();
    SPI_HostWriteByte(0xd7);
    i=SPI_HostReadByte();
    FLASH_CS_1();

    return i;
}

static void wait_busy(void)
{
    unsigned int    i=0;
    while (i++<255)
    {
        if (AT45DB_StatusRegisterRead()&0x80)
        {
            break;
        }
    }
}

static void read_page(unsigned int page,unsigned char * pHeader)
{
    unsigned int i=0;

    wait_busy();

    FLASH_CS_0();
    SPI_HostWriteByte(0x53);
    SPI_HostWriteByte((unsigned char)(page >> 6));
    SPI_HostWriteByte((unsigned char)(page << 2));
    SPI_HostWriteByte(0x00);
    FLASH_CS_1();

    wait_busy();

    FLASH_CS_0();
    SPI_HostWriteByte(0xD4);
    SPI_HostWriteByte(0x00);
    SPI_HostWriteByte(0x00);
    SPI_HostWriteByte(0x00);
    SPI_HostWriteByte(0x00);
    for (i=0; i<512; i++)
    {
        *pHeader++ = SPI_HostReadByte();
    }
    FLASH_CS_1();

}

static void write_page(unsigned int page,unsigned char * pHeader)
{
    unsigned int i;

    wait_busy();

    FLASH_CS_0();
    SPI_HostWriteByte(0x87);
    SPI_HostWriteByte(0);
    SPI_HostWriteByte(0);
    SPI_HostWriteByte(0);
    for(i=0; i<512; i++)
    {
        SPI_HostWriteByte(*pHeader++);
    }
    FLASH_CS_1();

    wait_busy();

    FLASH_CS_0();
    SPI_HostWriteByte(0x86);
    SPI_HostWriteByte((unsigned char)(page>>6));
    SPI_HostWriteByte((unsigned char)(page<<2));
    SPI_HostWriteByte(0x00);
    FLASH_CS_1();
}


#include <rtthread.h>
/* SPI DEVICE */
static struct rt_device spi_flash_device;

/* RT-Thread Device Driver Interface */
static rt_err_t rt_spi_flash_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_spi_flash_open(rt_device_t dev, rt_uint16_t oflag)
{

    return RT_EOK;
}

static rt_err_t rt_spi_flash_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_spi_flash_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    return RT_EOK;
}

static rt_size_t rt_spi_flash_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    /* only supply single block read: block size 512Byte */
    read_page(pos/512,buffer);
    return RT_EOK;
}

static rt_size_t rt_spi_flash_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    /* only supply single block write: block size 512Byte */
    write_page(pos/512,(unsigned char*)buffer);
    return RT_EOK;
}

void rt_hw_spi_flash_init(void)
{
    GPIO_Configuration();

    /* register spi_flash device */
    spi_flash_device.init    = rt_spi_flash_init;
    spi_flash_device.open    = rt_spi_flash_open;
    spi_flash_device.close   = rt_spi_flash_close;
    spi_flash_device.read 	 = rt_spi_flash_read;
    spi_flash_device.write   = rt_spi_flash_write;
    spi_flash_device.control = rt_spi_flash_control;

    /* no private */
    spi_flash_device.private = RT_NULL;

    rt_device_register(&spi_flash_device, "spi0",
                       RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);
}
