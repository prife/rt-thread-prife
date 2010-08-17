#ifndef PROTOCOL_H
#define PROTOCOL_H

// NOTE:  P2X_xxx means  packet from pc to x3k.
//        X2P_xxx means packet from x3k to pc.

// Tag definition.
enum
{
// TAG_NULL only used in enum and verify pswd command.
    TAG_NULL=0,

// Command below are only used in get or set command.
	TAG_RESET,
	TAG_RETURNFACTORY,
	TAG_POWERUPTIME,

	TAG_MODEL,
    TAG_SERIALNUM,
    TAG_NAME,
    TAG_FIRMVER,
	TAG_PSWD_STATUS,
    TAG_PASSWD,

    TAG_MAC,
    TAG_DHCP,
    TAG_IP,
    TAG_NETMASK,
    TAG_GATEWAY,
    TAG_DNS,

    TAG_WMODE,
    TAG_LPORT,
    TAG_SERVINFO,
	TAG_UDP,
	TAG_UDPMC,
	TAG_CONNS,

    TAG_BAUDRATE,
    TAG_DATABITS,
    TAG_STOPBITS,
    TAG_PARITY,
	TAG_FLOWCONTROL,

	TAG_BYTESSENT,
	TAG_BYTESRECVED,
	
	TAG_FRAMESPLIT,
};

// Command type definition.
enum
{
    CMD_VERIFY_PSWD = 1,
    CMD_GET,
    CMD_SET,
    CMD_ENUM,
};

// Return value definition.
enum
{
    RESULT_OK = 1,
    RESULT_ERROR,
    RESULT_PASSWORD_REQUIRED,
};

/////////////////////////////////////////////////
// Network endian (big-endian) read-write function.
#define RBIGU2(p)       ((p)[0]<<8|(p)[1])
#define WBIGU2(p,s)     do{(p)[0]=((s>>8)&0xffu); (p)[1]=(s&0xffu);}while(0)
#define RBIGU4(p)       ((p)[0]<<24|(p)[1]<<16|(p)[2]<<8|(p)[3])
#define WBIGU4(p,i)     do{(p)[0]=((i>>24)&0xffu);(p)[1]=((i>>16)&0xffu);(p)[2]=((i>>8)&0xffu);(p)[3]=(i&0xffu);}while(0)

/////////////////////////////////////////////////
#define MAGIC_P2X       (0x1698)

#define P2X_HEADER_LEN          (14)
#define P2X_DATA_PTR(buf)       ((unsigned char*)buf+P2X_HEADER_LEN)
// Get
#define P2X_GET_MAGIC(buf)	    RBIGU2(buf+0)
#define P2X_GET_TYPE(buf)       (buf[2])
#define P2X_GET_PCID(buf)       RBIGU4(buf+3)
#define P2X_GET_X3KID(buf)      RBIGU4(buf+7)
#define P2X_GET_TAG(buf)        (buf[11])
#define P2X_GET_LENGTH(buf)     RBIGU2(buf+12)
// Set
#define P2X_SET_MAGIC(buf)      WBIGU2(buf+0,MAGIC_P2X)
#define P2X_SET_TYPE(buf,type)  (buf[2]=(type&0xffu))
#define P2X_SET_PCID(buf,id)    WBIGU4(buf+3,id)
#define P2X_SET_X3KID(buf,id)   WBIGU4(buf+7,id)
#define P2X_SET_TAG(buf,tag)    (buf[11]=(tag&0xffu))
#define P2X_SET_LENGTH(buf,len) WBIGU2(buf+12,len)

/*
 * UDP PC to X3000 command format:
 * [magic 2B] Magic number indicates this is a UDP command.
 * [cmd type 1B]
 * [pc_id 4B]
 * [x3k_id 4B]
 * [cmd tag 1B]
 * [cmd length 2B] length of the rest of data.
 * [cmd content] content of command.
 */
/////////////////////////////////////////////////
#define MAGIC_X2P (0x1799)

#define X2P_HEADER_LEN          (15)
#define X2P_DATA_PTR(buf)       ((unsigned char*)buf+X2P_HEADER_LEN)
// Get
#define X2P_GET_MAGIC(buf)      RBIGU2(buf+0)
#define X2P_GET_TYPE(buf)       (buf[2])
#define X2P_GET_PCID(buf)       RBIGU4(buf+3)
#define X2P_GET_X3KID(buf)      RBIGU4(buf+7)
#define X2P_GET_RESULT(buf)     (buf[11])
#define X2P_GET_TAG(buf)        (buf[12])
#define X2P_GET_LENGTH(buf)     RBIGU2(buf+13)
// Set
#define X2P_SET_MAGIC(buf)      WBIGU2(buf+0,MAGIC_X2P)
#define X2P_SET_TYPE(buf,type)  (buf[2]=(type&0xffu))
#define X2P_SET_PCID(buf,id)    WBIGU4(buf+3,id)
#define X2P_SET_X3KID(buf,id)   WBIGU4(buf+7,id)
#define X2P_SET_RESULT(buf,res) (buf[11]=(res&0xffu))
#define X2P_SET_TAG(buf,tag)    (buf[12]=(tag&0xffu))
#define X2P_SET_LENGTH(buf,len) WBIGU2(buf+13,len)


/*
 * UDP X3000 to PC command format:
 * [magic 2B]
 * [type 1B]
 * [pc_id 4B]
 * [x3k_id 4B]
 * [cmd result 1B]
 * [cmd tag 1B]
 * [cmd length 2B]
 * [cmd content]
 */

/*
 * Protocol:
 *
 * <== Enum command:
 *      magic
 *      type == CMD_ENUM
 *      pc_id == PC_ID
 *      x3k_id  (ignored).
 *      tag  (ignored).
 *      length == (number of enumed id)*sizeof(x3k_id)
 *      content == enumed ids.
 * ==> Enum return:
 *      magic
 *      type == CMD_ENUM
 *      pc_id == PC_ID
 *      x3k_id == serial_number
 *      result == RESULT_OK
 *      tag == TAG_NULL
 *      length == 0
 *      content ignored.
 *
 * <== Get command:
 *      magic
 *      type == CMD_GET
 *      pc_id == PC_ID
 *      x3k_id == (get from enum cmd).
 *      tag == (all tag support).
 *      length == 0 (ignored)
 *      content == null (ignored).
 * ==> Get return:
 *      magic
 *      type == CMD_GET
 *      pc_id == PC_ID
 *      x3k_id == serial_number
 *      result == RESULT_OK/RESULT_ERROR
 *      tag
 *      length
 *      content
 *
 * <== Verify password command:
 *      magic
 *      type == CMD_VERIFY_PSWD
 *      pc_id == PC_ID
 *      x3k_id == (get from enum cmd).
 *      tag == TAG_NULL (ignored).
 *      length == password length.
 *      content == password.
 * ==> Verify password return:
 *      magic
 *      type == CMD_VERIFY_PSWD
 *      pc_id == PC_ID
 *      x3k_id == serial_number
 *      result == RESULT_OK/RESULT_ERROR
 *      tag == TAG_NULL
 *      length == 0 (ignored).
 *      content == null (ignored).
 *
 * <== Set command:
 *      magic
 *      type == CMD_SET
 *      pc_id == PC_ID
 *      x3k_id == (get from enum cmd).
 *      tag
 *      length
 *      content
 * ==> Set return:
 *      magic
 *      type == CMD_SET
 *      pc_id == PC_ID
 *      x3k_id == serial_number
 *      result == RESULT_OK/RESULT_ERROR
 *      tag == TAG_NULL
 *      length == 0 (ignored).
 *      value == null (ignored).
 */


#endif //PROTOCOL_H


