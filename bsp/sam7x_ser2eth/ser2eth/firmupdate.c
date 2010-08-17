//
// NOTE: All code must be located in ram instead of flash.
//

#include "firmupdate.h"
#include "AT91SAM7X256.h"
#include "efc.h"
#include "rtthread.h"
#include "rthw.h"

#define GUID_LEN 36
// encrypt data is 2 long value.
const static char guid[] = "46B3CFC8-B05C-4253-A1A6-8BA1400E071E";
static long key[4] = {0xFE209Ab5,0xC4B2536A,0x9918FCB6,0x38DC8EB7}; // should not be const.
static int slide_index = 0;
static long enc_databuf[2];
static char mode = 0;
// mode 0: slide guid
// mode 1: check encrypt data.

//////////////////////////////////////////////////////////////////
// xxtea
static long idiv(long a,long b)
{
	long c = 0;;
	while(a>=b)
    {
    	c++;
    	a=a-b;
	}
	return c;
}

#define MX (z>>5^y<<2) + (y>>3^z<<4)^(sum^y) + (k[p&3^e]^z);

long btea(long* v, long n, long* k) {
	unsigned long z=v[n-1], y=v[0], sum=0, e, DELTA=0x9e3779b9;
	long p, q ;
	if (n > 1) {
		//q = 6 + 52/n;
		q = 6 + idiv(52,n);
		while (q-- > 0) {
			sum += DELTA;
			e = (sum >> 2) & 3;
			for (p=0; p<n-1; p++) y = v[p+1], z = v[p] += MX;
			y = v[0];
			z = v[n-1] += MX;
		}
		return 0 ;
	} else if (n < -1) {
		n = -n;
		//q = 6 + 52/n;
		q = 6 + idiv(52,n);
		sum = q*DELTA ;
		while (sum != 0) {
			e = (sum >> 2) & 3;
			for (p=n-1; p>0; p--) z = v[p-1], y = v[p] -= MX;
			z = v[n-1];
			y = v[0] -= MX;
			sum -= DELTA;
		}
		return 0;
	}
	return 1;
}
//////////////////////////////////////////////////////////////////

static char readChar(void)
{
	 while ((AT91C_BASE_US0->US_CSR & AT91C_US_RXRDY) == 0);
	 return (char)(AT91C_BASE_US0->US_RHR);
}

static void sendChar(char ch)
{
	while ((AT91C_BASE_US0->US_CSR & AT91C_US_TXRDY) == 0);
	AT91C_BASE_US0->US_THR = ch;
}


void flash_writepages(unsigned int *destPageAddr,unsigned int *srcPageAddr, int pages)
{
	int size;
	int startPage;

	startPage = ((int)destPageAddr - AT91C_IFLASH) / AT91C_IFLASH_PAGE_SIZE;
	if( pages <= 0 )
		return;

	while( pages-- )
	{
		unsigned int status;
		for( size = AT91C_IFLASH_PAGE_SIZE; size >=4; size -=4 )
		{
			*destPageAddr++ = *srcPageAddr++;
		}

		// begin program.
		#define FMCN_FLASH(mck)         ((((mck) / 2000000) * 3) << 16)
		AT91C_BASE_EFC->EFC_FMR = (AT91C_BASE_EFC->EFC_FMR & ~AT91C_MC_FMCN) | FMCN_FLASH(BOARD_MCK);
		AT91C_BASE_EFC->EFC_FCR = (0x5A << 24) | (startPage << 8) | AT91C_MC_FCMD_START_PROG;
	    do
		{

	        status = AT91C_BASE_EFC->EFC_FSR;
	    }
	    while ((status & AT91C_MC_FRDY) == 0);

		// end program.
		startPage++;
	}

}


static void updateFirmware(void)
{
	char ch;
	char *pbuf;
	int block = 0;

	pbuf = rt_malloc(1024); // must be 4 bytes aligned.

	if( pbuf == RT_NULL )
	{
		rt_kprintf("Not enough memory!\n");
		return;
	}

// From now, all code should be in ram.
	while(1)
	{
		int i;
		char crc8;

		// recv 0x55 flag.
		ch = readChar();
		if( ch != 0x55 ) // if ch == 0xaa, we just return and reset.
			return;

		// recv data and calculate crc.
		crc8 = 0;
		for( i = 0; i < 1024; i++ )
		{
			pbuf[i] = readChar();
			crc8 ^= pbuf[i];
		}

		// check crc.
		if( crc8 != readChar() )
		{
			sendChar(0x44);
			continue;
		}

		// crc is ok, we decrypt data and write to flash.
		btea((long*)pbuf,-(1024/4),key);

		flash_writepages((unsigned int*)(AT91C_IFLASH+block*1024),
			(unsigned int*)pbuf,1024/AT91C_IFLASH_PAGE_SIZE);
		block++;

		// after all, we send 0x66.
		sendChar(0x66);
	}
}

static void resetDevice(void)
{
	AT91C_BASE_RSTC->RSTC_RMR = AT91C_BASE_RSTC->RSTC_RMR &
		(~AT91C_RSTC_KEY) | AT91C_RSTC_URSTEN | (0xA5u << 24);
	AT91C_BASE_RSTC->RSTC_RCR = AT91C_RSTC_PERRST | AT91C_RSTC_PROCRST | (0xA5u << 24);
}

//////////////////////////////////////////////////////////////////

void checkFirmUpdateMagic(char c)
{
	switch(mode)
	{
		case 0:
		{
			if( c != guid[slide_index] )
			{
				slide_index = 0;
				return;
			}
			slide_index++;
			if( slide_index == GUID_LEN ) // enter mode 1.
			{
				slide_index = 0;
				mode = 1;
			}
			break;
		}
		case 1:
		{
			((char*)enc_databuf)[slide_index] = c;
			slide_index++;
			if( slide_index == 8 ) // check encrypt.
			{
				btea(enc_databuf,-2,key);
				if( enc_databuf[0] & enc_databuf[1] )
				{
					// encryption error.
					slide_index = 0;
					mode = 0;
					return;
				}
				else
				{
					int i;
					// OK, we enter update mode.
					rt_hw_interrupt_disable(); // disable interrupt forever.

					// first, send back guid.
					for( i = 0; i < GUID_LEN; i++ )
						sendChar(guid[i]);

					// second, flash.
					updateFirmware();

					// third, reset.
					resetDevice();
				}
			}
			break;
		}
	}

}

