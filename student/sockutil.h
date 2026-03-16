#pragma once

// System headers required by the socket wrapper declarations.
#include <sys/socket.h>
#include <sys/types.h>

#define LISTEN_PORT 3310
#define ROBOT_VERSION "3.0"
#define LOCAL_SERVER_IPADDR "127.0.0.1"
#define REMOTE_SERVER_IPADDR "172.16.75.1"
#define SID "0000000000"
typedef long long int ll;
struct exp_result {
    int requested_buf;   // Requested `SO_RCVBUF` value.
    int actual_buf;      // Actual `SO_RCVBUF` value after kernel adjustment.
    ll total_messages;
    ll total_bytes;
    double duration_sec;
    double throughput;   // bytes/sec
};


 
void unix_error(const char *msg);

// Socket creation and connection helpers.
int Socket(int domain, int type, int protocol);
void Bind(int sockfd, struct sockaddr *addr, socklen_t addrlen);
void Listen(int sockfd, int backlog);
int Accept(int listenfd, struct sockaddr *addr, socklen_t *addrlen);
void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
void Close(int fd);

// Socket option helpers.
void Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
void Getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);

// UDP send and receive helpers.
ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t Recv(int sockfd, void *buf, size_t len, int flags);

// Robust I/O helpers that loop until the requested byte count is transferred.
int Sock_write(int fd, const char* buf, int n);
int Sock_read(int fd, char* buf, int n);

// Export experiment results to a CSV file for plotting.
void export_csv(exp_result results[], int n, const char* filename);

// Choose the ROBOT/server IP address at runtime from `REMOTE_ROBOT`.
const char* get_server_ipaddr();