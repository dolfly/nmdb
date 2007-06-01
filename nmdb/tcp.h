
#ifndef _TCP_H
#define _TCP_H

/* Maximum number of TCP connections */
#define MAX_TCPFD 2048

int tcp_init(void);
void tcp_close(int fd);
void tcp_newconnection(int fd, short event, void *arg);

#endif

