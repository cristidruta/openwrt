#ifndef DEVSOCKET_H_
#define DEVSOCKET_H_
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>

enum DevSocketError{
    SRV_SOCKET_NO_ERROR=0,
    SRV_SOCKET_INVALID_ERROR_CODE,
    SRV_SOCKET_ACCEPT_ERROR,
    SRV_SOCKET_BIND_ERROR,
    SRV_SOCKET_BUFOVER_ERROR,
    SRV_SOCKET_CONNECT_ERROR,
    SRV_SOCKET_FILESYSTEM_ERROR,
    SRV_SOCKET_GETOPTION_ERROR,
    SRV_SOCKET_HOSTNAME_ERROR,
    SRV_SOCKET_INIT_ERROR,
    SRV_SOCKET_LISTEN_ERROR,
    SRV_SOCKET_PEERNAME_ERROR,
    SRV_SOCKET_PROTOCOL_ERROR,
    SRV_SOCKET_RECEIVE_ERROR,
    SRV_SOCKET_REQUEST_TIMEOUT,
    SRV_SOCKET_SERVICE_ERROR,
    SRV_SOCKET_SETOPTION_ERROR,
    SRV_SOCKET_SOCKNAME_ERROR,
    SRV_SOCKET_SOCKETTYPE_ERROR,
    SRV_SOCKET_TRANSMIT_ERROR,
};

class DevSocket {
    protected:
        sa_family_t address_family;
        int socket_type;
        int port_number;
        int protocol_family;

        sockaddr_in remote_addrsin;
        sockaddr_in sockaddr_sin;
        DevSocketError socket_error;

        int srv_socket;
    private:
        bool m_bstate_connect;
        int conn_socket;
        int bytes_read;
        int bytes_send;

    public:
        DevSocket();
        virtual ~DevSocket();
        DevSocket(int port);
        unsigned long bindserver_listen(int maxconn_num);
        int udprecvfrom(void *buf, int bytes, int seconds, int useconds,int flags);
        int udpSendTo(void *buf, int bytes, int flags);

#if 0
        DevSocket(int port, int stype, char* strhostname);
        DevSocket(sa_family_t af,int stype,int pro_family, int port,char *strhostname);
        int socket_create(void);
        unsigned int sock_connect();
        unsigned int sock_accept();
        int Read_Select(int maxsock,int seconds,int useconds);
        int client_recv(void *buf, int bytes, int flags);
        int server_recv(int socket,void *pbuf,int bytes, int seconds,int useconds,int flags);
        int sock_send(int soket,const void *buf,int bytes,int flags);
        int udprecvfrom(void *buf, int bytes, int flags);
#endif
}
#endif /* DEVSOCKET_H_ */
