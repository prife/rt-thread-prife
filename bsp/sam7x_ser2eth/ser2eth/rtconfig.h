/* RT-Thread config file */
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

//#define CONFIG_DEBUG
// Defined in target configuration.

/* RT_NAME_MAX*/
#define RT_NAME_MAX	8

/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE	4

/* PRIORITY_MAX*/
#define RT_THREAD_PRIORITY_MAX	32

/* Tick per Second*/
#define RT_TICK_PER_SECOND	100

/* SECTION: RT_DEBUG */
/* Thread Debug*/
#ifdef CONFIG_DEBUG
#define RT_THREAD_DEBUG
#define RT_DEBUG
#endif

/* Using Hook*/
#define RT_USING_HOOK

/* SECTION: IPC */
/* Using Semaphore*/
#define RT_USING_SEMAPHORE

/* Using Mutex*/
#define RT_USING_MUTEX

/* Using Event*/
#define RT_USING_EVENT

/* Using Faset Event*/
/* #define RT_USING_FASTEVENT */

/* Using MailBox*/
#define RT_USING_MAILBOX

/* Using Message Queue*/
#define RT_USING_MESSAGEQUEUE

/* SECTION: Memory Management */
/* Using Memory Pool Management*/
//#define RT_USING_MEMPOOL

/* Using Dynamic Heap Management*/
#define RT_USING_HEAP

/* Using Small MM*/
#define RT_USING_SMALL_MEM
//#define RT_MEM_DEBUG

/* Using SLAB Allocator*/
/* #define RT_USING_SLAB */

/* SECTION: Device System */
/* Using Device System*/
#define RT_USING_DEVICE
#define RT_USING_UART1
#define RT_USING_UART2
#ifdef CONFIG_DEBUG
#define RT_USING_DBGU
#endif
#define RT_UART_RX_BUFFER_SIZE	2048
//#define RT_UART_TX_BUFFER_SIZE	128

/* SECTION: Console options */
/* the buffer size of console*/
#define RT_CONSOLEBUF_SIZE	128

/* SECTION: FinSH shell options */
/* Using FinSH as Shell*/
#ifdef CONFIG_DEBUG
#define RT_USING_FINSH
#endif
/* use symbol table */
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION

/* SECTION: a mini libc */
/* Using mini libc library*/
/* #define RT_USING_MINILIBC */

/* SECTION: C++ support */
/* Using C++ support*/
/* #define RT_USING_CPLUSPLUS */

/* SECTION: lwip, a lighwight TCP/IP protocol stack */
/* Using lighweight TCP/IP protocol stack*/
#define RT_USING_LWIP
#define RT_LWIP_ETHTHREAD_PRIORITY 5
#define RT_LWIP_ETHTHREAD_MBOX_SIZE 48
#define RT_LWIP_ETHTHREAD_STACKSIZE 1024

#define RT_LWIP_TCPTHREAD_PRIORITY 6
#define RT_LWIP_TCPTHREAD_MBOX_SIZE 8
#define RT_LWIP_TCPTHREAD_STACKSIZE 4096

/* Trace LwIP protocol*/
#ifdef CONFIG_DEBUG
#define RT_LWIP_DEBUG
#endif

/* Enable ICMP protocol*/
#define RT_LWIP_ICMP

/* Enable IGMP protocol*/
#define RT_LWIP_IGMP

/* Enable UDP protocol*/
#define RT_LWIP_UDP

/* Enable TCP protocol*/
#define RT_LWIP_TCP

/* the number of simulatenously active TCP connections*/
#define RT_LWIP_TCP_PCB_NUM	13

/* TCP sender buffer space*/
#define RT_LWIP_TCP_MSS 100
#define RT_LWIP_TCP_SND_BUF	400
#define RT_LWIP_TCP_WND RT_LWIP_TCP_MSS
//#define MEMP_NUM_TCP_SEG 32
/* Enable SNMP protocol*/
/* #define RT_LWIP_SNMP */

/* Using DHCP*/
#define RT_LWIP_DHCP
#define RT_LWIP_DNS
extern struct ip_addr getCfgDNSaddr(void);
#define DNS_SERVER_ADDRESS      (getCfgDNSaddr().addr)
#define DNS_MAX_NAME_LENGTH		128
#define DNS_MAX_SERVERS			1

/* SECTION: DFS options */
//#define RT_USING_DFS
/* the max number of mounted filesystem */
#define DFS_FILESYSTEMS_MAX			1
/* the max number of opened files 		*/
#define DFS_FD_MAX					2
/* the max number of cached sector 		*/
#define DFS_CACHE_MAX_NUM   		4

#define RT_LWIP_USING_RT_MEM

// tcp keep alive.
#define LWIP_TCP_KEEPALIVE			1
#define TCP_KEEPIDLE_DEFAULT		3000UL
#define TCP_KEEPINTVL_DEFAULT		1000UL
#define TCP_KEEPCNT_DEFAULT			3U

#define RT_DEFAULT_TCP_RECVMBOX_SIZE 100

#endif
