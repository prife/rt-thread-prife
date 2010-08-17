#include <rthw.h>
#include <rtthread.h>
#include <netif/ethernetif.h>
#include "dev_emac.h"
#include "AT91SAM7X256.h"
#include "at91lib.h"
#include "mii.h"
#include <string.h>
#include <ctype.h>
#include "settings.h"


static const Pin PINS_EMAC = {0x3FFFF, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_PERIPH_A, PIO_DEFAULT};
struct eth_device dev_emac;
unsigned char rx_tx_tmpbuf[EMAC_FRAME_LENTGH_MAX];
static struct rt_semaphore tx_sem;

#ifdef DUMP_PACKET
#define DUMP_LINE_LEN 16
static void hex_dump(unsigned char *buf, unsigned int size)
{
	int lineCnt = 0;
	unsigned char *pch = buf;
	unsigned char linebuf[DUMP_LINE_LEN+1];
	rt_kprintf("\n");
	while( pch != (buf+size) )
	{
		if( lineCnt == 0)
			rt_kprintf("%08X ",pch-buf);
		rt_kprintf("%02X ",*pch);
		pch++;
		lineCnt++;
		if( lineCnt == DUMP_LINE_LEN )
		{
			int tmp;
			memmove(linebuf,pch-DUMP_LINE_LEN,DUMP_LINE_LEN);
			for( tmp=0;tmp<DUMP_LINE_LEN;tmp++ )
				isalnum(linebuf[tmp])?tmp:(linebuf[tmp]='.');
			linebuf[DUMP_LINE_LEN] = 0;
			rt_kprintf("%s\n",linebuf);
			lineCnt = 0;
		}
	}
}
#endif //DUMP_PACKET

void emac_isr(int vector)
{
    // RX packet
    if ( (AT91C_BASE_EMAC->EMAC_ISR & AT91C_BASE_EMAC->EMAC_IMR & AT91C_EMAC_RCOMP) ||
		AT91C_BASE_EMAC->EMAC_RSR & AT91C_EMAC_REC )
    {
		// A frame has been received.
		eth_device_ready(&dev_emac);
    }
    EMAC_Handler();
}

static volatile int delay=999999;
static rt_err_t dev_emac_init(rt_device_t dev)
{
	//unsigned int id1,id2;
	unsigned int status;

	// wait for 8201 start.
	while(delay--);
    // Init EMAC.
    EMAC_Init(AT91C_ID_EMAC,NULL, EMAC_CAF_ENABLE, EMAC_NBC_DISABLE);
	EMAC_SetMdcClock(BOARD_MCK);
	EMAC_EnableMdio();
	EMAC_EnableMII();
	PIO_Configure(&PINS_EMAC, 1);
/*
	// reset 8201.
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_BMCR,&status,0);
	EMAC_WritePhy(AT91C_PHY_ADDR,MII_BMCR,BMCR_RESET,0);
	// delay more than 10ms.
	delay=99;
	while(delay--);
	// re-enable 8201.
	status &= (~BMCR_RESET);
	EMAC_WritePhy(AT91C_PHY_ADDR,MII_BMCR,status,0);
*/	

// dump registers
{
	unsigned int reg;
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_BMCR,&reg,0);
	rt_kprintf("MII_BMCR      %#x\n",reg);
	if( (reg & 0x1000) == 0 ) goto Failed;
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_BMSR,&reg,0);
	rt_kprintf("MII_BMSR      %#x\n",reg);
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_PHYSID1,&reg,0);
	rt_kprintf("MII_PHYSID1   %#x\n",reg);
	if( reg != 0 ) goto Failed;
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_PHYSID2,&reg,0);
	rt_kprintf("MII_PHYSID2   %#x\n",reg);
	if( reg != 0x8201 ) goto Failed;
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_ADVERTISE,&reg,0);
	rt_kprintf("MII_ADVERTISE %#x\n",reg);
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_LPA,&reg,0);
	rt_kprintf("MII_LPA       %#x\n",reg);
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_EXPANSION,&reg,0);
	rt_kprintf("MII_EXPANSION %#x\n",reg);
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_NSR,&reg,0);
	rt_kprintf("MII_NSR       %#x\n",reg);
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_LBREMR,&reg,0);
	rt_kprintf("MII_LBREMR    %#x\n",reg);
	//if( reg != 0x5c0 ) goto Failed;
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_DCOUNTER,&reg,0);
	rt_kprintf("MII_DCOUNTER  %#x\n",reg);
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_FCSCOUNTER,&reg,0);
	rt_kprintf("MII_FCSCOUNTER %#x\n",reg);
	EMAC_ReadPhy(AT91C_PHY_ADDR,MII_PHYADDR,&reg,0);
	rt_kprintf("MII_PHYADDR   %#x\n",reg);
//#ifdef RTL8201CP
	//if( ((reg>>7) & 0x1F) != 1 ) goto Failed;
//#endif
//#ifdef RTL8201BL
	//if( ((reg>>8) & 0x1F) != 1 ) goto Failed;
//#endif
}

	/////////////////////////////////////////////////
	// Init PHY, poll id register.
	/*
    do
    {
        if(!EMAC_ReadPhy(AT91C_PHY_ADDR,MII_PHYSID1,&id1,0))
            goto Failed;
        if(!EMAC_ReadPhy(AT91C_PHY_ADDR,MII_PHYSID2,&id2,0))
            goto Failed;
    }while( ((id2 << 16) | (id1 & 0xffff)) != MII_RTL8201_ID );
	*/

    // Read BMSR and set link speed.
	//if(!EMAC_ReadPhy(AT91C_PHY_ADDR,MII_BMSR,&status,0))
		//goto Failed;
/*
	if( (status&BMSR_ANEGCOMPLETE)==0 )
	{
		rt_kprintf("Auto-negotiation timeout, default: 100Mbps Full-Duplex.\n");
        status = LPA_100FULL;
	}
    else
    {
	    rt_kprintf("Auto-negotiation OK.\n");
        if(!EMAC_ReadPhy(AT91C_PHY_ADDR, MII_LPA, &status, 0))
		    goto Failed;
    }
*/
	// Set link speed.
	status = LPA_100FULL;
    EMAC_SetLinkSpeed(status&(LPA_100FULL|LPA_100HALF),status&(LPA_100FULL|LPA_10FULL));

	/* setup interrupt */
	rt_hw_interrupt_install(AT91C_ID_EMAC, emac_isr, RT_NULL);
	*(volatile unsigned int*)(0xFFFFF000 + AT91C_ID_EMAC * 4) = AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL | 5;
	rt_hw_interrupt_umask(AT91C_ID_EMAC);

	return RT_EOK;

Failed:
    rt_kprintf("PHY init error, reset device!\n\n");
	setResetDevice();
    return RT_ERROR;

}

/* control the interface */
static rt_err_t dev_emac_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
	switch(cmd)
	{
	case NIOCTL_GADDR:
		/* get mac address */
		if(args)
			rt_memcpy(args, getMac(), 6);
		else return -RT_ERROR;
		break;
	default :
		break;
	}

	return RT_EOK;
}

/* Open the ethernet interface */
static rt_err_t dev_emac_open(rt_device_t dev, rt_uint16_t oflags)
{
	return RT_EOK;
}

/* Close the interface */
static rt_err_t dev_emac_close(rt_device_t dev)
{
	return RT_EOK;
}

/* Read */
static rt_size_t dev_emac_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
	rt_set_errno(-RT_ENOSYS);
	return 0;
}

/* Write */
static rt_size_t dev_emac_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
	rt_set_errno(-RT_ENOSYS);
	return 0;
}

/* ethernet device interface */
/*
 * Transmit packet.
 */
static rt_err_t dev_emac_tx( rt_device_t dev, struct pbuf* p)
{
	struct pbuf* q;
    unsigned char *pos = rx_tx_tmpbuf;

	/* lock tx operation */
	rt_sem_take(&tx_sem, RT_WAITING_FOREVER);

	for (q = p; q != NULL; q = q->next)
	{
        memmove(pos,q->payload,q->len);
        pos += q->len;
    }
    EMAC_Send(rx_tx_tmpbuf,(pos-rx_tx_tmpbuf),RT_NULL);

#ifdef DUMP_PACKET
	rt_kprintf("Dumping Tx package.\n");
	hex_dump(rx_tx_tmpbuf,(pos-rx_tx_tmpbuf));
#endif //DUMP_PACKET

	rt_sem_release(&tx_sem);

	return 0;
}

static struct pbuf* dev_emac_rx(rt_device_t dev)
{
	struct pbuf *p = RT_NULL;
	struct pbuf *q = RT_NULL;
    unsigned int recvSize;
    unsigned char result;
    unsigned char *pos;

    do
    {
        result = EMAC_Poll(rx_tx_tmpbuf,EMAC_FRAME_LENTGH_MAX,&recvSize);
        if( result == EMAC_RX_OK )
            break;
        else if( result == EMAC_RX_NO_DATA )
            return RT_NULL;
        else
            rt_kprintf("EMAC_Poll() returns with error\n");
    }while(1);

#ifdef DUMP_PACKET
	rt_kprintf("Dumping Rx package.\n");
	hex_dump(rx_tx_tmpbuf,recvSize);
#endif //DUMP_PACKET

    p = pbuf_alloc(PBUF_LINK,recvSize, PBUF_RAM);
    if( p == RT_NULL )
    {
        rt_kprintf("dev_emac_rx() failed to alloc pbuf\n");
        return RT_NULL;
    }
    pos = rx_tx_tmpbuf;
    for(q = p; q != RT_NULL; q= q->next)
    {
        memmove(q->payload,pos,q->len);
        pos += q->len;
    }

    return p;
}

int sam7xether_register(char *name)
{
	volatile rt_err_t result;

	/* init rt-thread device interface */
	dev_emac.parent.init	= dev_emac_init;
	dev_emac.parent.open	= dev_emac_open;
	dev_emac.parent.close	= dev_emac_close;
	dev_emac.parent.read	= dev_emac_read;
	dev_emac.parent.write	= dev_emac_write;
	dev_emac.parent.control	= dev_emac_control;
	dev_emac.eth_rx			= dev_emac_rx;
	dev_emac.eth_tx			= dev_emac_tx;

	rt_sem_init(&tx_sem, "emac", 1, RT_IPC_FLAG_FIFO);

	result = eth_device_init(&(dev_emac), (char*)name);
	RT_ASSERT(result == RT_EOK);

	return RT_EOK;
}
