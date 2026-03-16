#include <cstring>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "sockutil.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>


// ---- CSAPP-style error handling ----

void unix_error(const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


// ---- Socket creation and connection wrappers ----

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


// ---- Socket option wrappers ----

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




// `Sock_write` is a robust write helper that loops until all `n` bytes are written.
// It handles partial writes from the kernel, such as when the TCP send buffer is full.
// Returns `n` on success and `-1` on error.

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

// `Sock_read` is a robust read helper that loops until `n` bytes are read or EOF.
// It handles partial reads from the kernel, such as TCP receive-buffer boundaries.
// Returns the number of bytes actually read, which may be less than `n` on EOF.
// Returns `-1` on error.

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

// Export experiment results to a CSV file for plotting.
void export_csv(exp_result results[], int n, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Cannot open %s for writing: %s\n", filename, strerror(errno));
        return;
    }
    fprintf(fp, "round,requested_buf,actual_buf,total_messages,total_bytes,duration_sec,throughput_bytes_per_sec\n");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%d,%d,%d,%lld,%lld,%.1f,%.2f\n",
                i + 1, results[i].requested_buf, results[i].actual_buf,
                results[i].total_messages, results[i].total_bytes,
                results[i].duration_sec, results[i].throughput);
    }
    fclose(fp);
    printf("Results exported to %s\n", filename);
}

const char* get_server_ipaddr()
{
    const char* remote_robot = getenv("REMOTE_ROBOT");
    // Use the local ROBOT by default. Switch to the remote ROBOT for any
    // non-empty value other than the explicit false-like values below.
    if (remote_robot == NULL ||
        strcmp(remote_robot, "false") == 0 ||
        strcmp(remote_robot, "0") == 0) {
        return LOCAL_SERVER_IPADDR;
    }
    return REMOTE_SERVER_IPADDR;
}
