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



// logger style
void logger(const char* level, const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[%s] ", level);
    vfprintf(stderr, fmt, args);
    if (errno){
        fprintf(stderr, ": %s", strerror(errno));
    }
    fprintf(stderr, "\n");
    va_end(args);
}

// CSAPP style
void unix_error(char *msg)
{
    logger(ERROR, msg);
    exit(0);
}

int Socket(int domain, int type, int protocol) 
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("Socket error");
    return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen) 
{
    int rc;
    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
	unix_error("Connect error");
}


/*
@book{bryant2016csapp,
  title     = {Computer Systems: A Programmer's Perspective},
  author    = {Bryant, Randal E. and O'Hallaron, David R.},
  edition   = {3rd},
  year      = {2016},
  publisher = {Pearson}
}
*/

int Sock_write(int fd, char* buf, int n){
    int left_count = n;
    int rc = 0;
    char* sockbuf = buf;
    while(left_count > 0){
        rc = write(fd, buf,left_count);
        if (rc <= 0){
            return -1;
        }
        left_count -= rc;
        sockbuf += rc;
    }
    return n;
}

int Sock_read(int fd, char* buf, int n){
    int left_count = n;
    int rc = 0;
    char* sockbuf = buf;
    while(left_count > 0){
        rc = read(fd, buf,left_count);
        if (rc < 0){
            return -1;
        }
        if (rc == 0){
            break;
        }
        left_count -= rc;
        sockbuf += rc;
    }
    return (n - left_count);
}