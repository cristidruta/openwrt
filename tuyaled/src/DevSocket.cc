#include "DevSocket.h"


/* 构造函数： 
 *      create udp socket
 *      指定端口 */
DevSocket::DevSocket(int port)
{
    bytes_send = 0;
    bytes_read = 0;
    m_bstate_connect = false;
    socket_error=SRV_SOCKET_NO_ERROR;
    address_family=AF_INET;
    port_number=port;
    socket_type=SOCK_DGRAM;
    protocol_family=IPPROTO_UDP;
    sockaddr_sin.sin_family=address_family;
    sockaddr_sin.sin_addr.s_addr=INADDR_ANY;
    sockaddr_sin.sin_port=htons(port_number);
    srv_socket=socket(address_family, socket_type, protocol_family);
}

/* 服务端套接字绑定监听函数
 * 返回值:  -1：失败
 *          0 ：成功 */
unsigned long DevSocket::bindserver_listen(int maxconn_num)
{
    if(bind(srv_socket,(struct sockaddr*)&sockaddr_sin, sizeof(struct sockaddr*)) == -1)
    {
        socket_error=SRV_SOCKET_BIND_ERROR;
        return -1;
    }

    if(listen(srv_socket,maxconn_num) == -1)
    {
        socket_error=SRV_SOCKET_LISTEN_ERROR;
        return -1;
    }
    return 0;
}

/* udp发送函数 */
int DevSocket::udpSendTo(void *buf, int bytes, int flags)
{
    int addr_size = (int)sizeof(sockaddr_in);
    bytes_send = 0;
    int num_moved = 0;
    int num_req = (int)bytes;
    char *pSendbuffer = (char *)buf;
    while (bytes_moved < bytes)
    {
        if ((num_moved = sendto(srv_socket,
                               pSendbuffer, 
                               num_req-bytes_moved, 
                               flags, 
                               (const struct sockaddr *)sockaddr_sin, 
                               addr_size)) > 0)
        {
            bytes_send += num_moved;
            pSendbuffer += num_moved;
        }

        if (num_moved < 0)
        {
            socket_error = SRV_SOCKET_TRANSMIT_ERROR;
            return -1;
        }
    }
    return bytes_send;
}

/* 服务端UDP接收函数 */
int DevSocket::udprecvfrom(void *buf, int bytes, int seconds, int useconds,int flags)
{
    socklen_t addr_size = (socklen_t)sizeof(sockaddr_in);
    bytes_read = 0;
    int num_read = 0;
    int num_req = (int)bytes;
    char *preceivebuffer = (char *)buf;
    while(bytes_read < bytes) {
        if(!ReadSelect(srv_socket, seconds, useconds)) {
            socket_error = SRV_SOCKET_REQUEST_TIMEOUT;
            return -1;
        }
        if((num_read = recvfrom(srv_socket, preceivebuffer, num_req - bytes_read, flags
                        , (struct sockaddr *)remote_addrsin, &addr_size)) > 0) {
            bytes_read += num_read;
            preceivebuffer += num_read;
        }
        if(num_read < 0) {
            socket_error = SRV_SOCKET_RECEIVE_ERROR;
            return -1;
        }
    }
    return bytes_read;
}

#if 0
const unsigned int srv_socket_DEFAULT_PORT=4096;
DevSocket::DevSocket()
{
    m_bstate_connect = false;
    address_family=AF_INET;
    socket_type=SOCK_STREAM;
    protocol_family=IPPROTO_TCP;
    port_number = srv_socket_DEFAULT_PORT;
    srv_socket = -1;

    socket_error = SRV_SOCKET_NO_ERROR;
    bytes_send = 0;
    bytes_read  = 0;
}

/* 构造函数：
 *      指定端口，套接字类型，端口名,创建套接字 */
DevSocket::DevSocket(int port, int stype, char* strhostname)
{
    bytes_send = 0;
    bytes_read = 0;
    m_bstate_connect = false;
    socket_error=SRV_SOCKET_NO_ERROR;

    address_family=AF_INET;
    port_number=port;
    if (stype == SOCK_STREAM)          //判断套接字的类型
    {
        socket_type=SOCK_STREAM;
        protocol_family=IPPROTO_TCP;
    }
    else if(st==SOCK_DGRAM)   
    {
        socket_type=SOCK_DGRAM;
        protocol_family=IPPROTO_UDP;
    }
    else
    {
        socket_error = SRV_SOCKET_SOCKETTYPE_ERROR;
        return -1;
    }
    sockaddr_sin.sin_family=address_family;
    if (strhostname)
    {
        hostent *hostentnm = gethostbyname(strhostname);
        if (hostentnm==(struct hostent *)0){
            socket_error=SRV_SOCKET_HOSTNAME_ERROR;
            return -1;
        }
        sockaddr_sin.sin_addr.s_addr=*((unsigned long*) hostentnm->h_addr);
        else
            sockaddr_sin.sin_addr.s_addr=INADDR_ANY;
        sockaddr_sin.sin_port=htons(port_number);
        srv_socket=socket(address_family, socket_type, protocol_family);

    }
}

/* 构造函数：
 *      指定协议家族， 地址家族，端口，套接字类型，端口名，创建套接字 */
DevSocket::DevSocket(sa_family_t af,int stype,int pro_family,
        int port,char *strhostname)
{
    bytes_send = 0;
    bytes_read  = 0;
    address_family = af;
    socket_type = stype;
    protocol_family = pro_family;
    port_number = port;
    sockaddr_sin.sin_family = address_family;
    socket_error = SRV_SOCKET_NO_ERROR;
    m_bstate_connect = false;
    if (strhostname) {
        struct hostent *hostentnm = gethostbyname(strhostname);
        if(hostentnm==(struct hostent *) 0) {
            socket_error=SRV_SOCKET_HOSTNAME_ERROR;
            return -1;
        }
        sockaddr_sin.sin_addr.s_addr=*((unsigned long*) hostnm->h_addr);
    }
    else
        sockaddr_sin.sin_addr.s_addr=INADDR_ANY;
    sockaddr_sin.sin_port=htons(port_number);
    srv_socket=socket(address_family, socket_type, protocol_family);
}

/* 创建初始化套接字函数，创建套接字 */
int DevSocket::socket_create(void)
{
    srv_socket = socket(address_family, socket_type, protocol_family);
    if (srv_socket<0)
    {
        socket_error=SRV_SOCKET_INIT_ERROR;
        return -1;
    }
    return srv_socket;

}

/* 客户端套接字连接函数 */
unsigned int DevSocket::sock_connect()
{
    if(connect(srv_socket,(struct sockaddr*)&sin,sizeof(sin)) == -1)
    {
        socket_error=SRV_SOCKET_CONNECT_ERROR;
        m_bstate_connect = false;
        return -1;
    }
    else
    {
        m_bstate_connect = true;
        return 1;
    }
}

/* 服务端接受函数 */
unsigned int DevSocket::sock_accept()
{
    socklen_t addr_size = (socklen_t)sizeof(remote_addrsin);
    conn_socket = accept(srv_socket,(struct sockaddr*)&remote_addrsin , &addr_size);
    if(conn_socket < 0)
    {
        socket_error = SRV_SOCKET_ACCEPT_ERROR;
        return -1;
    }
    return conn_socket;
}

/* 服务端轮询监听接收操作 */
int DevSocket::Read_Select(int maxsock,int seconds,int useconds)
{
    struct timeval timeout;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(maxsock, &readfds);
    timeout.tv_sec = seconds;
    timeout.tv_usec = useconds;
    return select(maxsock+1, &readfds, 0 ,0, &timeout);
}

/* TCP客户端接收函数 */
int DevSocket::client_recv(void *buf, int bytes, int flags)
{
    bytes_read = 0;
    int num_reads = 0;
    int num_requestrecv = bytes;     //需要接收的字节数
    char *preceivebuffer = (char*)buf;
    //循环接收需要接收的字节数
    while(bytes_read < bytes){
        if((num_reads = recv(srv_socket, preceivebuffer, num_requestrecv - bytes_read, flags))>0)
        {
            bytes_read += num_read;
            preceivebuffer += num_read;        
        }
        if (num_read<0){
            socket_error=SRV_SOCKET_RECEIVE_ERROR;
            return -1;
        }
    }
    return bytes_read;
}

/* TCP服务端接收函数 */
int DevSocket::server_recv(int socket, void *buf, int bytes,int seconds,int useconds,int flags)
{
    bytes_read=0;
    int num_read = 0;
    int num_requestrecv = (int)bytes;
    char*preceivebuffer = (char*)buf;
    while(bytes_read < bytes)
    {
        if(!ReadSelect(socket, seconds,useconds)){
            socket_error=SRV_SOCKET_REQUEST_TIMEOUT;
            return -1;
        }
        if((num_read = recv(socket, p, num_requestrecv - bytes_read, flags))>0){
            bytes_read += num_read;
            preceivebuffer += num_read;
        }
        if(num_read < 0){
            socket_error = SRV_SOCKET_RECEIVE_ERROR;
            return -1;
        }
    }
    return bytes_read;
}

/* TCP发送函数 */
int DevSocket::sock_send(int soket,const void *buf,int bytes,int flags)
{
    bytes_send = 0;
    int num_sends = 0;
    int num_requestsend = (int)bytes;
    char *psendbuffer = (char*)buf;
    while(bytes_send < num_requestsend)
    {
        if((num_sends = send(soket, psendbuffer,  num_requestsend - bytes_send, flags))>0)
        {
            bytes_send += num_sends;
            psendbuffer += num_sends;
        }
        if(num_sends<0){
            socket_error=SRV_SOCKET_TRANSMIT_ERROR;
            return -1;
        }
    }
    return bytes_send;
}


/* 客户端UDP接收函数 */
int DevSocket::udprecvfrom(void *buf, int bytes, int flags)
{
    socklen_t addr_size = (socklen_t)sizeof(sockaddr_in);
    int num_read = 0;
    int num_req = (int)bytes;
    char *preceivebuffer = (char *)buf;
    while(bytes_read < bytes)
    {
        if((num_read = recvfrom(srv_socket, preceivebuffer, num_req - bytes_read
                        , flags, (struct sockaddr *)sockaddr_sin, &addr_size)) > 0)
        {
            bytes_read += num_read;
            p += num_read;
        }
        if(num_read < 0)
        {
            socket_error = SRV_SOCKET_RECEIVE_ERROR;
            return -1;
        }

    }
    return bytes_read;
}
#endif
