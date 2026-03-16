/*
Created by Qunzhong WANG (http://qunzhongwang.github.io)
qunzhong@link.cuhk.edu.hk
https://github.com/qunzhongwang
*/

// self define module
#include "sockutil.h"

// C std
#include <cstring>
#include <ctime>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>
#include <cstring>



// CXX std
#include <iostream>

// POSIX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>


int main(){

	// --------------------------------------------------------------------- //
	//								Step 1/2								 //
	// --------------------------------------------------------------------- //
	// Connect to ROBOT on TCP port 3310 and send 10-char student ID.
	// ROBOT is already listening; we act as the TCP client here.
	int client_fd;

    logger(INFO, "Student: Creating TCP socket as Client");
	client_fd = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IPADDR, &server_addr.sin_addr);
    server_addr.sin_port = htons(LISTEN_PORT);


    char listenPortBuffer[6] = {'\0'};
    Connect(
        client_fd,
        (struct sockaddr*) &server_addr,
        sizeof(server_addr)
    );
    logger(INFO, "Student: Connected to Robot on port %d", LISTEN_PORT);

    // send 10-char student ID to ROBOT
    logger(INFO, "Student: Writing SID: %s", SID);
    Sock_write(
        client_fd,
        SID,
        10
    );
    logger(INFO, "Student: SID sent successfully");

	// --------------------------------------------------------------------- //
	//								Step 3									 //
	// --------------------------------------------------------------------- //
	// ROBOT sends a 5-char port string "ddddd". We create a listening
	// socket on that port and accept ROBOT's connection (~1s later).
	// The accepted connection is conn_fd — this is STUDENT's "s2".
    logger(INFO, "Student: Reading listen port from Robot");
    Sock_read(
        client_fd,
        listenPortBuffer,
        5
    );
    logger(INFO, "Student: Received listen port: %s", listenPortBuffer);


    int listen_fd;
    logger(INFO, "Student: Creating TCP listen socket on port %s", listenPortBuffer);
	listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in service;
    memset(&service, 0, (socklen_t) sizeof(service));
	service.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IPADDR, &service.sin_addr);
    service.sin_port = htons(atoi(listenPortBuffer));

    Bind(
        listen_fd,
        (struct sockaddr*) &service,
        (socklen_t) sizeof(service)
    );
    Listen(listen_fd, 1);
    logger(INFO, "Student: Listening on port %s, waiting for Robot...", listenPortBuffer);

    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
	memset(&client_addr, 0, addr_size);

    int conn_fd = Accept(
        listen_fd,
        (sockaddr*) &client_addr,
        &addr_size
    );
    logger(INFO, "Student: Accepted connection from Robot on port %s", listenPortBuffer);

	// --------------------------------------------------------------------- //
	//								Step 4									 //
	// --------------------------------------------------------------------- //
	// ROBOT sends 12-char string "fffff,eeeee." on conn_fd (s2).
	// fffff = ROBOT's UDP port, eeeee = STUDENT's UDP port.
	// We create a UDP socket s3, bind to eeeee, then:
	//   - sendto num (5 < num < 10) to ROBOT on fffff
	//   - recv num*10 char string from ROBOT on eeeee
	// ROBOT sends the string 5 times; we only need to receive one.
    char udpPortBuffer[13] = {'\0'};
    Sock_read(conn_fd, udpPortBuffer, 12);
    logger(INFO, "Student: Received UDP port info: %s", udpPortBuffer);

    // parse "fffff,eeeee." — first 5 chars = robot UDP port, chars 6-10 = student UDP port
    char robotUdpPortString[6] = {'\0'};
    for (int i = 0; i < 5; i++){
        robotUdpPortString[i] = udpPortBuffer[i];
    }
    int robotUdpPort = atoi(robotUdpPortString);
    logger(INFO, "Student: Robot UDP port = %s (%d)", robotUdpPortString, robotUdpPort);

    char studentUdpPortString[6] = {'\0'};
    for (int i = 0; i < 5; i++){
        studentUdpPortString[i] = udpPortBuffer[6+i];
    }
    int studentUdpPort = atoi(studentUdpPortString);
    logger(INFO, "Student: Student UDP port = %s (%d)", studentUdpPortString, studentUdpPort);

    // create UDP socket s3 and bind to student's UDP port
    int udp_fd;
	udp_fd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in studentAddr;
	studentAddr.sin_family = AF_INET;
	studentAddr.sin_port = htons(studentUdpPort);
	studentAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	Bind(udp_fd, (struct sockaddr *) &studentAddr, sizeof(studentAddr));
    logger(INFO, "Student: UDP socket s3 bound to port %d", studentUdpPort);

	// prepare robot's UDP address for sendto
	struct sockaddr_in robotAddr;
	robotAddr.sin_family = AF_INET;
	robotAddr.sin_port = htons(robotUdpPort);
	robotAddr.sin_addr.s_addr = inet_addr(SERVER_IPADDR);

    // send num (5 < num < 10) to ROBOT on fffff
    int num = 7;
    char numBuffer[2] = {'\0'};
    numBuffer[0] = num + '0';
    logger(INFO, "Student: Sending num=%d to Robot UDP port %d", num, robotUdpPort);
    Sendto(udp_fd, numBuffer, 1, 0, (struct sockaddr*)&robotAddr, sizeof(robotAddr));
    logger(INFO, "Student: Sent num=%d successfully", num);

    // receive num*10 char string from ROBOT (ROBOT sends 5 times, we only need one)
    char stringBuffer[100] = {'\0'};
    Recv(udp_fd, stringBuffer, 100, 0);
    logger(INFO, "Student: Received string (%d bytes): %s", (int)strlen(stringBuffer), stringBuffer);

	// --------------------------------------------------------------------- //
	//								Step 5									 //
	// --------------------------------------------------------------------- //
	// Send the received string back to ROBOT at UDP port fffff,
	// 5 times, once every 1 second. ROBOT checks if strings match.
    logger(INFO, "Student: Echoing string back to Robot 5 times...");
    for (int i = 0; i < 5; i++){
        Sendto(udp_fd, stringBuffer, 10 * num, 0, (struct sockaddr*)&robotAddr, sizeof(robotAddr));
        logger(INFO, "Student: UDP echo packet %d/5 sent", i+1);
        sleep(1);
    }

	// --------------------------------------------------------------------- //
	//								Step 7									 //
	// --------------------------------------------------------------------- //
	// Study the effect of receiver buffer size on TCP socket conn_fd (s2).
	// 1. Get existing SO_RCVBUF of conn_fd and print it.
	// 2. Send "bsXXX" to ROBOT on conn_fd so ROBOT knows the buffer size.
	// 3. ROBOT will send messages for 30 seconds on s2.
	// 4. Count and report received messages/bytes every 10 seconds.
    int bufsize;
    socklen_t len = sizeof(bufsize);
    Getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, &len);
    logger(INFO, "Student: Current receive buffer size: %d bytes", bufsize);

    char bufSizeString[100] = {'\0'};
    snprintf(bufSizeString, 100, "bs%d", bufsize);
    logger(INFO, "Student: Sending buffer size info to Robot: %s", bufSizeString);
    Sock_write(conn_fd, bufSizeString, strlen(bufSizeString));

    // set receive timeout — after robot stops sending, recv returns EAGAIN
    // instead of blocking forever. 2s is enough for robot to be well past done.
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    Setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    logger(INFO, "Student: Receiving messages for 30 seconds...");
    char messageBuffer[1024] = {'\0'};
    int messageCnt = 0;
    int bytesCnt = 0;
    time_t start_t;
    time(&start_t);
    time_t curr_t;
    time(&curr_t);
    time_t end_t = start_t + 30;

    // NOTE: raw recv (not Recv wrapper) because SO_RCVTIMEO causes
    // recv to return -1 with EAGAIN on timeout — that's expected, not an error
    while(curr_t < end_t){
        time(&curr_t);
        int rc = recv(conn_fd, messageBuffer, 1024, 0);
        if (rc > 0){
            bytesCnt += rc;
            messageCnt += 1;
        }
        if (curr_t - start_t > 10){
            logger(INFO, "[Student] Number of received messages: %d, Number of received bytes: %d", messageCnt, bytesCnt);
            messageCnt = 0; bytesCnt = 0;
            start_t = curr_t;
        }
    }
    errno = 0; // clear stale EAGAIN/ETIMEDOUT from SO_RCVTIMEO

    // --------------------------------------------------------------------- //
    //                              Step 8                                   //
    // --------------------------------------------------------------------- //
    // Repeat step 7 with different receive buffer sizes to measure throughput.
    // For each size in [1,5,10,25,50,200,500,1000] * 1000 bytes:
    //   1. setsockopt SO_RCVBUF to new size
    //   2. send "bsXXX" to ROBOT so it knows the new buffer size
    //   3. ROBOT sends messages for 30 seconds
    //   4. count messages/bytes, print stats every 10 seconds
    //   5. compute throughput = total_received_bytes / 30s
    int value_range[8] = {1,5,10,25,50,200,500,1000};

    for (int i = 0; i < 8; i++){
        int new_bufsize = value_range[i] * 1000;
        Setsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &new_bufsize, sizeof(new_bufsize));

        // kernel may double the value or clamp it — read back actual
        int actual_bufsize;
        socklen_t optlen = sizeof(actual_bufsize);
        Getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &actual_bufsize, &optlen);
        logger(INFO, "[Student] Round %d/8: requested buffer %d, actual: %d", i+1, new_bufsize, actual_bufsize);

        // inform ROBOT of new buffer size
        char bufSizeString2[100] = {'\0'};
        snprintf(bufSizeString2, 100, "bs%d", actual_bufsize);
        Sock_write(conn_fd, bufSizeString2, strlen(bufSizeString2));
        logger(INFO, "[Student] Sent buffer size info: %s, receiving for 30 seconds...", bufSizeString2);

        memset(messageBuffer, 0, 1024);
        int messageCnt2 = 0;
        int bytesCnt2 = 0;
        int totalBytes = 0;
        time_t start_t2;
        time(&start_t2);
        time_t curr_t2;
        time(&curr_t2);
        time_t end_t2 = start_t2 + 30;

        // raw recv — SO_RCVTIMEO returns EAGAIN on timeout, not a real error
        while(curr_t2 < end_t2){
            time(&curr_t2);
            int rc = recv(conn_fd, messageBuffer, 1024, 0);
            if (rc > 0){
                bytesCnt2 += rc;
                totalBytes += rc;
                messageCnt2 += 1;
            }
            if (curr_t2 - start_t2 > 10){
                logger(INFO, "[Student] Number of received messages: %d, Number of received bytes: %d", messageCnt2, bytesCnt2);
                messageCnt2 = 0; bytesCnt2 = 0;
                start_t2 = curr_t2;
            }
        }

        errno = 0; // clear stale EAGAIN/ETIMEDOUT from SO_RCVTIMEO
        double throughput = (double)totalBytes / 30.0;
        logger(INFO, "[Student] Buffer size: %d, Throughput: %.2f bytes/sec", actual_bufsize, throughput);
    }

    Close(client_fd);
    Close(conn_fd);
    Close(listen_fd);
    Close(udp_fd);
    logger(INFO, "Student: All sockets closed, exiting");

    return 0;
}
