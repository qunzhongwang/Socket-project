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

	// char listenPortBuffer[1024];

	// char* studentIP;
	// int messageBufferSize = sizeof(messageBuffer);
	int client_fd;

    // create client socket to connect to the server
    logger(INFO, "Student: Creating TCP socket as Client");

    // s2
	client_fd = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IPADDR, &server_addr.sin_addr);
    server_addr.sin_port = htons(LISTEN_PORT);
    
    
    // 
    char listenPortBuffer[6] = {'\0'};
    Connect(
        client_fd,
        (struct sockaddr*) &server_addr,
        sizeof(server_addr)
    );
    logger(INFO, "Student: Writting SID");
    logger(INFO, SID);
    Sock_write(
        client_fd,
        SID,
        10
    );
    logger(INFO, "Student: Write successfully");
    logger(INFO, "Student: Reading Port");
    Sock_read(
        client_fd,
        listenPortBuffer,
        5
    );
    logger(INFO, "Student: Read successfully");
    logger(INFO, listenPortBuffer);


    int listen_fd;
    logger(INFO, "Student: Creating TCP socket as server");
	listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in service;
    memset(&service, 0, (socklen_t) sizeof(service));
	service.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IPADDR, &service.sin_addr);
    service.sin_port = htons(atoi(listenPortBuffer));

    int rc = bind(
        listen_fd,
        (struct sockaddr*) &service,
        (socklen_t) sizeof(service)
    );

    listen(
        listen_fd,
        1
    );

    struct sockaddr_in client_addr; 
    int size = sizeof(client_addr);
	memset(&client_addr, 0,  size);

    int conn_fd = accept(
        listen_fd,
        (sockaddr*) &client_addr,
        (socklen_t *) & size
    );
    logger(INFO, "Student: Connected to Robot Client successfully");
    char udpPortBuffer[13];
    Sock_read(conn_fd, udpPortBuffer, 12);
    logger(INFO, "Student: Read UDP Port successfully");
    logger(INFO, udpPortBuffer);

    // // Create a UDP socket to send the data
	// printf("\nPreparing socket s3 <%d>...", iUDPPortStudent);
    int udp_fd;
	udp_fd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    char robotUdpPortString[6] = {'\0'};;
    for (int i = 0; i < 5; i++){
        robotUdpPortString[i] = udpPortBuffer[i];
    }
    int robotUdpPort = atoi(robotUdpPortString);
    logger(INFO, "Student: Read Robot UDP Port successfully");
    logger(INFO, robotUdpPortString);

    char studentUdpPortString[6] =  {'\0'};;
    for (int i = 0; i < 5; i++){
        studentUdpPortString[i] = udpPortBuffer[6+i];
    }
    int studentUdpPort = atoi(studentUdpPortString);

    logger(INFO, "Student: Read Student UDP Port successfully");
    logger(INFO, studentUdpPortString);

	struct sockaddr_in studentAddr;
	studentAddr.sin_family = AF_INET;
	studentAddr.sin_port = htons(studentUdpPort);
	studentAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(udp_fd, (struct sockaddr *) &studentAddr, sizeof(studentAddr));
	
	//	Prepare the receiver address for the UDP packet
	struct sockaddr_in robotAddr;
	robotAddr.sin_family = AF_INET;
	robotAddr.sin_port = htons(robotUdpPort);
	robotAddr.sin_addr.s_addr = inet_addr(SERVER_IPADDR);

    int num = 7;
    char numBuffer[2] =  {'\0'};;
    char stringBuffer[100] =  {'\0'};;
    numBuffer[0] = num + '0';
    logger(INFO, robotUdpPortString);

    sendto(udp_fd, numBuffer, 1, 0, (struct sockaddr*)&robotAddr, sizeof(robotAddr));
    logger(INFO, "Student: Send variable num successfully");
    logger(INFO, numBuffer);
    recv(udp_fd, stringBuffer, 100, 0);
    logger(INFO, "Student: Receive String successfully");
    logger(INFO, stringBuffer);

    for (int i = 0; i < 5; i++){
        sendto(udp_fd, stringBuffer, 10 * num, 0, (struct sockaddr*)&robotAddr, sizeof(robotAddr));
        sleep(1);
    }

    int bufsize;
    socklen_t len = sizeof(bufsize);
    getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, &len);
    logger(INFO, "Receive buffer: %d bytes\n", bufsize);

    char bufSizeString[100] = {'\0'};
    snprintf(bufSizeString, 100, "bs%d", bufsize);
    
    Sock_write(conn_fd, bufSizeString, strlen(bufSizeString));

    // set receive timeout so recv doesn't block forever after robot stops sending
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char messageBuffer[1024] = {'\0'};
    int messageCnt = 0;
    int bytesCnt = 0;
    time_t start_t;
    time(&start_t);
    time_t curr_t;
    time(&curr_t);
    time_t end_t = start_t + 30;

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

    // --------------------------------------------------------------------- //
    //                              Step 8                                   //
    // --------------------------------------------------------------------- //
    int value_range[8] = {1,5,10,25,50,200,500,1000};

    for (int i = 0; i < 8; i++){
        int new_bufsize = value_range[i] * 1000;
        setsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &new_bufsize, sizeof(new_bufsize));

        int actual_bufsize;
        socklen_t optlen = sizeof(actual_bufsize);
        getsockopt(conn_fd, SOL_SOCKET, SO_RCVBUF, &actual_bufsize, &optlen);
        logger(INFO, "[Student] Set buffer size to %d, actual: %d", new_bufsize, actual_bufsize);

        char bufSizeString2[100] = {'\0'};
        snprintf(bufSizeString2, 100, "bs%d", actual_bufsize);
        Sock_write(conn_fd, bufSizeString2, strlen(bufSizeString2));

        memset(messageBuffer, 0, 1024);
        int messageCnt2 = 0;
        int bytesCnt2 = 0;
        int totalBytes = 0;
        time_t start_t2;
        time(&start_t2);
        time_t curr_t2;
        time(&curr_t2);
        time_t end_t2 = start_t2 + 30;

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

        double throughput = (double)totalBytes / 30.0;
        logger(INFO, "[Student] Buffer size: %d, Throughput: %.2f bytes/sec", actual_bufsize, throughput);
    }

    close(client_fd);
    close(conn_fd);
    close(listen_fd);
    close(udp_fd);

    return 0;
}