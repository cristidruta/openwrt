#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define TCP_SERVER_PORT 1883
#define LISTENTQ 5
#define MAX_EPOLL_SIZE 10
#define DEF_PROCESS_NUM 1

int g_fd=-1;
struct epoll_event g_events[MAX_EPOLL_SIZE];

static int g_keeploop=1;

extern int led_onOff();
extern int led_color();
extern int main_service(int sockfd);

/*
 * Set TCP server's address
 */
static void fill_sockaddr(struct sockaddr_in *addr, char *ip, int port)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port?port:TCP_SERVER_PORT);
    if (ip == NULL)
    {
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if ((addr->sin_addr.s_addr = inet_addr(ip)) == EOF)
    {
       perror("fill_sockaddr(),inet_addr()"); 
       exit(EXIT_FAILURE);
    } 
}

/*
 * Create a TCP Server Return socketfd
 */
static int create_tcpsvr(char *ip, int port) 
{
    int sockfd;
    int reuse;
    struct sockaddr_in addr;

    if ((sockfd=socket(AF_INET, SOCK_STREAM, 0)) == EOF)
    {
        perror("create_tcpsvr(),socket()");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        printf("setsockopt() SO_REUSEADDR failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fill_sockaddr(&addr,ip,port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == EOF)
    {
        perror("create_tcpsvr(),bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd,LISTENTQ) == EOF)
    {
        perror("create_tcpsvr(),bind()");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

static void clearFd()
{
    int i=0;

    close(g_fd);
    for(i=0; i<MAX_EPOLL_SIZE; i++)
    {
        if (g_events[i].data.fd != g_fd)
        {
            g_events[i].data.fd=-1;
        }
    }
    g_fd=-1;
}

static void handle_sigusr1()
{
    if (led_onOff() < 0)
    {
        clearFd();
    }
}

static void handle_sigusr2()
{
    if (led_color() < 0)
    {
        clearFd();
    }
}

void handle_sigterm()
{
    g_keeploop = 0;
}

int main(int argc, char *argv[])
{
    struct epoll_event ev;
    int nfds=0, i=0;
    int epollfd=-1;
    int listenfd=-1;
    char cmd[256]={0};

    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    signal(SIGTERM, handle_sigterm);

    system("iptables -t nat -D PREROUTING -p tcp --dport 1883 -j DNAT --to-destination 192.168.1.1:1883");
    system("iptables -t nat -A PREROUTING -p tcp --dport 1883 -j DNAT --to-destination 192.168.1.1:1883");
    sprintf(cmd, "echo %d > /var/tuyaled_pid", (unsigned int)getpid());
    system(cmd);

    for(i=0; i<MAX_EPOLL_SIZE; i++)
    {
        g_events[i].data.fd=-1;
    }

    if((epollfd=epoll_create(MAX_EPOLL_SIZE)) == EOF)
    {
        printf("epoll_create() failed: %s!", strerror(errno));
        exit(EXIT_FAILURE);
    }

    listenfd=create_tcpsvr("0.0.0.0", TCP_SERVER_PORT);

    ev.events=EPOLLIN|EPOLLET;
    ev.data.fd=listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev))
    {
        printf("epoll_ctl() failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while (g_keeploop)
    {
        nfds=epoll_wait(epollfd, g_events, MAX_EPOLL_SIZE, -1);

        for(i=0; i<nfds; i++)
        {
            if(g_events[i].data.fd == listenfd)
            {
                int conn_fd=accept(listenfd, 0, 0);
                if (conn_fd==EOF)
                {
                    perror("accept() failed!");
                    g_keeploop=0;
                    break;
                }
                fcntl(conn_fd, F_SETFL, O_NONBLOCK);
                ev.events = EPOLLIN|EPOLLET;
                ev.data.fd = conn_fd;
                g_fd=conn_fd;
                if (epoll_ctl(epollfd,EPOLL_CTL_ADD, conn_fd, &ev) == EOF)
                {
                    perror("epoll_ctl() failed!");
                    g_keeploop=0;
                    break;
                }
            }
            else
            {
                int sockfd=g_events[i].data.fd;

                if (sockfd<0)
                {
                    continue;
                }

                if (main_service(sockfd) == -1)
                {
                    close(sockfd);
                    g_events[i].data.fd=-1;
                }
            }
        }
    }

    for(i=0; i<MAX_EPOLL_SIZE; i++)
    {
        if (g_events[i].data.fd != -1)
        {
            close(g_events[i].data.fd);
        }
    }

    system("iptables -t nat -D PREROUTING -p tcp --dport 1883 -j DNAT --to-destination 192.168.1.1:1883");

    return 0;
}
