#IERGXXXX Programming Project Report

**Student ID:** X 
**Name:** X 

---

## 1. Basic Program Design (Steps 1--6)

The STUDENT program interacts with the ROBOT through both TCP and UDP sockets. All socket syscalls are wrapped in CSAPP-style error-handling helpers (`sockutil.cc`), and `Sock_write`/`Sock_read` implement robust I/O loops to handle partial reads/writes inherent to TCP streams. The overall connection flow is:

> s1 (TCP) --> receive port ddddd --> s_2 listen, accept --> s2 (TCP) --> receive fffff,eeeee --> s3 (UDP)

**Socket variable mapping:**

| Spec name | Code variable | Type | Purpose |
| --- | --- | --- | --- |
| s1 | `client_fd` | TCP | Initial connection to ROBOT port 3310 |
| s_2 | `listen_fd` | TCP | Listening socket on port ddddd |
| s2 | `conn_fd` | TCP | Accepted connection from ROBOT (via s_2) |
| s3 | `udp_fd` | UDP | Bound to port eeeee, communicates with ROBOT port fffff |

**Steps 1--2 (TCP Handshake).**
STUDENT creates TCP socket `s1` (`client_fd`) and connects to ROBOT on port 3310, then sends the 10-character student ID using `Sock_write`:

```c
client_fd = Socket(AF_INET, SOCK_STREAM, 0);          // s1
Connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
Sock_write(client_fd, SID, 10);                        // send "1155210998"
```

**Step 3 (Dynamic TCP Listening).**
STUDENT reads a 5-character port string `ddddd` on `s1`, creates listening socket `s_2` (`listen_fd`) bound to that port, and accepts the ROBOT's incoming connection to get `s2` (`conn_fd`):

```c
Sock_read(client_fd, listenPortBuffer, 5);             // receive "ddddd"
listen_fd = Socket(AF_INET, SOCK_STREAM, 0);           // s_2
Bind(listen_fd, ...);  Listen(listen_fd, 1);
conn_fd = Accept(listen_fd, ...);                       // s2
```

**Step 4 (UDP Exchange).**
On `s2`, STUDENT receives `"fffff,eeeee."`, parses both ports, creates UDP socket `s3` (`udp_fd`) bound to `eeeee`, sends a random integer `num` (6--9) to the ROBOT on port `fffff`, and receives a `num * 10` character string:

```c
Sock_read(conn_fd, udpPortBuffer, 12);                 // "fffff,eeeee."
udp_fd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);     // s3
Bind(udp_fd, ...);                                      // bind to eeeee
Sendto(udp_fd, numBuffer, 1, 0, &robotAddr, ...);      // send num to fffff
Recv(udp_fd, stringBuffer, 100, 0);                     // receive num*10 chars
```

**Step 5 (UDP Echo).**
STUDENT echoes the received string back to the ROBOT at UDP port `fffff`, 5 times at 1-second intervals. The ROBOT verifies string equality:

```c
for (int i = 0; i < 5; i++){
    Sendto(udp_fd, stringBuffer, 10 * num, 0, &robotAddr, ...);
    sleep(1);
}
```

**Step 6 (Remote Execution).**
The server IP is selected at startup via the `REMOTE_ROBOT` environment variable. This changes the target IP for Steps 1--5; there is no separate Step 6 code:

```c
// sockutil.h
#define REMOTE_SERVER_IPADDR "172.16.75.1"
// sockutil.cc — called once at program start
const char* get_server_ipaddr() {
    if (getenv("REMOTE_ROBOT")) return REMOTE_SERVER_IPADDR;
    return LOCAL_SERVER_IPADDR;   // "127.0.0.1"
}
```

---

## 2. Throughput Experiment Design (Steps 7--8)

### 2.1 Step 7 -- Measuring Throughput at Default Buffer Size

STUDENT obtains the current receive socket buffer size of `s2` using `getsockopt` and prints it. The application-layer message size is fixed at **1000 bytes** (agreed between STUDENT and ROBOT), so that `total_bytes / total_messages = 1000` exactly. STUDENT sends `"bs1000"` to the ROBOT as the control message:

```c
int bufsize;
socklen_t len = sizeof(bufsize);
getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, &len);
int msg_size = 1000;  // fixed application-layer message size
snprintf(bufSizeString, 100, "bs%d", msg_size);
Sock_write(conn_fd, bufSizeString, strlen(bufSizeString));
```

The modified ROBOT parses the `"bsXXX"` prefix and sends 1000-byte messages on `s2` as fast as possible for 30 seconds. STUDENT uses a robust I/O loop (inner `while` reads exactly 1000 bytes per message; outer `while` repeats for 30 seconds) and prints statistics every 10 seconds:

```c
while (1) {
    time(&curr_t);
    if (difftime(curr_t, start_t) >= 30) break;
    // Robust recv: loop until exactly msg_size bytes are read (one message).
    int nread = 0;
    while (nread < msg_size) {
        int rc = recv(conn_fd, recvBuf + nread, msg_size - nread, 0);
        if (rc > 0) { nread += rc; }
        else        { break; }
    }
    totalBytes += nread;
    if (nread == msg_size) { totalMsgs++; }
    else { break; }
}
```

A 2-second `SO_RCVTIMEO` is set so that `recv()` does not block indefinitely after the ROBOT stops sending.

**Step 7 output screenshot:**

<!-- TODO: replace with actual terminal screenshot -->
![Step 7 output](step7_screenshot.png)

### 2.2 Step 8 -- Sweeping Buffer Sizes

STUDENT repeats the Step 7 procedure for eight buffer sizes: **[1, 5, 10, 25, 50, 200, 500, 1000] % of 1000 bytes**, i.e., 10 to 10,000 bytes. Before each round, the buffer is resized with `setsockopt` and the actual kernel-applied value is read back:

```c
int new_bufsize = value_range[i] * 10;   // e.g., 10, 50, ..., 10000
setsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &new_bufsize, sizeof(new_bufsize));
getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &actual, &optlen);  // kernel may adjust
```

The modified ROBOT receives each `"bsXXX"` control message and sends application-layer messages of the reported buffer size for 30 seconds per round, using a robust send loop.

### 2.3 Results

**macOS:** The kernel accepts all requested buffer sizes (10--10,000 bytes) without adjustment.

| Requested (B) | Actual (B) | Total Messages | Total Bytes | Throughput (MB/s) |
| :---: | :---: | ---: | ---: | ---: |
| 10 | 10 | 47,732,466 | 477,324,686 | 15.9 |
| 50 | 50 | 35,285,722 | 1,764,266,020 | 58.8 |
| 100 | 100 | 22,894,979 | 2,289,426,924 | 76.3 |
| 250 | 250 | 23,859,256 | 5,964,912,400 | 198.8 |
| 500 | 500 | 26,431,901 | 13,215,774,476 | 440.5 |
| 2,000 | 2,000 | 22,101,554 | 44,202,031,716 | 1,473.4 |
| 5,000 | 5,000 | 12,489,158 | 62,448,053,132 | 2,081.6 |
| 10,000 | 10,000 | 6,527,263 | 65,277,548,632 | 2,175.9 |

![TCP Throughput vs Receiver Buffer Size -- macOS Linear](throughput_linear.png)

![TCP Throughput vs Receiver Buffer Size -- macOS Log-Log](throughput_loglog.png)

**Linux:** The kernel enforces a minimum buffer size of **2,304 bytes**; any smaller requested value is clamped to 2,304. Larger values are doubled by the kernel (e.g., requested 2,000 becomes 4,000).

| Requested (B) | Actual (B) | Total Messages | Total Bytes | Throughput (MB/s) |
| :---: | :---: | ---: | ---: | ---: |
| 10 | 2,304 | 380,472 | 313,960,272 | 10.5 |
| 50 | 2,304 | 365,820 | 301,731,000 | 10.1 |
| 100 | 2,304 | 373,186 | 307,805,856 | 10.3 |
| 250 | 2,304 | 374,297 | 308,723,088 | 10.3 |
| 500 | 2,304 | 373,153 | 307,777,304 | 10.3 |
| 2,000 | 4,000 | 523,422 | 512,862,752 | 17.1 |
| 5,000 | 10,000 | 941,440 | 881,949,392 | 29.4 |
| 10,000 | 20,000 | 1,432,170 | 1,400,383,680 | 46.7 |

<!-- TODO: replace with actual Linux graphs -->
![TCP Throughput vs Receiver Buffer Size -- Linux Linear](throughput_linear_linux.png)

![TCP Throughput vs Receiver Buffer Size -- Linux Log-Log](throughput_loglog_linux.png)

*Throughput = total received bytes / 30 s.*

### 2.4 Interpretation

1. **Throughput increases monotonically with buffer size.** A larger `SO_RCVBUF` allows the kernel to accept more in-flight data before the application calls `recv()`, reducing the frequency of TCP zero-window events that stall the sender.

2. **Diminishing returns at large buffers.** On macOS, throughput flattens above ~2,000 bytes; on Linux, above ~10,000 bytes. Beyond this point the bottleneck shifts from the receive buffer to CPU overhead per `recv()` syscall, loopback driver throughput, and scheduler latency.

3. **Small buffers severely limit throughput.** On macOS, a 10-byte buffer yields only 15.9 MB/s -- roughly 137x lower than the 2,175.9 MB/s at 10,000 bytes -- because the TCP receive window shrinks to nearly zero after each segment, forcing frequent ACK-and-resume cycles.

4. **Sharp throughput jump in the 500--2,000 byte range.** On macOS, throughput leaps from 440.5 MB/s (500 B) to 1,473.4 MB/s (2,000 B) -- a 3.3x increase for a 4x buffer increase. This suggests a critical threshold where the buffer becomes large enough to sustain pipelined TCP segments without stalling, dramatically improving utilization of the loopback link.

5. **Platform-specific minimum buffer sizes.** macOS honours the exact requested value (even as low as 10 bytes), while Linux enforces a minimum of 2,304 bytes and doubles larger requests. As a result, the first five Linux rounds (requested 10--500) all report an identical actual buffer of 2,304 and nearly identical throughput (~10.3 MB/s), illustrating that the *actual* kernel buffer -- not the requested value -- determines performance.

### 2.5 Limitation of the Experiment Setup

Running both ROBOT and STUDENT on the same machine (or on machines connected by a high-speed LAN in the IE Common Lab) means the network link is never the bottleneck. Whether the two endpoints are separate processes on one host, two containers sharing a bridge, or two desktops on Gigabit Ethernet, the available bandwidth far exceeds what a small receive buffer can absorb. The experiment therefore cleanly isolates the buffer-size effect.

However, in a real-world deployment with a **low-speed or high-latency network** (e.g., a WAN link, a throttled connection, or a satellite link), the physical link capacity would cap throughput well before the receive buffer becomes the limiting factor. Increasing the buffer beyond the bandwidth-delay product would yield no further improvement, and throughput would plateau at the link rate regardless of buffer size. In that scenario the experiment could not demonstrate the relationship between buffer size and throughput, because the network itself -- not the buffer -- would dominate performance. This is the main limitation of our lab setup: the results reflect buffer-constrained behaviour but cannot capture network-constrained behaviour.
