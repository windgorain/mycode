#ifndef SOCKET_OP_H
#define SOCKET_OP_H

#define MAX_BACK_CONN        32
#define NET_ERR_LEN         256
#define CONNECT_NONE        0

int create_socket(char *err, int domain, int type);
int set_nonblock_socket(char *err, int fd);
void set_net_error(char *err, const char *fmt, ...);
int set_socket_option(char *err, int fd, int option, int op);

int tcpSocketServer(char *err, char *ip, uint16_t port);
int tcpSocketClient(char *err, char *ip, uint16_t port, int flags);
int udpSocketServer(char *err, char *ip, uint16_t port);
int udpSocketClient(char *err, char *ip, uint16_t port, int flags);

int unixSocketClient(char *err, const char *unix_path, int type, int flags);
int unixSocketServer(char *err, const char *unix_path, int type, int flags);

int socketSend(int *err_num, int fd, uint8_t *buf, int buf_len);
int socketClose(int fd);
#endif
