#pragma once
#define LISTEN_PORT 3310
#define ROBOT_VERSION "3.0"
#define INFO "info"
#define WARNING "warning"
#define ERROR "error"
#define SERVER_IPADDR "127.0.0.1"
#define SID "1155210998"


// logger style (variadic, printf-like)
void logger(const char* level, const char* fmt, ...);

// CSAPP style
void unix_error(char *msg);

int Socket(int domain, int type, int protocol);

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);

int Sock_write(int fd, char* buf, int n);

int Sock_read(int fd, char* buf, int n);