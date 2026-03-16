#pragma once

// system headers needed for socket wrapper declarations
#include <sys/socket.h>
#include <sys/types.h>

#define LISTEN_PORT 3310
#define ROBOT_VERSION "3.0"
#define INFO "info"
#define WARNING "warning"
#define ERROR "error"
#define SERVER_IPADDR "127.0.0.1"
#define SID "1155210998"


/*
 * logger - variadic printf-style logging to stderr
 * format: [level] message [: errno string if ERROR]
 * only appends errno info for ERROR level to avoid stale errno on INFO calls
 */
void logger(const char* level, const char* fmt, ...);


/*
 * CSAPP-style wrappers
 * Capitalized versions of POSIX calls with built-in error handling.
 * On failure, each wrapper calls unix_error() which logs and exits.
 *
 * @book{bryant2016csapp,
 *   title     = {Computer Systems: A Programmer's Perspective},
 *   author    = {Bryant, Randal E. and O'Hallaron, David R.},
 *   edition   = {3rd},
 *   year      = {2016},
 *   publisher = {Pearson}
 * }
 */
void unix_error(const char *msg);

// socket creation and connection
int Socket(int domain, int type, int protocol);
void Bind(int sockfd, struct sockaddr *addr, socklen_t addrlen);
void Listen(int sockfd, int backlog);
int Accept(int listenfd, struct sockaddr *addr, socklen_t *addrlen);
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
void Close(int fd);

// socket options
void Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
void Getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);

// UDP sendto / recv
ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t Recv(int sockfd, void *buf, size_t len, int flags);

// robust I/O helpers (loop until n bytes transferred)
int Sock_write(int fd, const char* buf, int n);
int Sock_read(int fd, char* buf, int n);
