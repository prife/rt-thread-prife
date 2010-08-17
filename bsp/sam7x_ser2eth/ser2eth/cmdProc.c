#include "cmdProc.h"
#include "settings.h"
#include <string.h>
#include "protocol.h"

#define MAX_SUPPORT_PC 3

struct client_pc_t
{
	unsigned int pc_id;
	char	verified;
	char	inuse;
};

struct client_pc_t client_pc[MAX_SUPPORT_PC];

static int is_verified(unsigned int pcid)
{
	int i;
	for( i=0; i<MAX_SUPPORT_PC; i++ )
		if( client_pc[i].inuse && client_pc[i].pc_id == pcid && client_pc[i].verified)
			return 1;
	return 0;
}
static int check_has_pcid(unsigned int pcid)
{
	int i;
	for( i=0; i<MAX_SUPPORT_PC; i++ )
		if( client_pc[i].inuse && client_pc[i].pc_id == pcid )
			return 1;
	return 0;
}
static void set_verified(unsigned int pcid,int verified)
{
	int i;
	for( i=0; i<MAX_SUPPORT_PC; i++ )
	{
		if( client_pc[i].inuse && client_pc[i].pc_id == pcid)
		{
			client_pc[i].verified = verified;
			return;
		}
	}
}

static void check_add_pcid(unsigned int pcid)
{
	int i;

	if( check_has_pcid(pcid) )
		return;
	// not found, we find a unused slot and add.
	for( i=0; i<MAX_SUPPORT_PC; i++ )
	{
		if( !client_pc[i].inuse )
		{
			client_pc[i].inuse = 1;
			client_pc[i].pc_id = pcid;
			if( getPswdStatus() == PSWD_DISABLED )
				client_pc[i].verified = 1;
			else
				client_pc[i].verified = 0;
			return;
		}
	}
	// if reachs here, all slot are used, find a random slot and insert.
	// (we use pcid here, because pcid should be a random number.
	client_pc[pcid%MAX_SUPPORT_PC].pc_id = pcid;
	if( getPswdStatus() == PSWD_DISABLED )
		client_pc[pcid%MAX_SUPPORT_PC].verified = 1;
	else
		client_pc[pcid%MAX_SUPPORT_PC].verified = 0;
	// inuse must be 1
	return;
}

static int processEnum(unsigned char *buf)
{
	int mySerialNum = getSerialNum();
	unsigned char *pid = P2X_DATA_PTR(buf);
	unsigned short len = P2X_GET_LENGTH(buf)/4;
	unsigned int pcid = P2X_GET_PCID(buf);

	check_add_pcid(pcid);
	// x3k_id ignored.
	// tag ignored.
	while( len-- )
	{
		// ID is network-endian.
		if( mySerialNum == RBIGU4(pid) ) // My id found, mute.
			return 0;
		pid+=4;
	}
	// Not found, send my id.
	X2P_SET_MAGIC(buf);
	X2P_SET_TYPE(buf,CMD_ENUM);
	X2P_SET_PCID(buf,pcid);
	X2P_SET_X3KID(buf,mySerialNum);
	X2P_SET_RESULT(buf,RESULT_OK);
	X2P_SET_TAG(buf,TAG_NULL);
	X2P_SET_LENGTH(buf,0);
	return 1;
}

static int processVerifyPswd(unsigned char *buf)
{
	unsigned int pcid = P2X_GET_PCID(buf);
	int mySerialNum = getSerialNum();

    // this package not mine.
	if( mySerialNum != P2X_GET_X3KID(buf) )
		return 0;
	
	check_add_pcid(pcid);
	
	if( verifyPswd(P2X_DATA_PTR(buf)) )
	{
		// verify successful.
		set_verified(pcid,1);
		X2P_SET_RESULT(buf,RESULT_OK);
	}
	else
	{
		// verify failed.
		//set_verified(pcid,0);
		X2P_SET_RESULT(buf,RESULT_ERROR);
	}
	X2P_SET_MAGIC(buf);
	X2P_SET_TYPE(buf,CMD_VERIFY_PSWD);
	X2P_SET_PCID(buf,pcid);
	X2P_SET_X3KID(buf,mySerialNum);
	//X2P_SET_RESULT(buf,RESULT_OK);
	X2P_SET_TAG(buf,TAG_NULL);
	X2P_SET_LENGTH(buf,0);
	return 1;
}

static int processGet(unsigned char *buf)
{
    unsigned char *pdata;
    unsigned char tag;
	unsigned int pcid = P2X_GET_PCID(buf);
	int mySerialNum = getSerialNum();
    // this package not mine.
	if( mySerialNum != P2X_GET_X3KID(buf) )
		return 0;
		
	check_add_pcid(pcid);

    pdata = X2P_DATA_PTR(buf);
    tag = P2X_GET_TAG(buf);
    switch ( tag )
    {
        //case TAG_SERIALNUM:
        // serial_number is included in enum ack.
		case TAG_POWERUPTIME:
			{
				unsigned int t_sec = getPowerUpSeconds();
				WBIGU4(pdata,t_sec);
				X2P_SET_LENGTH(buf,4);
				break;
			}
		case TAG_MODEL:
			{
				strcpy((char*)pdata,(char*)getModel());
                X2P_SET_LENGTH(buf,strlen((char*)pdata)+1);
                break;
			}
        case TAG_NAME:
            {
                strcpy((char*)pdata,(char*)getName());
                X2P_SET_LENGTH(buf,strlen((char*)pdata)+1);
                break;
            }
        case TAG_FIRMVER:
            {
                unsigned int ver = getFirmVer();
				WBIGU4(pdata,ver);
                X2P_SET_LENGTH(buf,4);
                break;
            }
		case TAG_PSWD_STATUS:
			{
				pdata[0] = getPswdStatus();
				X2P_SET_LENGTH(buf,1);
				break;
			}
        //case TAG_PASSWD:
        // password can not be read.
        case TAG_MAC:
            {
                memmove(pdata,getMac(),6);
                X2P_SET_LENGTH(buf,6);
                break;
            }
        case TAG_DHCP:
            {
                pdata[0] = isDHCPEnabled();
                X2P_SET_LENGTH(buf,1);
                break;
            }
        case TAG_IP:
            {
                memmove(pdata,getIfIP(),4);
                X2P_SET_LENGTH(buf,4);
                break;
            }
        case TAG_NETMASK:
            {
                memmove(pdata,getIfNetMask(),4);
                X2P_SET_LENGTH(buf,4);
                break;
            }
        case TAG_GATEWAY:
            {
                memmove(pdata,getIfGateWay(),4);
                X2P_SET_LENGTH(buf,4);
                break;
            }
        case TAG_DNS:
            {
                memmove(pdata,getIfDNS(),4);
                X2P_SET_LENGTH(buf,4);
                break;
            }
        case TAG_WMODE:
            {
                pdata[0] = getWorkingMode();
                X2P_SET_LENGTH(buf,1);
                break;
            }
        case TAG_LPORT:
            {
                unsigned short port = getListenPort();
				WBIGU2(pdata,port);
                X2P_SET_LENGTH(buf,2);
                break;
            }
        case TAG_SERVINFO:
            {
                unsigned char *p = getServersInfo();
				if( RBIGU2(p) == 0 )
				{
					X2P_SET_LENGTH(buf,0);
				}
				else
				{
					int length = SERVER_INFO_LEN(p);
                	X2P_SET_LENGTH(buf,length);
					memmove(pdata,p,length);
				}
                break;
            }
		case TAG_UDP:
		case TAG_UDPMC:
			{
				unsigned short local_port,remote_port;
				unsigned char *ip;
				if( tag == TAG_UDP )
					getUDPModeInfo(&local_port,&remote_port,&ip);
				else
					getUDPMCModeInfo(&local_port,&remote_port,&ip);
				WBIGU2(pdata,local_port);
				WBIGU2(pdata+2,remote_port);
				memmove(pdata+4,ip,4);
				X2P_SET_LENGTH(buf,8);
				break;
			}
		case TAG_CONNS:
			{
				unsigned int len = getConnections(pdata);
				X2P_SET_LENGTH(buf,len);
				break;
			}
        case TAG_BAUDRATE:
            {
                pdata[0] = getBaudrate();
                X2P_SET_LENGTH(buf,1);
                break;
            }
        case TAG_DATABITS:
            {
                pdata[0] = getDatabits();
                X2P_SET_LENGTH(buf,1);
                break;
            }
        case TAG_STOPBITS:
            {
                pdata[0] = getStopbits();
                X2P_SET_LENGTH(buf,1);
                break;
            }
        case TAG_PARITY:
            {
                pdata[0] = getParity();
                X2P_SET_LENGTH(buf,1);
                break;
            }
		case TAG_FLOWCONTROL:
			{
				pdata[0] = getFlowcontrol();
				X2P_SET_LENGTH(buf,1);
				break;
			}
		case TAG_BYTESSENT:
			{
				unsigned int bytes = getBytesSent();
				WBIGU4(pdata,bytes);
                X2P_SET_LENGTH(buf,4);
                break;
			}
		case TAG_BYTESRECVED:
			{
				unsigned int bytes = getBytesRecv();
				WBIGU4(pdata,bytes);
                X2P_SET_LENGTH(buf,4);
                break;
			}
		case TAG_FRAMESPLIT:
			{
				unsigned short max_interval,max_datalen;
				getFrameSplit(&max_interval,&max_datalen);
				WBIGU2(pdata,max_interval);
				WBIGU2(pdata+2,max_datalen);
				X2P_SET_LENGTH(buf,4);
				break;
			}
        default: //TAG not support.
            return 0;
    }
    X2P_SET_MAGIC(buf);
    X2P_SET_TYPE(buf,CMD_GET);
    X2P_SET_PCID(buf,pcid);
    X2P_SET_X3KID(buf,mySerialNum);
    X2P_SET_TAG(buf,tag);
    X2P_SET_RESULT(buf,RESULT_OK);
	return 1;
}

static int processSet(unsigned char *buf)
{
    int result;
    unsigned char *pdata;
    unsigned char tag;
    unsigned int len;
	unsigned int pcid = P2X_GET_PCID(buf);
	int mySerialNum = getSerialNum();
    // this package not mine.
	if( mySerialNum != P2X_GET_X3KID(buf) )
		return 0;

    tag = P2X_GET_TAG(buf);
    pdata = P2X_DATA_PTR(buf);
    len = P2X_GET_LENGTH(buf);

    // password verification is required for "SET" commands.
    if( (getPswdStatus()==PSWD_ENABLED) && !is_verified(pcid) )
    {
        X2P_SET_RESULT(buf,RESULT_PASSWORD_REQUIRED);
        goto fill_header;
    }

    switch ( P2X_GET_TAG(buf) )
    {
		case TAG_RESET:
			{
				setResetDevice();
				// Should not reach here!!!
				result = 0;
				break;
			}
		case TAG_RETURNFACTORY:
			{
				result = setReturnToFactory();
				break;
			}
		case TAG_MODEL:
			{
				result = setModel(pdata);
				break;
			}
        case TAG_SERIALNUM: // internal command.
            {
				result = setSerialNum(RBIGU4(pdata));
                break;
            }
        case TAG_NAME:
            {
                result = setName(pdata);
                break;
            }
        case TAG_FIRMVER: // internal command.
            {
                result = setFirmVer(RBIGU4(pdata));
                break;
            }
		case TAG_PSWD_STATUS:
			{
				result = setPswdStatus(*pdata);
				break;
			}
        case TAG_PASSWD:
            {
                result = setPswd(pdata);
                break;
            }
        case TAG_MAC:
            {
                result = setMac(pdata);
                break;
            }
        case TAG_DHCP:
            {
                result = setDHCPEnabled(*pdata);
                break;
            }
        case TAG_IP:
            {
                result = setCfgIP(pdata);
                break;
            }
        case TAG_NETMASK:
            {
                result = setCfgNetMask(pdata);
                break;
            }
        case TAG_GATEWAY:
            {
                result = setCfgGateWay(pdata);
                break;
            }
        case TAG_DNS:
            {
                result = setCfgDNS(pdata);
                break;
            }
        case TAG_WMODE:
            {
                result = setWorkingMode(*pdata);
                break;
            }
        case TAG_LPORT:
            {
                result = setListenPort(RBIGU2(pdata));
                break;
            }
        case TAG_SERVINFO:
            {
				if( len == 0 )
					result = setServersInfo(NULL);
				else
                	result = setServersInfo(pdata);
                break;
            }
		case TAG_UDP:
			{
				result = setUDPModeInfo(RBIGU2(pdata),RBIGU2(pdata+2),pdata+4);
				break;
			}
		case TAG_UDPMC:
			{
				result = setUDPMCModeInfo(RBIGU2(pdata),RBIGU2(pdata+2),pdata+4);
				break;
			}
        case TAG_BAUDRATE:
            {
                result = setBaudrate(*pdata);
                break;
            }
        case TAG_DATABITS:
            {
                result = setDatabits(*pdata);
                break;
            }
        case TAG_STOPBITS:
            {
                result = setStopbits(*pdata);
                break;
            }
        case TAG_PARITY:
            {
                result = setParity(*pdata);
                break;
            }
		case TAG_FLOWCONTROL:
			{
				result = setFlowcontrol(*pdata);
				break;
			}
		case TAG_FRAMESPLIT:
			{
				result = setFrameSplit(RBIGU2(pdata),RBIGU2(pdata+2));
				break;
			}
        default: // Should not reach here
            return 0;
    }
    if( result )
        X2P_SET_RESULT(buf,RESULT_OK);
    else
        X2P_SET_RESULT(buf,RESULT_ERROR);
fill_header:
    X2P_SET_MAGIC(buf);
    X2P_SET_TYPE(buf,CMD_SET);
    X2P_SET_PCID(buf,pcid);
    X2P_SET_X3KID(buf,mySerialNum);
    X2P_SET_TAG(buf,tag);
    X2P_SET_LENGTH(buf,0);
	return 1;
}

// return value is the data length should be send. -1 indicates error status.
int processCMD(unsigned char *buf,int buflen)
{
	// check buflen.
	if( buflen < P2X_HEADER_LEN || buflen != (P2X_HEADER_LEN+P2X_GET_LENGTH(buf)) )
		return -1;
	// check magic number
	if( P2X_GET_MAGIC(buf) != MAGIC_P2X )
		return -1;
	// process command.
	switch( P2X_GET_TYPE(buf) )
	{
		case CMD_ENUM:
			if( !processEnum(buf) )
				return -1;
			break;
		case CMD_VERIFY_PSWD:
			if( !processVerifyPswd(buf) )
				return -1;
			break;
		case CMD_GET:
			if( !processGet(buf) )
				return -1;
			break;
		case CMD_SET:
			if( !processSet(buf) )
				return -1;
			break;
		default: // tag error!
			return -1;
	}
	return X2P_HEADER_LEN+X2P_GET_LENGTH(buf);
}

void commandSystemInit(void)
{
	memset(client_pc,0,sizeof(client_pc));
}




