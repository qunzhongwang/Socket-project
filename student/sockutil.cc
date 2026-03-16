#include <cstring>
#include <cstdarg>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "sockutil.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>


/*
 * logger - variadic printf-style logging to stderr
 * only appends errno info for ERROR level — INFO/WARNING calls
 * could have stale errno from previous syscalls which is misleading
 */
void logger(const char* level, const char* fmt, ...){
    int saved_errno = errno;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[%s] ", level);
    vfprintf(stderr, fmt, args);
    // only print errno for ERROR level
    if (strcmp(level, ERROR) == 0 && saved_errno){
        fprintf(stderr, ": %s", strerror(saved_errno));
    }
    fprintf(stderr, "\n");
    va_end(args);
    errno = 0;
}


// ---- CSAPP style — log and exit on error ----

void unix_error(const char *msg)
{
    logger(ERROR, "%s", msg);
    exit(1);
}


// ---- socket creation and connection wrappers ----

int Socket(int domain, int type, int protocol)
{
    int rc;
    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("Socket error");
    return rc;
}

void Bind(int sockfd, struct sockaddr *addr, socklen_t addrlen)
{
    if (bind(sockfd, addr, addrlen) < 0)
	unix_error("Bind error");
}

void Listen(int sockfd, int backlog)
{
    if (listen(sockfd, backlog) < 0)
	unix_error("Listen error");
}

int Accept(int listenfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;
    if ((rc = accept(listenfd, addr, addrlen)) < 0)
	unix_error("Accept error");
    return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    if (connect(sockfd, serv_addr, addrlen) < 0)
	unix_error("Connect error");
}

void Close(int fd)
{
    if (close(fd) < 0)
	unix_error("Close error");
}


// ---- socket option wrappers ----

void Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    if (setsockopt(sockfd, level, optname, optval, optlen) < 0)
	unix_error("Setsockopt error");
}

void Getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    if (getsockopt(sockfd, level, optname, optval, optlen) < 0)
	unix_error("Getsockopt error");
}


// ---- UDP wrappers ----

ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
    ssize_t rc;
    if ((rc = sendto(sockfd, buf, len, flags, dest_addr, addrlen)) < 0)
	unix_error("Sendto error");
    return rc;
}

ssize_t Recv(int sockfd, void *buf, size_t len, int flags)
{
    ssize_t rc;
    if ((rc = recv(sockfd, buf, len, flags)) < 0)
	unix_error("Recv error");
    return rc;
}


// ---- robust I/O helpers ----

/*
 * Sock_write - robust write, loops until all n bytes are written
 * handles partial writes from kernel (TCP send buffer full, etc.)
 * returns n on success, -1 on error
 */
int Sock_write(int fd, const char* buf, int n){
    int left_count = n;
    int rc = 0;
    const char* bufp = buf;
    while(left_count > 0){
        rc = write(fd, bufp, left_count);
        if (rc <= 0){
            return -1;
        }
        left_count -= rc;
        bufp += rc;
    }
    return n;
}

/*
 * Sock_read - robust read, loops until n bytes read or EOF
 * handles partial reads from kernel (TCP recv buffer, packet boundaries)
 * returns number of bytes actually read (may be < n on EOF)
 * returns -1 on error
 */
int Sock_read(int fd, char* buf, int n){
    int left_count = n;
    int rc = 0;
    char* bufp = buf;
    while(left_count > 0){
        rc = read(fd, bufp, left_count);
        if (rc < 0){
            return -1;
        }
        if (rc == 0){
            break;
        }
        left_count -= rc;
        bufp += rc;
    }
    return (n - left_count);
}
