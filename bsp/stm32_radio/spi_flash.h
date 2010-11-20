#ifndef SPI_FLASH_H_INCLUDED
#define SPI_FLASH_H_INCLUDED

/*
 * WARNING: !!! ENABLING DMA MODE MAY DESTROY YOUR DATA IN THE SPI FLASH !!!
 * Don't set SPI_FLASH_USE_DMA to 1 unless you know what you're doing!
 * However, readonly access is just fine. :)
 */
#define SPI_FLASH_USE_DMA         1
#define FLASH_SECTOR_SIZE	      512
#define SST_WRITE_BUFFER          1

extern void rt_hw_spi_flash_init(void);

#endif /* SPI_FLASH_H_INCLUDED */
