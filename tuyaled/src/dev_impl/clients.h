#ifndef __DEV_CLIENTS_H__ 
#define __DEV_CLIENTS_H__ 

int clients_init(clients_t* list);

void clients_clear(clients_t* list);

void clients_read_fds(clients_t* list, fd_set* set, int* max_fd);

int clients_handle_accept(int fd, clients_t* list, fd_set* set);

int clients_read(clients_t* list, fd_set* set);
#endif
