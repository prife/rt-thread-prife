#include "settings.h"
#include "at91lib.h"
#include "string.h"
#include "protocol.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "dev_emac.h"
#include "apptasks.h"

const unsigned char super_pswd[] = {'2','3','5','0','5','4','2','4',0};

const static struct t_conf
{
	char magic[4]; // magic number, indicates whether flash is initialized.
	// Device
    int serial_number;  // internal.
	unsigned char model[10]; //internal.
	unsigned char name[32];
	unsigned int firm_ver; // internal.
	unsigned int pswd_status;
	unsigned char password[12+1];

	// Network
	unsigned char mac[6];
	unsigned char dhcp_enabled;
	// there may be one byte padding.
	unsigned char ip[4];
	unsigned char netmask[4];
	unsigned char gateway[4];
	unsigned char dns[4];

	// Serial
	unsigned char baudrate;
	unsigned char databits;
	unsigned char stopbits;
	unsigned char parity;
	unsigned char flowcontrol;

	// Sockets
	unsigned char working_mode;
	// TCP server mode.
	unsigned short listen_port;
	// TCP client mode.
	unsigned char server_info[SERVER_INFO_BUFLEN];
	// UDP mode.
	unsigned short udp_local_port;
	unsigned short udp_remote_port;
	unsigned char udp_ip[128];
	// UDP multicast mode.
	unsigned short udpmc_local_port;
	unsigned short udpmc_remote_port;
	unsigned char udpmc_ip[4];
	
	// serial frame split.
	unsigned short max_interval;
	unsigned short max_datalen;


} pconf_content = {0xab}; // Let default content invalid.
struct t_conf *pconf = (struct t_conf*)&pconf_content;


unsigned int getPowerUpSeconds(void)
{
	// Warning!, tick will overflow after power up 497 days.
	return rt_tick_get()/RT_TICK_PER_SECOND;
}

int getPswdStatus(void)
{
	return pconf->pswd_status;
}
int setPswdStatus(unsigned int pswd_status)
{
	if( pswd_status >= PSWD_STATUS_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->pswd_status),&pswd_status,sizeof(pconf->pswd_status));
	return 1;
}

int verifyPswd(unsigned char *pswd)
{
	int len = strlen((char*)pswd)+1;
	if( len > sizeof(pconf->password) )
		return 0;
	if( !strcmp((char*)pswd,(char*)super_pswd) )
		return 1;
	if( !strcmp((char*)pswd,(char*)(pconf->password)) )
		return 1;
	else
		return 0;
}
int setPswd(unsigned char *pswd)
{
	int len = strlen((char*)pswd)+1;
	if( len > sizeof(pconf->password) )
		return 0;
	FLASHD_Write(ADDR(pconf->password),pswd,len);
	return 1;
}
int getSerialNum(void)
{
	return pconf->serial_number;
}
int setSerialNum(int serialNum)
{
	FLASHD_Write(ADDR(pconf->serial_number),&serialNum,sizeof(int));
	return 1;
}
unsigned char *getModel(void)
{
	return pconf->model;
}
unsigned int   setModel(unsigned char *model)
{
	int len = strlen((char*)model)+1;
	if( len > sizeof(pconf->model) )
		return 0;
	FLASHD_Write(ADDR(pconf->model),model,len);
		return 1;
}
unsigned char *getName(void)
{
	return pconf->name;
}
unsigned int   setName(unsigned char *name)
{
	int len = strlen((char*)name)+1;
	if( len > sizeof(pconf->name) )
		return 0;
	FLASHD_Write(ADDR(pconf->name),name,len);
		return 1;
	/*
	unsigned char zero = 0;
	FLASHD_Write(ADDR(pconf->name),name,
		strlen((char*)name)+1>sizeof(pconf->name) ? sizeof(pconf->name)
		: strlen((char*)name)+1 );
	if(strlen((char*)name)+1>sizeof(pconf->name)) // last byte mast be '\0'
		FLASHD_Write(ADDR(pconf->name)+sizeof(pconf->name)-1,&zero,1);
	return 1;
	*/
}
unsigned int getFirmVer(void)
{
	return pconf->firm_ver;
}
unsigned int setFirmVer(int ver)
{
	FLASHD_Write(ADDR(pconf->firm_ver),&ver,sizeof(int));
	return 1;
}
unsigned char *getMac(void)
{
	return pconf->mac;
}
unsigned int   setMac(unsigned char *mac)
{
	FLASHD_Write(ADDR(pconf->mac),mac,sizeof(pconf->mac));
	return 1;
}
unsigned int isDHCPEnabled(void)
{
	return pconf->dhcp_enabled;
}
unsigned int setDHCPEnabled(unsigned char enabled)
{
	if( enabled >= DHCP_STSTUS_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->dhcp_enabled),&enabled,sizeof(pconf->dhcp_enabled));
	return 1;
}
unsigned char *getCfgIP(void)
{
	return pconf->ip;
}
unsigned int   setCfgIP(unsigned char *ip)
{
	FLASHD_Write(ADDR(pconf->ip),ip,sizeof(pconf->ip));
	return 1;

}
unsigned char *getCfgNetMask(void)
{
	return pconf->netmask;
}
unsigned int   setCfgNetMask(unsigned char *nm)
{
	FLASHD_Write(ADDR(pconf->netmask),nm,sizeof(pconf->netmask));
	return 1;
}
unsigned char *getCfgGateWay(void)
{
	return pconf->gateway;
}
unsigned int   setCfgGateWay(unsigned char *gw)
{
	FLASHD_Write(ADDR(pconf->gateway),gw,sizeof(pconf->netmask));
	return 1;
}
unsigned char *getCfgDNS(void)
{
	return pconf->dns;
}
unsigned int   setCfgDNS(unsigned char *dns)
{
	FLASHD_Write(ADDR(pconf->dns),dns,sizeof(pconf->dns));
	return 1;
}

unsigned int getBytesSent(void)
{
	return usart_bytes_sent;
}
unsigned int getBytesRecv(void)
{
	return usart_bytes_recv;
}

unsigned char getWorkingMode(void)
{
	return pconf->working_mode;
}
unsigned int   setWorkingMode(unsigned char mode)
{
	if( mode >= MODE_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->working_mode),&mode,sizeof(pconf->working_mode));
	return 1;
}
unsigned short getListenPort(void)
{
	return pconf->listen_port;
}
unsigned int   setListenPort(unsigned short port)
{
	FLASHD_Write(ADDR(pconf->listen_port),&port,sizeof(pconf->listen_port));
	return 1;
}


unsigned char* getServersInfo(void)
{
	return pconf->server_info;
}
// NOTE: p==NULL clear the settings.
unsigned int setServersInfo(unsigned char *p)
{
	unsigned short len;
	if( p==NULL )
	{
		len = 0;
		FLASHD_Write(ADDR(pconf->server_info),&len,2);
	}
	else
	{
		len = SERVER_INFO_LEN(p);
		FLASHD_Write(ADDR(pconf->server_info),p,len);
	}
	return 1;
}
void getUDPModeInfo(unsigned short *localPort,unsigned short *remotePort,unsigned char **ip)
{
	*localPort = pconf->udp_local_port;
	*remotePort = pconf->udp_remote_port;
	*ip = pconf->udp_ip;
}
int setUDPModeInfo(unsigned short localPort, unsigned short remotePort,unsigned char *ip)
{
	FLASHD_Write(ADDR(pconf->udp_local_port),&localPort,2);
	FLASHD_Write(ADDR(pconf->udp_remote_port),&remotePort,2);
	FLASHD_Write(ADDR(pconf->udp_ip),ip,4);
	return 1;
}
void getUDPMCModeInfo(unsigned short *localPort,unsigned short *remotePort,unsigned char **ip)
{
	*localPort = pconf->udpmc_local_port;
	*remotePort = pconf->udpmc_remote_port;
	*ip = pconf->udpmc_ip;
}
int setUDPMCModeInfo(unsigned short localPort, unsigned short remotePort,unsigned char *ip)
{
	if( ip[0]>239 || ip[0]<224 ) //group mode ip range.
		return 0;
	FLASHD_Write(ADDR(pconf->udpmc_local_port),&localPort,2);
	FLASHD_Write(ADDR(pconf->udpmc_remote_port),&remotePort,2);
	FLASHD_Write(ADDR(pconf->udpmc_ip),ip,4);
	return 1;
}

unsigned int getBaudrate(void)
{
	return pconf->baudrate;
}
unsigned int setBaudrate(unsigned char baudrate)
{
	if( baudrate >= BR_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->baudrate),&baudrate,1);
	return 1;
}
unsigned int getDatabits(void)
{
	return pconf->databits;
}
unsigned int setDatabits(unsigned char databits)
{
	if( databits >= DB_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->databits),&databits,1);
	return 1;
}
unsigned int getStopbits(void)
{
	return pconf->stopbits;
}
unsigned int setStopbits(unsigned char stopbits)
{
	if( stopbits >= SB_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->stopbits),&stopbits,1);
	return 1;
}
unsigned int getParity(void)
{
	return pconf->parity;
}
unsigned int setParity(unsigned char parity)
{
	if( parity >= PA_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->parity),&parity,1);
	return 1;
}
unsigned int getFlowcontrol(void)
{
	return pconf->flowcontrol;
}
unsigned int setFlowcontrol(unsigned char flowcontrol)
{
	if( flowcontrol >= FC_NUM )
		return 0;
	FLASHD_Write(ADDR(pconf->flowcontrol),&flowcontrol,1);
	return 1;
}
void getFrameSplit(unsigned short *max_interval,unsigned short *max_datalen)
{
	*max_interval = pconf->max_interval;
	*max_datalen = pconf->max_datalen;
}
	
unsigned int setFrameSplit(unsigned short max_interval,unsigned short max_datalen)
{
	if( max_datalen > 1024 || max_datalen == 0 )
		return 0;
	FLASHD_Write(ADDR(pconf->max_interval),&max_interval,2);
	FLASHD_Write(ADDR(pconf->max_datalen),&max_datalen,2);
	return 1;
}

unsigned int setResetDevice(void)
{
	RSTC_SetUserResetEnable(1);
	RSTC_ChipReset();
	// Never reach here.
	return 1;
}

#ifndef CONFIG_DEBUG
static const Pin PIN_PA7 = {0x01<<7, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_1, PIO_PULLUP};
#endif

static void setFirstBootInit(void)
{
#ifndef CONFIG_DEBUG
	PIO_Configure(&PIN_PA7,1); // for ease to erase(touch pin91 and 92).
	FLASHD_SetSecurityBit();
#endif
	setModel(DEFAULT_MODEL);
	setSerialNum(DEFAULT_SERIAL);
	setFirmVer(DEFAULT_FIRM_VER);
	setMac(DEFAULT_MAC);	// default mac
}
unsigned int setReturnToFactory(void)
{
	setPswdStatus(DEFAULT_PSWD_STATUS);
	setPswd(DEFAULT_PSWD);
	setName(DEFAULT_NAME);

	setDHCPEnabled(DEFAULT_DHCP);
	setCfgIP(DEFAULT_IP);
	setCfgNetMask(DEFAULT_NETMASK);
	setCfgGateWay(DEFAULT_GW);
	setCfgDNS(DEFAULT_DNS);
	setWorkingMode(DEFAULT_MODE);
	setListenPort(DEFAULT_LISTEN_PORT);
	setServersInfo(DEFAULT_SERVERS_INFO);
	setUDPModeInfo(DEFAULT_UDP_LOCALPORT,DEFAULT_UDP_REMOTEPORT,DEFAULT_UDP_IP);
	setUDPMCModeInfo(DEFAULT_UDPMC_LOCALPORT,DEFAULT_UDPMC_REMOTEPORT,DEFAULT_UDPMC_IP);
	setBaudrate(DEFAULT_BAUDRATE);
	setDatabits(DEFAULT_DATABITS);
	setStopbits(DEFAULT_STOPBITS);
	setParity(DEFAULT_PARITY);
	setFlowcontrol(DEFAULT_FLOWCONTROL);
	setFrameSplit(DEFAULT_MAX_INTERVAL,DEFAULT_MAX_DATALEN);
	// we should write magic number at last.
	FLASHD_Write(ADDR(pconf->magic),FLASH_MAGIC,sizeof(pconf->magic));
	return 1;
}



//////////////////////////////////////////////////////////////////
struct ip_addr getCfgDNSaddr()
{
	struct ip_addr dns_addr;
	unsigned char *pdns = getCfgDNS();
	IP4_ADDR(&dns_addr,pdns[0],pdns[1],pdns[2],pdns[3]);
	return dns_addr;
}
static unsigned char ipbuf[4];
#define IP2UCHAR(pbuf,ipaddr) do{struct ip_addr ip=ipaddr;pbuf[0]=ip4_addr1(&(ip));\
pbuf[1]=ip4_addr2(&(ip));pbuf[2]=ip4_addr3(&(ip));pbuf[3]=ip4_addr4(&(ip));}while(0)
unsigned char *getIfIP(void)
{
	IP2UCHAR(ipbuf,netif_default->ip_addr);
	return ipbuf;
}
unsigned char *getIfNetMask(void)
{
	IP2UCHAR(ipbuf,netif_default->netmask);
	return ipbuf;
}
unsigned char *getIfGateWay(void)
{
	IP2UCHAR(ipbuf,netif_default->gw);
	return ipbuf;
}
unsigned char *getIfDNS(void)
{
	IP2UCHAR(ipbuf,dns_getserver(0));
	return ipbuf;
}
//////////////////////////////////////////////////////////////////


// NOTE: these value should according to the definitions in settings.h
const unsigned int br_list[] = {\
	110,300,600,1200,2400,4800,9600,14400,19200,28800,38400,57600,115200,
	230400,460800,921600};
const unsigned int db_list[] = {\
AT91C_US_CHRL_5_BITS,AT91C_US_CHRL_6_BITS,AT91C_US_CHRL_7_BITS,AT91C_US_CHRL_8_BITS,};
const unsigned int sb_list[] = {\
AT91C_US_NBSTOP_1_BIT,AT91C_US_NBSTOP_15_BIT,AT91C_US_NBSTOP_2_BIT,};
const unsigned int cs_list[] = {\
AT91C_US_PAR_NONE,AT91C_US_PAR_EVEN,AT91C_US_PAR_ODD,AT91C_US_PAR_MARK,AT91C_US_PAR_SPACE};


void usart_reconfigure(void)
{
    unsigned int mode = AT91C_US_USMODE_NORMAL
                        | AT91C_US_CLKS_CLOCK
                        | AT91C_US_CHMODE_NORMAL;
	mode |= db_list[getDatabits()];
	mode |= sb_list[getStopbits()];
	mode |= cs_list[getParity()];
	USART_Configure(AT91C_BASE_US0, mode,br_list[getBaudrate()], BOARD_MCK);
	USART_Configure(AT91C_BASE_US1, mode,br_list[getBaudrate()], BOARD_MCK);
	USART_SetTransmitterEnabled(AT91C_BASE_US0, 1);
	USART_SetTransmitterEnabled(AT91C_BASE_US1, 1);
	USART_SetReceiverEnabled(AT91C_BASE_US0, 1);
	USART_SetReceiverEnabled(AT91C_BASE_US1, 1);
}

void settings_powerup_init(void)
{
	// if flash is not initialized, init it first.
	// NOTE: the MAC address are initialized before here, so
	// we should reboot device after writing default values to flash.
	//setReturnToFactory();
	if( memcmp(&pconf->magic,FLASH_MAGIC,sizeof(pconf->magic)) )
	{
		setFirstBootInit();
		setReturnToFactory();
		setResetDevice();
	}
	// re-configure serial.
	usart_reconfigure();
	// lwip should be configured by it self.
}
