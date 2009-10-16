#ifndef FMT0371_H_INCLUDED
#define FMT0371_H_INCLUDED

//----------  LCD_RESET -------------
#define LCD_RST_PORT          GPIOF
#define LCD_RST_PIN           GPIO_Pin_10
#define LCD_RST_RCC           RCC_APB2Periph_GPIOF
/**************************************/
#define LCD_RST_0             GPIO_ResetBits(LCD_RST_PORT,LCD_RST_PIN)
#define LCD_RST_1             GPIO_SetBits(LCD_RST_PORT,LCD_RST_PIN)
//----------  LCD_RESET -------------

#define LCD_ADDR              (*((volatile unsigned char *) 0x64000000)) // RS = 0
#define LCD_DATA              (*((volatile unsigned char *) 0x64000004)) // RS = 1

#define LCD_DATA16(a)         LCD_DATA = (unsigned char)(a>>8);LCD_DATA = (unsigned char)a // RS = 1 & WIDHT = 16
#define LCD_DATA16_READ(a)	  do { a = (LCD_DATA << 8) | (LCD_DATA); } while (0)
#define LCD_WR_CMD(a,b,c)     LCD_ADDR = b;LCD_DATA16(c)
#define LCD_WR_REG(a)         LCD_ADDR = a
#define LCD_WR_DATA8(a)       LCD_DATA = a

extern void ftm0371_port_init(void);
extern void ftm0371_init(void);

/*
16位(R5G6B5)
内存范围
0x02   D7:D0  X起始地址
0x03   D8:D0  Y起始地址
0x04   D7:D0  X结束地址
0x05   D8:D0  Y结束地址
*/

#endif // FMT0371_H_INCLUDED
