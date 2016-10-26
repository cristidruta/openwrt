#ifndef TCPPROXY_controlers_h_INCLUDED
#define TCPPROXY_controlers_h_INCLUDED

#include <sys/select.h>

#include "slist.h"
//#include "tcp.h"


void controlers_read_fds(fd_set* set, int* max_fd);
int controlers_handle_accept(fd_set* set);
int controlers_read(fd_set* set);
int ctlers_fd();
int ctlers_init();
void ctlers_clean();
#endif
