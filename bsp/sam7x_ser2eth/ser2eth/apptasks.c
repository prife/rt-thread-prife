#include "apptasks.h"
#include <rtthread.h>
#include "cmdproc.h"
#include "settings.h"
#include "protocol.h"
#include "board.h"
#include "led.h"

#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/dhcp.h"
#include "lwip/inet.h"

// return: 0  timeout.
//         >0 length
//         <0 socket error
    int
blocking_lwip_recv(int s, void *mem, size_t len, int timeout) // the len param here is the length to recv.
{
    int readlen, offset = 0;
    fd_set fds;
    int ret;
    struct timeval to;

    to.tv_sec = timeout/1000;
    to.tv_usec = (timeout%1000)*1000;

    while(1)
    {
        FD_ZERO(&fds);
        FD_SET(s,&fds);
        ret = lwip_select(s+1,&fds,0,0,&to);
        if( ret == 0 )
            break;
        else if( ret < 0 )
            return ret;

        readlen = lwip_recvfrom(s, ((char*)mem)+offset, len-offset, 0, NULL, NULL);
        if( readlen == 0 ) // select is ok and readlen == 0 means connection lost.
            return -1;
        else if( readlen < 0 )
            return readlen;
        if( readlen == (len-offset) )
        {
            offset = len;
            break;
        }
        offset += readlen;
    }
    return offset;
}
///////////////////////////////////////////////////////////////////////
// Setup tasks
#define SETUP_UDP_PORT 1983
#define SETUP_TCP_PORT 1982
#define DATA_BUF_SIZE 1500
static unsigned char setup_data_buf[DATA_BUF_SIZE];
static struct rt_mutex setup_data_buf_mutex;

void udp_setup_thread_entry(void *p)
{
    int sock;
    int bytes_read;
    struct sockaddr_in server_addr , client_addr;
    int optval  = 1;
    fd_set readset;

    /* create socket */
    if ((sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        rt_kprintf("Can not create udp setup socket.\n");
        return;
    }

    lwip_setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&optval ,sizeof(optval));

    /* init server socket address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SETUP_UDP_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rt_memset(&(server_addr.sin_zero),0, sizeof(server_addr.sin_zero));

    if (lwip_bind(sock,(struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) == -1)
    {
        rt_kprintf("Bind error\n");
        return;
    }

    rt_kprintf("UDPServer Waiting for client on port %d \n",SETUP_UDP_PORT);

    while (1)
    {
        int bytes_ret;
        rt_uint32_t addr_len = sizeof(struct sockaddr_in);

        FD_ZERO(&readset);
        FD_SET(sock, &readset);
        if( lwip_select(sock+1, &readset, 0, 0, 0) == 0 )
            continue;
        rt_mutex_take(&setup_data_buf_mutex,RT_WAITING_FOREVER);
        bytes_read = lwip_recvfrom(sock, setup_data_buf, DATA_BUF_SIZE, MSG_DONTWAIT,
                (struct sockaddr *)&client_addr, &addr_len);
        rt_mutex_release(&setup_data_buf_mutex);
        if( bytes_read < 0 )
        {
            continue;
        }
        rt_mutex_take(&setup_data_buf_mutex,RT_WAITING_FOREVER);
        setup_data_buf[bytes_read] = 0;
        bytes_ret = processCMD(setup_data_buf,bytes_read);
        rt_mutex_release(&setup_data_buf_mutex);
        if( bytes_ret > 0 )
        {
            // command execute success, send reply in buffer.
            client_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            rt_mutex_take(&setup_data_buf_mutex,RT_WAITING_FOREVER);
            lwip_sendto(sock,setup_data_buf,bytes_ret,0,(struct sockaddr*)&client_addr, sizeof(struct sockaddr));
            rt_mutex_release(&setup_data_buf_mutex);
        }
    }
    // Should not reach here!.
}

void tcp_setup_thread_entry(void *p)
{
    int listenfd;
    struct sockaddr_in saddr;
    fd_set readset;

    listenfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( listenfd == -1 )
    {
        rt_kprintf("TCP setup can not create socket!\n");
        return;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(SETUP_TCP_PORT);

    if( lwip_bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1 )
    {
        lwip_close(listenfd);
        rt_kprintf("TCP setup thread socket bind failed!\n");
        return;
    }

    /* Put socket into listening mode */
    if (lwip_listen(listenfd,2) == -1)
    {
        lwip_close(listenfd);
        rt_kprintf("Listen failed.\n");
        return;
    }

    /* Wait for data or a new connection */
    while(1)
    {
        /* Determine what sockets need to be in readset */
        FD_ZERO(&readset);
        FD_SET(listenfd, &readset);
        // wait forever.
        if( lwip_select(listenfd+1, &readset, 0, 0, 0) == 0 )
            continue;

        if (FD_ISSET(listenfd, &readset))
        {
            int ret,optval = 1;
            struct sockaddr_in addrin;
            u32_t sockaddrLen = sizeof(addrin);
            int socket = lwip_accept(listenfd,(struct sockaddr*)&addrin,&sockaddrLen);
            if( socket < 0 ) // accept failed.
            {
                rt_kprintf("TCP setup accept failed.\n");
                continue;
            }
            rt_kprintf("TCP setup accept connection.\n");
            // set socket keep alive.
            lwip_setsockopt(socket,SOL_SOCKET,SO_KEEPALIVE,&optval ,sizeof(optval));

            // begin to recv & send.
            while(1)
            {
                int dataLen,bytesRet;
                ret = blocking_lwip_recv(socket,setup_data_buf,P2X_HEADER_LEN,500);
                if( ret == 0 )
                    continue;
                if( ret != P2X_HEADER_LEN )
                    break;

                dataLen = P2X_GET_LENGTH(setup_data_buf);
                if( dataLen > 0 )
                {
                    int gotDataLen = blocking_lwip_recv(socket,setup_data_buf+P2X_HEADER_LEN,dataLen,500);
                    if( gotDataLen != dataLen )
                        break;
                }
                bytesRet = processCMD(setup_data_buf,P2X_HEADER_LEN+dataLen);
                if( lwip_send(socket,setup_data_buf,bytesRet,0) < 0 )
                    break;
            }
            rt_kprintf("TCP setup disconnected.\n");
            lwip_close(socket);
        }
    }// while(1) listen.
}




///////////////////////////////////////////////////////////////////////
// tcp io tasks.
#define MAX_LISTEN_SOCK 10
#define SOCKET_BUF_LEN 128
#define SOCKET_LIST_SIZE 12

struct socket_conn
{
    struct rt_mutex mu_sock;
    int used;

    int socket;
    struct sockaddr_in cliaddr;
};

struct socket_conn socket_list[SOCKET_LIST_SIZE];

void socket_conn_init(void)
{
    int i;
    char mutex_name[] = {'m','c','_','A',0};

    memset(socket_list,0,sizeof(socket_list));
    for( i = 0 ; i < SOCKET_LIST_SIZE ; i++ )
    {
        rt_mutex_init(&socket_list[i].mu_sock,mutex_name,RT_IPC_FLAG_FIFO);
        mutex_name[3]++;
    }
}


void net_listen_thread_entry(void *p)
{
    int listenfd;
    struct sockaddr_in saddr;
    fd_set readset;

    // check mode first.
    if( !((getWorkingMode() == TCP_SERVER) || (getWorkingMode() == TCP_AUTO)) )
        return;
    // Acquire our socket for listening for connections
    listenfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( listenfd == -1 )
    {
        rt_kprintf("Can not create socket!\n");
        return;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(getListenPort());     //server port

    if (lwip_bind(listenfd, (struct sockaddr *) &saddr, sizeof(saddr)) == -1)
    {
        lwip_close(listenfd);
        rt_kprintf("Socket bind failed!\n");
        return;
    }

    /* Put socket into listening mode */
    if (lwip_listen(listenfd, MAX_LISTEN_SOCK) == -1)
    {
        lwip_close(listenfd);
        rt_kprintf("Listen failed.\n");
        return;
    }

    /* Wait for data or a new connection */
    while(1)
    {
        /* Determine what sockets need to be in readset */
        FD_ZERO(&readset);
        FD_SET(listenfd, &readset);

        // wait forever.
        if( lwip_select(listenfd+1, &readset, 0, 0, 0) == 0 )
            continue;

        if (FD_ISSET(listenfd, &readset))
        {
            int i;
            socklen_t clilen = sizeof(struct sockaddr_in);

            for( i = 0 ; i < SOCKET_LIST_SIZE ; i++ )
            {
                // we don't block here.
                rt_err_t ret = rt_mutex_take(&(socket_list[i].mu_sock),RT_WAITING_NO);
                if( ret == -RT_ETIMEOUT )
                    continue;

                // we already took the mutex here.
                if( socket_list[i].used )
                {
                    rt_mutex_release(&(socket_list[i].mu_sock));
                    continue;
                }

                // it's an empty slot.
                socket_list[i].socket = lwip_accept(listenfd,
                        (struct sockaddr *)&(socket_list[i].cliaddr),&clilen);
                // if accept failed.
                if( socket_list[i].socket < 0 )
                {
                    rt_kprintf("Accept failed!\n");
                }
                // accept successfully.
                else
                {
                    int optval = 1;
                    // set keepalive.
                    lwip_setsockopt(socket_list[i].socket,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
                    // set used flag.
                    socket_list[i].used = 1;
                    rt_kprintf("Accept connection.\n");
                }
                rt_mutex_release(&(socket_list[i].mu_sock));
                break;
            }

            // check if not enough slot.
            if( i == SOCKET_LIST_SIZE )
            {
                // just accept and disconnect.
                int sock;
                struct sockaddr cliaddr;
                socklen_t clilen;
                sock = lwip_accept(listenfd, &cliaddr, &clilen);
                if (sock >= 0)
                    lwip_close(sock);
                rt_kprintf("Not enough slot, accept and close connection.\n");
            }
        }
        else
        {
            rt_kprintf("ERROR,should not reach here!\n");
        }
    }
}

void net_connect_thread_entry(void *p)
{
    int i,already_connected;
    unsigned char *info = getServersInfo();
    int servNum = RBIGU2(info);

    if( !((getWorkingMode() == TCP_CLIENT) || (getWorkingMode() == TCP_AUTO)) )
        return;
    if( servNum == 0 )
        return;

    // Do some valid check.
    if( servNum > MAX_SERVER_NUMBER )
    {
        rt_kprintf("Server number > %d, internal error!\n",MAX_SERVER_NUMBER);
        return;
    }
    info += 2; // point to first url (if exists).
    for( i = 0; i < servNum; i++ )
    {
        if( strlen((char*)info) >= URL_LEN )
        {
            rt_kprintf("url too long, internal error");
            return;
        }
        info+=(URL_LEN+2);
    }
    // Now, we finished validation.

    info = getServersInfo()+2; // re-point to first URL.
    while(1)
    {
        int i,sock_index;
        unsigned char *purl;
        unsigned short port;
        struct ip_addr ipaddr;
        err_t err;
        for( i = 0; i < servNum; i++ )
        {
            int optval;
            int tmpsock;
            struct sockaddr_in tmpaddr;

            purl = info + i*(URL_LEN+PORT_LEN);
            port = RBIGU2(purl+URL_LEN);
            // check if ip address string such as "1.2.3.4"
            ipaddr.addr = inet_addr((char*)purl);
            if( ipaddr.addr != INADDR_NONE )
                goto CONNECT;
            // it may be an real url.
            err = netconn_gethostbyname((char*)purl,&ipaddr);
            if( err == ERR_OK )
                goto CONNECT;
            // now, we failed to get host by name.
            continue;
CONNECT:
            // first, we should check if we have already connected.
            already_connected = 0;
            for( sock_index = 0 ; sock_index < SOCKET_LIST_SIZE ; sock_index++ )
            {
                // we should wait here.
                rt_mutex_take(&(socket_list[sock_index].mu_sock),RT_WAITING_FOREVER);
                if( (socket_list[sock_index].used) &&
                        (ipaddr.addr == socket_list[sock_index].cliaddr.sin_addr.s_addr) &&
                        (htons(port) == socket_list[sock_index].cliaddr.sin_port) )
                {
                    already_connected = 1;
                    rt_mutex_release(&(socket_list[sock_index].mu_sock));
                    break;
                }
                rt_mutex_release(&(socket_list[sock_index].mu_sock));
            }

            if( already_connected )
            {
                rt_kprintf("Already connected.\n");
                continue;
            }

            // Now, we create a connection first.
            // set keepalive.
            optval = 1;
            tmpsock = lwip_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            if( tmpsock < 0 )
            {
                rt_kprintf("Can not alloc socket!\n");
                continue;
            }
            lwip_setsockopt(tmpsock,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
            tmpaddr.sin_family = AF_INET;
            tmpaddr.sin_port = htons(port);
            tmpaddr.sin_addr.s_addr = ipaddr.addr;
            rt_memset(&(tmpaddr.sin_zero),0,sizeof(tmpaddr.sin_zero));
            if (lwip_connect(tmpsock,(struct sockaddr *)&(tmpaddr),sizeof(tmpaddr)) == -1)
            {
                rt_kprintf("Connect error!\n");
                lwip_close(tmpsock);
                continue;
            }
            // connect successfully.
            rt_kprintf("Connected to server OK\n");

            // now we should find an empty slot to store the socket.
            for( sock_index = 0 ; sock_index < SOCKET_LIST_SIZE ; sock_index++ )
            {
                // we don't wait here.
                rt_err_t ret = rt_mutex_take(&(socket_list[sock_index].mu_sock),RT_WAITING_NO);
                if( ret == -RT_ETIMEOUT )
                    continue;

                // mutex has been taken.
                if( socket_list[sock_index].used )
                {
                    rt_mutex_release(&(socket_list[sock_index].mu_sock));
                    continue;
                }

                // it's an empty slot.
                socket_list[sock_index].socket = tmpsock;
                socket_list[sock_index].cliaddr = tmpaddr;
                socket_list[sock_index].used = 1;
                rt_mutex_release(&(socket_list[sock_index].mu_sock));
                break;
            }
            // can not find any slot?
            if( sock_index == SOCKET_LIST_SIZE )
            {
                rt_kprintf("No space to store socket, close!\n");
                lwip_close(tmpsock);
            }
        } //for
        //sleep for a while.
        rt_thread_delay(RT_TICK_PER_SECOND*5);
    } //while
}

void udp_mode_init_thread_entry(void *p)
{
    int udp_sock;
    int optval = 1;
    struct sockaddr_in server_addr, client_addr;
    unsigned char mode = getWorkingMode();
    unsigned short local_port,remote_port;
    unsigned char *ip;
    struct ip_addr remote_ip;

    if( (mode != UDP) && (mode != UDP_MULTICAST) )
        return;
    if( mode == UDP )
        getUDPModeInfo(&local_port,&remote_port,&ip);
    else
        getUDPMCModeInfo(&local_port,&remote_port,&ip);
    IP4_ADDR(&remote_ip,ip[0],ip[1],ip[2],ip[3]);
    // we use only socket_list[0] to store the udp socket.
    if( (udp_sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 )
    {
        rt_kprintf("Can not create udp I/O socket.\n");
        return;
    }
    lwip_setsockopt(udp_sock,SOL_SOCKET,SO_BROADCAST,&optval ,sizeof(optval));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(local_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    rt_memset(&(server_addr.sin_zero),0, sizeof(server_addr.sin_zero));

    if (lwip_bind(udp_sock,(struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) == -1)
    {
        rt_kprintf("Bind error\n");
        return;
    }

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(remote_port);
    client_addr.sin_addr.s_addr = remote_ip.addr;

    rt_memset(&(client_addr.sin_zero),0, sizeof(server_addr.sin_zero));

    if( mode == UDP_MULTICAST ) //multi cast
    {
        struct ip_mreq mreq;
        int ret;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        mreq.imr_multiaddr.s_addr = client_addr.sin_addr.s_addr;
        ret = lwip_setsockopt(udp_sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq));
        if( ret != ERR_OK )
        {
            rt_kprintf("join group failed\n");
            return;
        }
    }


    // Store socket into socket list.
    rt_mutex_take(&(socket_list[0].mu_sock),RT_WAITING_FOREVER);
    socket_list[0].socket = udp_sock;
    socket_list[0].cliaddr = client_addr;
    socket_list[0].used = 1;
    rt_mutex_release(&(socket_list[0].mu_sock));

    if( mode == UDP )
        rt_kprintf("Working mode: UDP\n");
    else
        rt_kprintf("Working mode: UDP Multicast\n");

}


// This function will be called by tcp/udp setup thread
// so, the mutex must not cause huge delay.
unsigned int getConnections(unsigned char *buf)
{
    int i,offset;

    // we do not use mutex here, in most cases, it will not cause problem.
    // becaues we do not write or use it, just read.
    buf[0] = 0;
    offset = 1;
    for( i = 0 ; i < SOCKET_LIST_SIZE ; i++ )
    {
        if( socket_list[i].used )
        {
            buf[0]++;
            memmove(buf+offset,&(socket_list[i].cliaddr.sin_addr),4);
            offset+=4;
            WBIGU2(buf+offset,htons(socket_list[i].cliaddr.sin_port));
            offset+=2;
        }
    }
    return buf[0]*6+1;
}

////////////////////////////////////////////////////////////////////////
// USART tasks
struct rt_semaphore rx_sem;
struct rt_semaphore tx1_sem,tx2_sem;
rt_device_t dev_uart1,dev_uart2;

unsigned int usart_bytes_sent = 0;
unsigned int usart_bytes_recv = 0;

rt_err_t usart_rx_indicate(rt_device_t dev, rt_size_t size)
{
    while(size--)
    {
        rt_sem_release(&rx_sem);
    }
    return RT_EOK;
}
rt_err_t usart0_tx_complete(rt_device_t dev, void *pbuf)
{
    rt_sem_release(&tx1_sem);
    return RT_EOK;
}
rt_err_t usart1_tx_complete(rt_device_t dev, void *pbuf)
{
    rt_sem_release(&tx2_sem);
    return RT_EOK;
}

#define RX_BUF_SIZE 1024
static char rx_buf[RX_BUF_SIZE];
static short rx_buf_offset = 0;
struct rt_timer max_interval_timer;

void rx_interval_timeout(void* parameter)
{
    rt_sem_release(&rx_sem);
}
void usart_rx_thread_entry(void *p)
{
	unsigned short max_interval,max_datalen;
	unsigned int current_mode;

	getFrameSplit(&max_interval,&max_datalen);
    current_mode = getWorkingMode();

    while( rt_sem_take(&rx_sem,RT_WAITING_FOREVER) == RT_EOK )
    {
        register rt_base_t temp;
        FEED_THE_DOG();

        // if rj45 not connected, we should route data between 232 and 485.
        if( getLinkStatus() == 0 )
        {
            // we split rx_buf to 2 parts in order to save space.
            int len1,len2;
            // Clear the semaphore.
            temp = rt_hw_interrupt_disable();
            rx_sem.value = 0;
            rt_hw_interrupt_enable(temp);
            do
            {
                usart_led_flash();

                // recv usart1
                rt_sem_take(&tx2_sem,RT_WAITING_FOREVER);
                len1 = rt_device_read(dev_uart1,0,rx_buf,RX_BUF_SIZE/2);
                if( len1 )
                    dev_uart2->write(dev_uart2, 0, rx_buf, len1);
                else
                    rt_sem_release(&tx2_sem);

                // recv usart2
                rt_sem_take(&tx1_sem,RT_WAITING_FOREVER);
                len2 = rt_device_read(dev_uart2,0,rx_buf+RX_BUF_SIZE/2,RX_BUF_SIZE/2);
                if( len2 )
                    dev_uart1->write(dev_uart1, 0, rx_buf+RX_BUF_SIZE/2, len2);
                else
                    rt_sem_release(&tx1_sem);
            }while( (len1 != 0) && (len2 != 0) );
            continue;
        }
        else
        {
            // Clear the semaphore.
            temp = rt_hw_interrupt_disable();
            rx_sem.value = 0;
            rt_hw_interrupt_enable(temp);
            while( 1 )
            {
                int len;

                usart_led_flash();

                // read data.
                len = rt_device_read(dev_uart1,0,rx_buf+rx_buf_offset,RX_BUF_SIZE-rx_buf_offset);
                if( len == 0 )
                {
                    len = rt_device_read(dev_uart2,0,rx_buf+rx_buf_offset,RX_BUF_SIZE-rx_buf_offset);
                }
                usart_bytes_recv += len;

                // If buffer is empty and we received data, start the timer.
                if( rx_buf_offset == 0 )
                {
                    if( len == 0 )
                    {
                        break;
                    }
                    else
                    {
                        // if interval less than 10ms, we send data immediately.
                        if( max_interval >= 10 )
                            rt_timer_start(&max_interval_timer);
                    }
                }

                // move offset pointer.
                rx_buf_offset += len;

                // check if we should send data out.
                if( rx_buf_offset < max_datalen &&
                    max_interval_timer.parent.flag & RT_TIMER_FLAG_ACTIVATED )
                {
                    break;
                }

                // Send data out.
                if((current_mode == TCP_SERVER)||
                        (current_mode == TCP_CLIENT)||
                        (current_mode == TCP_AUTO))
                {
                    int i;
                    // send data.
                    for( i = 0 ; i < SOCKET_LIST_SIZE ; i++ )
                    {
                        // we should not use RT_WAITING_FOREVER here.
                        if( rt_mutex_take(&(socket_list[i].mu_sock),10) != RT_EOK )
                            continue;
                        // slot not used.
                        if( socket_list[i].used )
                        {
                            // shall not blocking.
                            if( lwip_send(socket_list[i].socket,rx_buf,rx_buf_offset,0) < 0 )
                            {
                                // connection lost.
                                lwip_close(socket_list[i].socket);
                                socket_list[i].used = 0;
                                rt_kprintf("Connection lost.\n");
                            }
                        }
                        rt_mutex_release(&(socket_list[i].mu_sock));
                    }
                }
                else if( (current_mode == UDP)||
                        (current_mode == UDP_MULTICAST) )
                {
                    if( rt_mutex_take(&(socket_list[0].mu_sock),10) == RT_EOK )
                    {
                        if( socket_list[0].used )
                        {
                            lwip_sendto(socket_list[0].socket,rx_buf,rx_buf_offset,0,
                                    (struct sockaddr*)&(socket_list[0].cliaddr), sizeof(struct sockaddr));
                        }
                        rt_mutex_release(&(socket_list[0].mu_sock));
                    }
                }
                else
                {
                    rt_kprintf("fatal error! rx thread exits.\n");
                    return;
                }
                rx_buf_offset = 0;
            }
        }
    }
    rt_kprintf("Error taking semaphore, usart rx exit!\n");
}

#define TX_BUF_SIZE 1024
static char tx_buf[TX_BUF_SIZE];
void usart_tx_thread_entry(void *p)
{
    fd_set readset;
    struct timeval timeout;
    int fd_max;

    timeout.tv_sec = 0; // second.
    timeout.tv_usec = 100*1000; //wait micro second.


    while( 1 )
    {
        int i;
        // prepare select fd.
        FD_ZERO(&readset);
        fd_max = 0;
        for( i = 0 ; i < SOCKET_LIST_SIZE ; i++ )
        {
            if( socket_list[i].used )
            {
                if( fd_max < socket_list[i].socket )
                    fd_max = socket_list[i].socket;
                FD_SET(socket_list[i].socket,&readset);
            }
        }
        // no connection.
        if( fd_max == 0 )
        {
            rt_thread_delay(1); // delay 10ms.
            continue;
        }

        if( lwip_select(fd_max+1,&readset,NULL,0,&timeout) == 0 )
            continue;

        for( i = 0 ; i < SOCKET_LIST_SIZE ; i++ )
        {
            rt_err_t ret;
            // NOTE: we can take tx semaphore here means that the DMA buffer is not in use.
            // We must NOT write the DMA buffer until notified to be used.
            rt_sem_take(&tx1_sem,RT_WAITING_FOREVER);
            rt_sem_take(&tx2_sem,RT_WAITING_FOREVER);
            // Timeout 100ms.
            ret = rt_mutex_take(&(socket_list[i].mu_sock),10);
            if( ret == -RT_ETIMEOUT )
            {
                rt_kprintf("Taking mu_sock timeout.\n");
                rt_sem_release(&tx2_sem);
                rt_sem_release(&tx1_sem);
                continue;
            }
            if( socket_list[i].used && FD_ISSET(socket_list[i].socket,&readset) )
            {
                unsigned short dataLen = lwip_recv(socket_list[i].socket,tx_buf,TX_BUF_SIZE,MSG_DONTWAIT);
                if( dataLen > 0 )
                {
                    usart_bytes_sent += dataLen;
                    usart_led_flash();
                    dev_uart1->write(dev_uart1,0,tx_buf,dataLen);
                    dev_uart2->write(dev_uart2,0,tx_buf,dataLen);
                    // we have sent data to usart.
                    rt_mutex_release(&(socket_list[i].mu_sock));
                    continue;
                }
                else
                {
                    lwip_close(socket_list[i].socket);
                    socket_list[i].used = 0;
                    rt_kprintf("recv failed or FIN recv, close socket.\n");
                }
            }
            rt_mutex_release(&(socket_list[i].mu_sock));
            rt_sem_release(&tx2_sem);
            rt_sem_release(&tx1_sem);
        }
    }
}

void usart_dev_init(void)
{
    dev_uart1 = rt_device_find("uart1");
    if( dev_uart1 == RT_NULL )
    {
        rt_kprintf("uart1 not found!!\n");
        return;
    }
    dev_uart2 = rt_device_find("uart2");
    if( dev_uart2 == RT_NULL )
    {
        rt_kprintf("uart2 not found!!\n");
        return;
    }
    rt_device_set_rx_indicate(dev_uart1,usart_rx_indicate);
    rt_device_set_rx_indicate(dev_uart2,usart_rx_indicate);
    rt_device_set_tx_complete(dev_uart1,usart0_tx_complete);
    rt_device_set_tx_complete(dev_uart2,usart1_tx_complete);
    rt_device_open(dev_uart1,RT_DEVICE_OFLAG_RDWR);
    rt_device_open(dev_uart2,RT_DEVICE_OFLAG_RDWR);
}

////////////////////////////////////////////////////////////////////////
// lwip init , copied from sys_arch_init.c

static void _lwip_sys_init(void)
{
    struct ip_addr ipaddr, netmask, gw;
    unsigned char *ip;

    ip = getCfgIP();
    IP4_ADDR(&ipaddr,ip[0],ip[1],ip[2],ip[3]);
    ip = getCfgGateWay();
    IP4_ADDR(&gw, ip[0],ip[1],ip[2],ip[3]);
    ip = getCfgNetMask();
    IP4_ADDR(&netmask,ip[0],ip[1],ip[2],ip[3]);

    tcpip_init(RT_NULL, RT_NULL);

    netif_set_addr(netif_default, &ipaddr, &netmask, &gw);
    // We SHOULD define an IP address (udp setup should work any time).
    // So, it may send DHCP packets of src_addr not all zero, tested ok.
    /*
       if( isDHCPEnabled() )
       {
    // when DHCP is enabled, ip/netmask/gateway should be set to all 0.
    IP4_ADDR(&ipaddr,0,0,0,0);
    netif_set_addr(netif_default, &ipaddr, &ipaddr, &ipaddr);
    }
    else
    {
    netif_set_addr(netif_default, &ipaddr, &netmask, &gw);
    }*/
    netif_set_up(netif_default);

    if( isDHCPEnabled() )
        dhcp_start(netif_default);
    else
        dhcp_stop(netif_default);
}

////////////////////////////////////////////////////////////////////////
// Init all ipc objects.
static void ipcs_init(void)
{
	unsigned short dummy,max_interval;
	getFrameSplit(&max_interval,&dummy);
    rt_mutex_init(&setup_data_buf_mutex,"musb",RT_IPC_FLAG_FIFO);
    rt_sem_init(&rx_sem,"semrx",0,RT_IPC_FLAG_FIFO);
    rt_sem_init(&tx1_sem,"semtx0",1,RT_IPC_FLAG_FIFO);
    rt_sem_init(&tx2_sem,"semtx1",1,RT_IPC_FLAG_FIFO);
    rt_timer_init(&max_interval_timer,"rx_tmr",rx_interval_timeout,
        RT_NULL,max_interval/1000*RT_TICK_PER_SECOND,RT_TIMER_FLAG_ONE_SHOT);
}

static void threads_init(void)
{
    rt_thread_t usart_rx_thread;
    rt_thread_t usart_tx_thread;
    rt_thread_t net_listen_thread; // server mode thread.
    rt_thread_t net_connect_thread; // client mode thread.
    rt_thread_t udp_mode_thread; // udp mode thread.
    rt_thread_t udp_setup_thread;
    rt_thread_t tcp_setup_thread;

    // UDP setup thread.
    udp_setup_thread = rt_thread_create("udpset",udp_setup_thread_entry,RT_NULL,1024, 15, 5);
    rt_thread_startup(udp_setup_thread);
    // TCP setup thread.
    tcp_setup_thread = rt_thread_create("tcpset",tcp_setup_thread_entry,RT_NULL,1024, 17, 5);
    rt_thread_startup(tcp_setup_thread);

    // Server mode listen thread.
    net_listen_thread = rt_thread_create("netlisn",net_listen_thread_entry, RT_NULL,1024, 16, 5);
    rt_thread_startup(net_listen_thread);
    // Client mode connect thread.
    net_connect_thread = rt_thread_create("netconn",net_connect_thread_entry, RT_NULL,1024, 16, 5);
    rt_thread_startup(net_connect_thread);
    // Udp mode thread.
    udp_mode_thread = rt_thread_create("udpmode",udp_mode_init_thread_entry,RT_NULL,1024,16,5);
    rt_thread_startup(udp_mode_thread);

    // USART rx thread.  rx is interrupt.
    usart_rx_thread = rt_thread_create("urx",usart_rx_thread_entry, RT_NULL,1024, 3, 5);
    rt_thread_startup(usart_rx_thread);
    // USART tx thread.  tx is dma.
    usart_tx_thread = rt_thread_create("utx",usart_tx_thread_entry, RT_NULL,1024, 3, 5);
    rt_thread_startup(usart_tx_thread);

}

void apps_init(void)
{

    // init command system.
    commandSystemInit();

    // init ipc objects.
    ipcs_init();

    // init socket_conn array.
    socket_conn_init();

    // init settings stored in flash.
    settings_powerup_init();

    // init lwip, modified by me.
    _lwip_sys_init();

    // init serial devices.
    usart_dev_init();

    // check and start all threads.
    threads_init();

    // init idle hook functions.
    rt_thread_idle_sethook(idle_hook_function);

}


