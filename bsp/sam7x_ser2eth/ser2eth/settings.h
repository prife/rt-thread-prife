#ifndef SETTINGS_H
#define SETTINGS_H

#include "protocol.h"

#define FLASH_MAGIC		"X3K!"
#define ADDR(x)	((unsigned int)(&(x)))

//################################################
// Server info format, all 
// // [Server number 2B] (big-endian)
// // [Server0 url 128B] + [Server0 port 2B] (big-endian)
// // [Server1 ...
// // ...
#define URL_LEN	128
#define PORT_LEN 2
#define SERVER_INFO_LEN(p)	((RBIGU2(p)*(URL_LEN+PORT_LEN))+2)
#define MAX_SERVER_NUMBER	8
#define SERVER_INFO_BUFLEN ((URL_LEN+PORT_LEN)*MAX_SERVER_NUMBER+2)

// Machine default settings.
#define DEFAULT_PSWD_STATUS PSWD_DISABLED
#define DEFAULT_PSWD    "1234"
#define DEFAULT_MODEL   "X3000"
#define DEFAULT_SERIAL	1000
#define DEFAULT_FIRM_VER (201)
#define DEFAULT_NAME    "Device Name"
// MT
#define DEFAULT_MAC     "\x00\x4D\x54\x01\x00\x01"
#define DEFAULT_DHCP    0
#define DEFAULT_IP      "\x0A\x00\x00\x0A"
#define DEFAULT_NETMASK "\xff\xff\xff\x00"
#define DEFAULT_GW      "\x0A\x00\x00\x01"
#define DEFAULT_DNS     "\xCA\x61\x10\xC3" //202.97.16.195 
#define DEFAULT_MODE          TCP_AUTO
// TCP server
#define DEFAULT_LISTEN_PORT   1984
// TCP client
#define DEFAULT_SERVERS_INFO  NULL
// UDP
#define DEFAULT_UDP_LOCALPORT  2010
#define DEFAULT_UDP_REMOTEPORT 2011
#define DEFAULT_UDP_IP       "\xFF\xFF\xFF\xFF"
// UDP Multicast
#define DEFAULT_UDPMC_LOCALPORT 2012
#define DEFAULT_UDPMC_REMOTEPORT 2013
#define DEFAULT_UDPMC_IP      "\xEA\x05\x06\x07" //234.5.6.7

#define DEFAULT_BAUDRATE      BR9600
#define DEFAULT_DATABITS      DB8
#define DEFAULT_STOPBITS      SB1
#define DEFAULT_PARITY        PANONE
#define DEFAULT_FLOWCONTROL   FCNONE

#define DEFAULT_MAX_INTERVAL  20
#define DEFAULT_MAX_DATALEN   1024

// ONLY this section should be copyed to smartsetlib.h
///------------------------------------------------------
///------------------------------------------------------
// Password status
enum
{
	PSWD_DISABLED = 0,
	PSWD_ENABLED,
	PSWD_STATUS_NUM,
};
enum
{
	DHCP_DISABLED = 0,
	DHCP_ENABLED,
	DHCP_STSTUS_NUM,
};

// Working Mode
enum
{
	TCP_SERVER = 0,
	TCP_CLIENT,
	TCP_AUTO,
	UDP,
	UDP_MULTICAST,
	MODE_NUM,
};
// BaudRate
enum
{
    BR110 = 0,
    BR300,
    BR600,
    BR1200,
    BR2400,
    BR4800,
    BR9600,
    BR14400,
    BR19200,
    BR28800,
    BR38400,
    BR57600,
    BR115200,
	BR230400,
	BR460800,
	BR921600,
    BR_NUM,
};
// Data bits
enum
{
    DB5 = 0,
    DB6,
    DB7,
    DB8,
    DB_NUM,
};
// Stop bits
enum
{
    SB1 = 0,
    SB15,
    SB2,
    SB_NUM,
};
// Parity
enum
{
	PANONE = 0,
	PAEVEN,
	PAODD,
	PAMARK,
	PASPACE,
	PA_NUM,
};
// Flow control
enum
{
	FCNONE = 0,
	FCXONXOFF,
	FCHARDWARE,
	FC_NUM,
};
///------------------------------------------------------
///------------------------------------------------------

// IMPORTANT!!! Those "setxxx" functions are ONLY used to write
// configurations to flash. it will not take effect until you call
// usart_reconfigure() or after a device reset. 
extern unsigned int setResetDevice(void);
extern unsigned int setReturnToFactory(void);

extern unsigned int getPowerUpSeconds(void);
extern int getPswdStatus(void);
extern int setPswdStatus(unsigned int pswd_status);
extern int verifyPswd(unsigned char*pswd);
extern int setPswd(unsigned char *pswd);
extern int getSerialNum(void);
extern int setSerialNum(int serialNum);
extern unsigned char *getName(void);
extern unsigned int   setName(unsigned char *name);
extern unsigned char *getModel(void);
extern unsigned int   setModel(unsigned char *name);
extern unsigned int getFirmVer(void);
extern unsigned int setFirmVer(int ver);
extern unsigned char *getMac(void);
extern unsigned int   setMac(unsigned char *mac);
extern unsigned int isDHCPEnabled(void);
extern unsigned int setDHCPEnabled(unsigned char enabled);
extern unsigned char *getCfgIP(void);
extern unsigned char *getIfIP(void);
extern unsigned int   setCfgIP(unsigned char *ip);
extern unsigned char *getCfgNetMask(void);
extern unsigned char *getIfNetMask(void);
extern unsigned int   setCfgNetMask(unsigned char *nm);
extern unsigned char *getCfgGateWay(void);
extern unsigned char *getIfGateWay(void);
extern unsigned int   setCfgGateWay(unsigned char *gw);
extern unsigned char *getCfgDNS(void);
extern unsigned char *getIfDNS(void);
extern unsigned int   setCfgDNS(unsigned char *dns);
extern unsigned int getBytesSent(void);
extern unsigned int getBytesRecv(void);
extern unsigned char getWorkingMode(void);
extern unsigned int   setWorkingMode(unsigned char mode);
extern unsigned short getListenPort(void);
extern unsigned int   setListenPort(unsigned short port);
extern unsigned char* getServersInfo(void);
extern unsigned int setServersInfo(unsigned char *p);
extern void getUDPModeInfo(unsigned short *localPort,unsigned short *remotePort,unsigned char **purl);
extern int setUDPModeInfo(unsigned short localPort, unsigned short remotePort,unsigned char *purl);
extern void getUDPMCModeInfo(unsigned short *localPort,unsigned short *remotePort,unsigned char **ip);
extern int setUDPMCModeInfo(unsigned short localPort, unsigned short remotePort,unsigned char *ip);
extern unsigned int getBaudrate(void);
extern unsigned int setBaudrate(unsigned char baudrate);
extern unsigned int getDatabits(void);
extern unsigned int setDatabits(unsigned char databits);
extern unsigned int getStopbits(void);
extern unsigned int setStopbits(unsigned char stopbits);
extern unsigned int getParity(void);
extern unsigned int setParity(unsigned char parity);
extern unsigned int getFlowcontrol(void);
extern unsigned int setFlowcontrol(unsigned char flowcontrol);
extern void getFrameSplit(unsigned short *max_interval,unsigned short *max_datalen);
extern unsigned int setFrameSplit(unsigned short max_interval,unsigned short max_datalen);
// buf format:
// unsigned char conns;
// struct
// {
//    unsigned char ip_addr[4];
//    unsigned char remote_port[2]; (big-endian);
// } connection[ conns ];
// This function is implemented in apptasks.c
// return: buffer length.
extern unsigned int getConnections(unsigned char *buf);

//////////////////////////////


// it's a wrapper of getDNS();
extern struct ip_addr getCfgDNSaddr(void);
extern void usart_reconfigure(void);
extern void settings_powerup_init(void);



#endif //SETTINGS_H

