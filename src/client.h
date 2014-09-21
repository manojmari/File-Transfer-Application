/*
 * client.h
 *
 *  Created on: Feb 13, 2014
 *      Author: manoj
 */
#ifndef CLIENT_H_
#define CLIENT_H_
#include <string>
#include "server.h"
#include "common.h"

class Client {
private:
	int address, myport, serverSockfd, clientSockfd;
	std::string strMyIpAddress;
	std::multiset<sock_info, comp> oServersList;
	struct sock_info serverInfo;
	sock_info oMyInfo;

	static const int STDIN = 0;
	static const int BACKLOG = 3;

	bool bRegistered, bRecievingList;

	std::string List;
	int userInput(Server *myServer);
	int openSocketAndConnect(struct addrinfo *res, int currPortNo,
			Server *myServer);
	int getAddress(std::string ipAddress, int socket, struct addrinfo **res);
	int openListeningPort(struct addrinfo *res);
	int openSocketAndListen(struct addrinfo *res);
	void handleServerList(Server *myServer);
	void handleData(int current_fd, Server *myServer);
	std::string getData(int current_fd);
	void getMyHostName(struct sock_info *oMyInfo);
public:
	Client(int sock);
	int establishConnection(std::string ipAddress, int serverPort,
			Server *myServer);

};

#endif /* CLIENT_H_ */
