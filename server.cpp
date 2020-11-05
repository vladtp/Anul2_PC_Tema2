#include <iostream>
#include <unordered_map>
#include <list>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h> 
#include "helpers.h"
								
using namespace std;

int create_udp_socket(int port, fd_set *read_fds, int &fdmax) {
	// create UDP socket
	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_socket == 0, "UDP socket failed")

	// bind UDP socket
	struct sockaddr_in serv_addr;
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int ret = ::bind(udp_socket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "UDP bind failed");

	// add TCP sockfd to read_fds
	FD_SET(udp_socket, read_fds);
	fdmax = max(fdmax, udp_socket);

	return udp_socket;
}

int create_tcp_socket(int port, fd_set *read_fds, int &fdmax) {
	// create TCP socket
    int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sockfd == 0, "TCP socket failed")

	// set TCP options
	int yes = 1;
	int ret = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));
	DIE(ret < 0, "TCP set options failed");

    // bind TCP socket
	struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = ::bind(tcp_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "TCP bind failed");

    // accept new clients
    ret = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "TCP listen failed");

	// add TCP sockfd to read_fds
	FD_SET(tcp_sockfd, read_fds);
	fdmax = max(fdmax, tcp_sockfd);

	return tcp_sockfd;
}


void accept_tcp_connexion(int tcp_sockfd, fd_set *read_fds, int &fdmax,
		 std::unordered_map<int, string> &active_clients,
		 std::unordered_map<string, int> &id_to_sock, std::unordered_map<string,
		 list<srv_msg>> &store) {
	// accept connection
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	int newsockfd = accept(tcp_sockfd, (struct sockaddr *) &cli_addr, &cli_len);
	DIE(newsockfd < 0, "failed to accept TCP connection");

	// get ID from client
	cli_msg recv_msg;
	int ret = recv(newsockfd, &recv_msg, sizeof(cli_msg), 0);
	DIE(ret < 0, "Failed to recive ID from TCP client");

	// add new soket to the file descriptor set
	FD_SET(newsockfd, read_fds);
	fdmax = max(fdmax, newsockfd);

	// keep track of ID and current open sockets
	string id;
	id = recv_msg.topic;
	active_clients[newsockfd] = id;
	id_to_sock[id] = newsockfd;

	// check if there are any stored messages for client
	if (store.find(id) != store.end()) {
		if (store[id].size() > 0) {
			for (auto msg : store[id]) {
				int ret = send(newsockfd, &msg, sizeof(srv_msg), 0);
				DIE(ret < 0, "forwarding stored message to TCP client failed")
			}
			store.erase(store.find(id));
		}
	}

	// print message to stdin
	printf("New client %s connected from %s:%d.\n", recv_msg.topic,
			inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
}

void recive_tcp_message(int sockfd, fd_set *read_fds,
		std::unordered_map<int, string> &active_clients,
		std::unordered_map<string, list<subscriber>> &topic_to_id,
		std::unordered_map<string, int> &id_to_sock) {
	// get message
	char msg[BUFF_LEN];
	memset(msg, 0, BUFF_LEN);
	int ret = recv(sockfd, msg, sizeof(msg), 0);
	DIE(ret < 0, "recieve from TCP client failed");

	if (ret == 0) {
		// connexion closed
		string id = active_clients[sockfd];
		cout << "Client " << id << " disconnected\n";
		active_clients.erase(active_clients.find(sockfd));
		id_to_sock.erase(id_to_sock.find(id));
		close(sockfd);
		
		// remove socket from file descriptor set 
		FD_CLR(sockfd, read_fds);
	} else {
		// check if a subscribe command was send and add ID to the topic subscriber list
		// else remove ID from topic subscriber list
		cli_msg *recv_msg = (cli_msg *) msg;
		if (recv_msg->op == SUB_OP) {
			string topic = recv_msg->topic;
			subscriber new_sub;
			new_sub.id  = active_clients[sockfd];
			new_sub.sf = recv_msg->sf;
			if (topic_to_id.find(topic) == topic_to_id.end()) {
				list<subscriber> aux;
				topic_to_id[topic] = aux;
			}
			topic_to_id[topic].push_back(new_sub);
			// cout << "ID: " << new_sub.id << " subscribed to " << topic << " with SF set to " << recv_msg->sf << "\n"; // DEBUG
		} else if(recv_msg->op == UNSUB_OP) {
			string topic = recv_msg->topic;
			string id = active_clients[sockfd];
			list<subscriber> subs = topic_to_id[topic];
			for (auto it = subs.begin(); it != subs.end(); ) {
				if (it->id.compare(id) == 0) {
					subs.erase(it);
					break;
				}
				it++;
			}
			topic_to_id[topic] = subs;
			// cout << "ID: " << id << " unsubscribed from " << topic << "\n"; // DEBUG
		}
	}
}

bool read_stdin() {
	char msg[BUFF_LEN];

	// read input from keyboard
	memset(msg, 0, BUFF_LEN);
	fgets(msg, BUFF_LEN - 1, stdin);

	// close server
	if (strncmp(msg, "exit", 4) == 0) {
		return  false;
	}
	return true;
}

void forward(int udp_sockfd, std::unordered_map<string, list<subscriber>> &topic_to_id,
		std::unordered_map<string, int> &id_to_sock,
		std::unordered_map<string, list<srv_msg>> &store) {
	char buffer[BUFF_LEN];
	memset(buffer, 0 ,BUFF_LEN);
	struct sockaddr_in from_client;
	socklen_t socklen;
	srv_msg send_msg;

	// get message from UDP client
	recvfrom(udp_sockfd, buffer, BUFF_LEN, 0, (struct sockaddr *) &from_client, &socklen);

	// prepare msg to send to TCP clients
	memcpy(send_msg.topic, buffer, 50);
	send_msg.topic[50] = '\0';
	send_msg.type = (uint8_t) buffer[50];
	memcpy(send_msg.payload, &buffer[51], 1500);
	send_msg.payload[1500] = '\0';
	send_msg.ip_addr = from_client.sin_addr;
	send_msg.port_addr = from_client.sin_port;

	// send msg to each subscriber
	string topic = send_msg.topic;
	list<subscriber> subs = topic_to_id[topic];
	for (auto sub : subs) {
		int id_sockfd = id_to_sock[sub.id];
		// if client is currently connected
		if (id_sockfd != 0) {
			int ret = send(id_sockfd, &send_msg, sizeof(srv_msg), 0);
			DIE(ret < 0, "forwarding message to TCP client failed")
		} else {
			// if store&forward is set
			int sf = sub.sf;
			if (sf == 1) {
				// if no list is created, make one
				if (store.find(sub.id) == store.end()) {
					list<srv_msg> aux;
					store[sub.id] = aux;
				}
				store[sub.id].push_back(send_msg);
			}
		}
	}
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: ./server <PORT_DORIT>";
        return -1;
    }

	bool running = true;
    int ret, fdmax = 0, port = atoi(argv[1]);
	std::unordered_map<int, string> active_clients;
	std::unordered_map<string, list<subscriber>> topic_to_id;
	std::unordered_map<string, int> id_to_sock;
	std::unordered_map<string, list<srv_msg>> store;
	 

	// initialize file descriptor set
    fd_set read_fds, tmp_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	// create tcp socket for connecting with clients and udp socket
    int tcp_sockfd = create_tcp_socket(port, &read_fds, fdmax);
	int udp_sockfd = create_udp_socket(port, &read_fds, fdmax);

	while (running) {
		tmp_fds = read_fds; 
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select failed");

		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcp_sockfd) {
					// new connexion from client
					accept_tcp_connexion(tcp_sockfd, &read_fds, fdmax,
							active_clients, id_to_sock, store);
				} else if (i == udp_sockfd) {
					// get msg from UDP client and forward it to TCP client
					forward(udp_sockfd, topic_to_id, id_to_sock, store);
				} else if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
					// read command from stdin
					running = read_stdin();
				} else {
					// new data has been recived from one of the clients
					recive_tcp_message(i, &read_fds, active_clients, topic_to_id, id_to_sock);
				}
			}
		}
	}

	close(tcp_sockfd);
	close(udp_sockfd);
	for (int i = 3; i <= fdmax; ++i) {
		if (FD_ISSET(i, &read_fds)) {
			close(i);
		}
	}
	return 0;
}