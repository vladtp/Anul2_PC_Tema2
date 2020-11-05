#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

using namespace std;

int main(int argc, char *argv[])
{
	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char msg[BUFF_LEN];
    cli_msg send_msg;

	fd_set read_fds;
	fd_set tmp_fds;	

	if (argc != 4) {
		cout << "Usage: ./subscriber <ID_Client> <IP_Server> <Port_Server>";
        return -1;
	}

    // create TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Failed to create socket");

    // connect to server
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "Function inet_aton failed");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "Failed to connect to server");

    // send client ID to server
    send_msg.op = ID_OP;
    memcpy(&send_msg.topic, argv[1], sizeof(send_msg.topic));
    ret = send(sockfd, &send_msg, sizeof(send_msg), 0);
    DIE(ret < 0, "failed to send ID");

    // add socket and stdin to file descriptor set
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	FD_SET(sockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	while (1) {
		tmp_fds = read_fds; 
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// read input from keyboard
            char command[50], topic[50];
            unsigned char sf;
			memset(msg, 0, BUFF_LEN);
			// fgets(msg, BUFF_LEN - 1, stdin);
            cin >> command;
			if (strncmp(command, "exit", strlen("exit")) == 0) {
				break;
			}
            if (strncmp(command, "subscribe", strlen("subscribe")) == 0) {
                cin >> topic >> sf;
                if (sf == '1') {
                    sf = 1;
                } else {
                    sf = 0;
                }
                send_msg.op = SUB_OP;
                strcpy(send_msg.topic, topic);
                send_msg.sf = sf;

                // send message to server
                ret = send(sockfd, &send_msg, sizeof(send_msg), 0);
                DIE(ret < 0, "Sending message to server failed");
                cout << "Subscribed to " << topic << ".\n";
            } else if (strncmp(command, "unsubscribe", strlen("unsubscribe")) == 0) {
                cin >> topic;
                send_msg.op = UNSUB_OP;
                strcpy(send_msg.topic, topic);

                // send message to server
                ret = send(sockfd, &send_msg, sizeof(send_msg), 0);
                DIE(ret < 0, "Sending message to server failed");
                cout << "Unsubscribed from " << topic << ".\n";
            } else {
                perror("Unknown command or wrong syntax");
            }	
		} else if (FD_ISSET(sockfd, &tmp_fds)) {
            srv_msg recv_msg;
			ret = recv(sockfd, &recv_msg, sizeof(srv_msg), 0);

            // connexion closed
            if (ret == 0) {
                break;
            }
            
            // parse message and write to stdin
            uint8_t type = recv_msg.type;
            string src_addr = inet_ntoa(recv_msg.ip_addr);
            src_addr += ":";
            uint16_t src_port = ntohs(recv_msg.port_addr);
            if (type == INT_TYPE) {
                cout << src_addr << ":" << src_port << " - " << recv_msg.topic << " - INT - ";
                uint8_t sign = recv_msg.payload[0];
                if (sign == 1) {
                    cout << "-";
                } else {
                    cout << "+";
                }
                uint32_t value;
                memcpy(&value, recv_msg.payload + 1, sizeof(uint32_t));
                value = ntohl(value);
                cout << value << "\n";
            } else if (type == SHORT_TYPE) {
                cout << src_addr << ":" << src_port << " - " << recv_msg.topic
                     << " - SHORT_REAL - ";
                uint16_t value;
                memcpy(&value, recv_msg.payload, sizeof(uint_fast16_t));
                value = htons(value);
                float real_value = 1.0 * value / 100;
                cout << real_value << "\n";
            } else if (type == STRING_TYPE) {
                cout << src_addr << ":" << src_port << " - " << recv_msg.topic << " - STRING - ";
                cout << recv_msg.payload << "\n";
            } else if (type == FLOAT_TYPE) {
                cout << src_addr << ":" << src_port << " - " << recv_msg.topic << " - FLOAT - ";
                uint8_t sign = recv_msg.payload[0];
                if (sign == 1) {
                    cout << "-";
                } else {
                    cout << "+";
                }
                uint32_t value;
                memcpy(&value, recv_msg.payload + 1, sizeof(uint32_t));
                value = ntohl(value);
                uint8_t exp = recv_msg.payload[5];
                float real_value = 1.0 * value * pow(10, -1 * exp);
                cout << real_value << "\n";
            }
		}
	}

	close(sockfd);
	return 0;
}
