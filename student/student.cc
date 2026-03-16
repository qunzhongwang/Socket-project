/*
Created by Qunzhong WANG (http://qunzhongwang.github.io)
qunzhong@link.cuhk.edu.hk
https://github.com/qunzhongwang
*/

// Project headers
#include "sockutil.h"

// C standard library headers
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdio.h>
#include <errno.h>

// POSIX headers
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(){
    const char* serverIpAddr = get_server_ipaddr();
    printf("[Student: Setup] Using server IP %s (REMOTE_ROBOT=%s)\n",
           serverIpAddr,
           getenv("REMOTE_ROBOT") ? getenv("REMOTE_ROBOT") : "<unset>");

	// --------------------------------------------------------------------- //
	//								Step 1/2								 //
	// --------------------------------------------------------------------- //
	// Connect to the ROBOT on TCP port 3310 and send the 10-character student ID.
    
	int client_fd;

    printf("[Student: Step 1] Creating TCP socket as Client\n");
	client_fd = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, serverIpAddr, &server_addr.sin_addr);
    server_addr.sin_port = htons(LISTEN_PORT);


    char listenPortBuffer[6] = {'\0'};
    Connect(
        client_fd,
        (struct sockaddr*) &server_addr,
        sizeof(server_addr)
    );
    printf("[Student: Step 2] Connected to Robot on port %d\n", LISTEN_PORT);

    // Send the 10-character student ID to the ROBOT.
    printf("[Student: Step 2] Writing SID: %s\n", SID);
    Sock_write(
        client_fd,
        SID,
        10
    );
    printf("[Student: Step 2] SID sent successfully\n");

	// --------------------------------------------------------------------- //
	//								Step 3									 //
	// --------------------------------------------------------------------- //
	// The ROBOT sends a 5-character port string, "ddddd". Create a listening
	// socket on that port and accept the ROBOT's connection about 1 second later.

    printf("[Student: Step 3] Reading listen port from Robot\n");
    Sock_read(
        client_fd,
        listenPortBuffer,
        5
    );
    printf("[Student: Step 3] Received listen port: %s\n", listenPortBuffer);


    int listen_fd;
    printf("[Student: Step 3] Creating TCP listen socket on port %s\n", listenPortBuffer);
	listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in service;
    memset(&service, 0, (socklen_t) sizeof(service));
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(atoi(listenPortBuffer));

    Bind(
        listen_fd,
        (struct sockaddr*) &service,
        (socklen_t) sizeof(service)
    );
    Listen(listen_fd, 1);
    printf("[Student: Step 3] Listening on port %s, waiting for Robot...\n", listenPortBuffer);

    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
	memset(&client_addr, 0, addr_size);

    int conn_fd = Accept(
        listen_fd,
        (sockaddr*) &client_addr,
        &addr_size
    );
    printf("[Student: Step 3] Accepted connection from Robot on port %s\n", listenPortBuffer);

	// --------------------------------------------------------------------- //
	//								Step 4									 //
	// --------------------------------------------------------------------- //
	// The ROBOT sends the 12-character string "fffff,eeeee." on `conn_fd` (s2).
	// Here, `fffff` is the ROBOT's UDP port and `eeeee` is the STUDENT's UDP port.
	// Create a UDP socket `s3`, bind it to `eeeee`, then:
	// send `num` (where 5 < num < 10) to the ROBOT on `fffff`
	// receive a `num * 10` character string from the ROBOT on `eeeee`

    char udpPortBuffer[13] = {'\0'};
    Sock_read(conn_fd, udpPortBuffer, 12);
    printf("[Student: Step 4] Received UDP port info: %s\n", udpPortBuffer);

    // Parse "fffff,eeeee.": the first 5 characters are the ROBOT UDP port,
    // and characters 6-10 are the STUDENT UDP port.
    char robotUdpPortString[6] = {'\0'};
    for (int i = 0; i < 5; i++){
        robotUdpPortString[i] = udpPortBuffer[i];
    }
    int robotUdpPort = atoi(robotUdpPortString);
    printf("[Student: Step 4] Robot UDP port = %s (%d)\n", robotUdpPortString, robotUdpPort);

    char studentUdpPortString[6] = {'\0'};
    for (int i = 0; i < 5; i++){
        studentUdpPortString[i] = udpPortBuffer[6+i];
    }
    int studentUdpPort = atoi(studentUdpPortString);
    printf("[Student: Step 4] Student UDP port = %s (%d)\n", studentUdpPortString, studentUdpPort);

    // Create UDP socket `s3` and bind it to the student's UDP port.
    int udp_fd;
	udp_fd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in studentAddr;
	studentAddr.sin_family = AF_INET;
	studentAddr.sin_port = htons(studentUdpPort);
	studentAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	Bind(udp_fd, (struct sockaddr *) &studentAddr, sizeof(studentAddr));
    printf("[Student: Step 4] UDP socket s3 bound to port %d\n", studentUdpPort);

	// Prepare the ROBOT's UDP address for `sendto`.
	struct sockaddr_in robotAddr;
	robotAddr.sin_family = AF_INET;
	robotAddr.sin_port = htons(robotUdpPort);
	robotAddr.sin_addr.s_addr = inet_addr(serverIpAddr);

    // Send `num` (where 5 < num < 10) to the ROBOT on `fffff`.
    int num = rand()%4 + 6;
    char numBuffer[2] = {'\0'};
    numBuffer[0] = num + '0';
    printf("[Student: Step 4] Sending num=%d to Robot UDP port %d\n", num, robotUdpPort);
    Sendto(udp_fd, numBuffer, 1, 0, (struct sockaddr*)&robotAddr, sizeof(robotAddr));
    printf("[Student: Step 4] Sent num=%d successfully\n", num);

    // Receive the `num * 10` character string from the ROBOT.
    // The ROBOT sends it 5 times, but only one copy is needed.
    char stringBuffer[100] = {'\0'};
    Recv(udp_fd, stringBuffer, 100, 0);
    printf("[Student: Step 4] Received string (%d bytes): %s\n", (int)strlen(stringBuffer), stringBuffer);

	// --------------------------------------------------------------------- //
	//								Step 5									 //
	// --------------------------------------------------------------------- //
	// Send the received string back to the ROBOT at UDP port `fffff`
	// 5 times, once per second. The ROBOT checks whether the strings match.

    printf("[Student: Step 5] Echoing string back to Robot 5 times...\n");
    for (int i = 0; i < 5; i++){
        Sendto(udp_fd, stringBuffer, 10 * num, 0, (struct sockaddr*)&robotAddr, sizeof(robotAddr));
        printf("[Student: Step 5] UDP echo packet %d/5 sent\n", i+1);
        sleep(1);
    }

    // --------------------------------------------------------------------- //
	//								Step 6								 //
	// --------------------------------------------------------------------- //
    // Remote experiment.
    // This step is covered by the runtime choice between
    // `LOCAL_SERVER_IPADDR` and `REMOTE_SERVER_IPADDR`.
    // If `REMOTE_SERVER_IPADDR` is selected, Steps 1-5 already run the Step 6 experiment.

    

	// --------------------------------------------------------------------- //
	//								Step 7									 //
	// --------------------------------------------------------------------- //
	// Study the effect of the receive buffer size on TCP socket `conn_fd` (s2).
	// 1. Read and print the current `SO_RCVBUF` value of `conn_fd`.
	// 2. Tell the ROBOT the agreed message size (1000 bytes) via "bsXXX".
	// 3. The ROBOT sends 1000-byte messages on `s2` for 30 seconds.
	// 4. Count complete messages and report every 10 seconds + final summary.

    int bufsize;
    socklen_t len = sizeof(bufsize);
    Getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, &len);

    // Send actual buffer size to ROBOT; ROBOT will cap message size at 1000.
    // Both sides use 1000-byte messages so total_bytes / total_messages = 1000.
    int msg_size = 1000;

    char bufSizeString[100] = {'\0'};
    snprintf(bufSizeString, 100, "bs%d", bufsize);
    Sock_write(conn_fd, bufSizeString, strlen(bufSizeString));

    // Set a receive timeout so that `recv` returns `EAGAIN` after the ROBOT
    // stops sending, instead of blocking forever. Two seconds is sufficient.
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    Setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char* recvBuf = (char*)malloc(msg_size);

    printf("============================================================\n");
    printf("[Step 7] EXPERIMENT: Receiving messages from ROBOT for 30 seconds...\n");
    printf("============================================================\n");

    ll step7_totalMsgs = 0;
    ll step7_totalBytes = 0;
    time_t start_t;
    time(&start_t);
    time_t curr_t;
    int next_report = 10; // print stats every 10 seconds

    while (1) {
        time(&curr_t);
        double elapsed = difftime(curr_t, start_t);
        if (elapsed >= 30) break;

        // Print 10-second interval statistics.
        if (elapsed >= next_report) {
            printf("[%.1fs] Messages: %lld, Bytes: %lld, Throughput: %.2f KB/s\n",
                   elapsed, step7_totalMsgs, step7_totalBytes,
                   (double)step7_totalBytes / elapsed / 1024.0);
            next_report += 10;
        }

        // Robust recv: loop until exactly msg_size bytes are read (one message).
        int nread = 0;
        while (nread < msg_size) {
            int rc = recv(conn_fd, recvBuf + nread, msg_size - nread, 0);
            if (rc > 0) {
                nread += rc;
            } else {
                break; // timeout or error
            }
        }
        step7_totalBytes += nread;
        if (nread == msg_size) {
            step7_totalMsgs++;
        } else {
            break; // incomplete message — robot likely stopped
        }
    }
    free(recvBuf);
    errno = 0;

    double step7_duration = 30.0;
    double step7_throughput = (double)step7_totalBytes / step7_duration;
    printf("\n============================================================\n");
    printf("EXPERIMENT RESULTS\n");
    printf("============================================================\n");
    printf("[STUDENT] Receiver buffer size: %d bytes\n", bufsize);
    printf("[STUDENT] Message size: %d bytes\n", bufsize);
    printf("[STUDENT] Number of received messages: %lld\n", step7_totalMsgs);
    printf("[STUDENT] Total received bytes: %lld\n", step7_totalBytes);
    printf("[STUDENT] Duration: %.2f seconds\n", step7_duration);
    printf("[STUDENT] Throughput: %.2f bytes/s (%.2f KB/s)\n",
           step7_throughput, step7_throughput / 1024.0);

    // --------------------------------------------------------------------- //
    //                              Step 8                                   //
    // --------------------------------------------------------------------- //
    // Repeat Step 7 with different receive buffer sizes to measure throughput.
    int value_range[8] = {1,5,10,25,50,200,500,1000};
    // Store the results for the Step 8 experiment rounds.
    exp_result results[8];

    for (int i = 0; i < 8; i++){
        
        int new_bufsize = value_range[i] * 10;
        Setsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &new_bufsize, sizeof(new_bufsize));

        // The kernel may double or clamp the requested value, so read back the actual size.
        int actual_bufsize;
        socklen_t optlen = sizeof(actual_bufsize);
        Getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &actual_bufsize, &optlen);
        printf("[Student: Step 8] Round %d/8: requested buffer %d, actual: %d\n", i+1, new_bufsize, actual_bufsize);

        // Inform the ROBOT of the new buffer size.
        char bufSizeString2[100] = {'\0'};
        snprintf(bufSizeString2, 100, "bs%d", actual_bufsize);
        Sock_write(conn_fd, bufSizeString2, strlen(bufSizeString2));

        // Each application-layer message is `actual_bufsize` bytes.
        int msg_size8 = actual_bufsize;
        char* recvBuf8 = (char*)malloc(msg_size8);
        printf("[Student: Step 8] Receiving %d-byte messages for 30 seconds...\n", msg_size8);

        ll totalMsgs = 0;
        ll totalBytes = 0;
        time_t start_t2;
        time(&start_t2);
        time_t curr_t2;

        while (1) {
            time(&curr_t2);
            if (curr_t2 - start_t2 >= 30) break;

            // Robust recv: loop until exactly msg_size8 bytes are read.
            int nread = 0;
            while (nread < msg_size8) {
                int rc = recv(conn_fd, recvBuf8 + nread, msg_size8 - nread, 0);
                if (rc > 0) {
                    nread += rc;
                } else {
                    break; // timeout or error
                }
            }
            totalBytes += nread;
            if (nread == msg_size8) {
                totalMsgs++;
            } else {
                break; // incomplete message — robot likely stopped
            }
        }
        free(recvBuf8);

        errno = 0;
        double throughput = (double)totalBytes / 30.0;
        printf("[Student: Step 8] Number of received messages: %lld, total received bytes: %lld\n",
               totalMsgs, totalBytes);
        printf("[Student: Step 8] Buffer size: %d, Throughput: %.2f bytes/sec\n",
               actual_bufsize, throughput);

        results[i].requested_buf  = new_bufsize;
        results[i].actual_buf     = actual_bufsize;
        results[i].total_messages = totalMsgs;
        results[i].total_bytes    = totalBytes;
        results[i].duration_sec   = 30.0;
        results[i].throughput     = throughput;
    }

    // Export the summary results to a CSV file.
    export_csv(results, 8, "throughput_results.csv");

    // Drain conn_fd until the robot closes its end (or the existing 2s SO_RCVTIMEO
    // fires), so the robot is never mid-send when we close — avoids SIGPIPE on robot.
    {
        char drain_buf[4096];
        while (recv(conn_fd, drain_buf, sizeof(drain_buf), 0) > 0)
            ;
    }

    Close(client_fd);
    Close(conn_fd);
    Close(listen_fd);
    Close(udp_fd);
    printf("Student: All sockets closed, exiting\n");

    return 0;
}
