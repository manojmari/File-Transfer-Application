/*
 * client.cpp
 *
 *  Created on: Feb 13, 2014
 *      Author: manoj
 */
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

Client::Client(int port) {

	myport = port;
	strMyIpAddress = getMyIpAddress();

	oMyInfo.port_no = port;
	strcpy(oMyInfo.ip_address, (char *) strMyIpAddress.c_str());

	getMyHostName(&oMyInfo);

	struct addrinfo *res;
	bRegistered = false;
	bRecievingList = false;
	Server *myServer;
	Server servr(oMyInfo, 3);
	myServer = &servr;

	while (1) {
		cout << strDisplayMsg;
		cout.flush();
		myServer->read_fds = myServer->master;
		if (select(myServer->fdmax + 1, &(myServer->read_fds), NULL, NULL, NULL)
				== -1) {
			perror("select");
			exit(4);
		}
		for (int i = 0; i <= myServer->fdmax; i++) {
			if (FD_ISSET(i,&(myServer->read_fds))) {
				if (i == STDIN) {
					//user enters a key
					userInput(myServer);
					//continue;
				} else {
					cout << string(strlen(strDisplayMsg), '\b');
					if (i == myServer->sockfd) {
						//new connect request
						int new_fd = (*myServer).acceptClient(oServersList);

					} else if (i == serverSockfd) {
						//data from server
						cout << string(strlen(strDisplayMsg), ' ') << endl;
						handleServerList(myServer);
					} else if ((*myServer).isClient(i)) {
						//data from some client
						myServer->download(i);
					} else {
						cout << "unexpected reached here" << endl;
					}
				}
			}
		}

	}
}

/*
 * processes data from the server
 * which will always be the servers server-ip list
 * */

void Client::handleServerList(Server *myServer) {
	char buffer[MAXDATASIZE];
	if (!bRecievingList) {
		bRecievingList = true;
		oServersList.clear();
		oServersList.insert(serverInfo);
	}

	int response = (*myServer).getClientMessage(serverSockfd, buffer);
	if (response == 1) {
		cout << "Server stopped!!\nBye!" << endl;
		exit(0);
	}
	if (strcmp(buffer, "EOF") == 0) {
		bRecievingList = false;
		cout << "\nChange in the network\nUpdated Server IP List:\n";
		(*myServer).displayList(oServersList);
		cout << "\n\n";
	} else {
		struct sock_info *temp;
		temp = (struct sock_info*) buffer;
		oServersList.insert(*temp);
	}
	cout.flush();
}

/*
 * accept user input for client
 *
 * */
int Client::userInput(Server *myServer) {

	string uInput = "help";
	getline(cin, uInput);

	vector<string> arrSplit = splitString(uInput, " ");
	if (arrSplit.size() == 0)
		return 0;
	if (arrSplit[0].compare("creator") == 0) {
		displayCreatorInfo();
		return 0;
	}
	std::transform(arrSplit[0].begin(), arrSplit[0].end(), arrSplit[0].begin(),
			::tolower);
	if (arrSplit[0].compare("register") == 0) {
		if (!bRegistered) {
			if (arrSplit.size() < 3) {
				cout << "Not enough Parameters (See help for options)" << endl;
				return 0;
			}
			if (!checkInteger(arrSplit[2])) {
				cout << "The port number should be a number" << endl;
				return 0;
			}
			int port = atoi((char*) (arrSplit[2]).c_str());
			serverSockfd = establishConnection(arrSplit[1], port, myServer);

			/*adding servers details*/
			if (serverSockfd != -1) {
				bRegistered = true;
				cout << "Successfully registered." << endl;
			} else {
				cout << "Error Registering!!" << endl;
			}
		} else {
			cout << "You are already connected to a server!!" << endl;
		}
		return 0;
	}
	if (arrSplit[0].compare("exit") == 0) {
		cout << "Bye!" << endl;
		exit(1);
	}

	if (arrSplit[0].compare("myip") == 0) {
		cout << "Your IP is: " << strMyIpAddress << endl;
		return 0;
	}

	if (arrSplit[0].compare("myport") == 0) {
		cout << "Your Port number is " << myport << endl;
		return 0;
	}

	if (arrSplit[0].compare("help") == 0) {
		displayFunctions(false, bRegistered);
		return 0;
	}
	if (arrSplit[0].compare("list") == 0) {
		cout << "\n\nList of users you are connected to:" << endl;
		(*myServer).displayList();
		cout << "\n\n";
		return 0;
	}
	if (arrSplit[0].compare("clear") == 0) {
		system("clear");
		return 0;
	}

	if (arrSplit[0].compare("upload") == 0) {
		if (!bRegistered) {
			cout << "Please register before performing this action!" << endl;
			return 0;
		}
		if (arrSplit.size() % 2 == 0 || arrSplit.size() == 1) {
			cout << "Improper Parameters!!!" << endl;
			return 0;
		}
		for (int index = 1; index + 1 < arrSplit.size(); index = index + 2) {
			if (checkInteger(arrSplit[index])) {
				int id = atoi(arrSplit[index].c_str());
				if (id > 1) {
					myServer->upload(id - 1, arrSplit[index + 1]);
				} else {
					cout << "Permission denied: cannot send file to the server"
							<< endl;
				}
			} else {
				cout << "ID should be a number! error in " << arrSplit[index]
						<< "!!" << endl;
			}
		}
		return 0;
	}

	if (arrSplit[0].compare("download") == 0) {
		if (!bRegistered) {
			cout << "Please register before performing this action!" << endl;
			return 0;
		}
		if (arrSplit.size() % 2 == 0 || arrSplit.size() == 1) {
			cout << "Improper Parameters!!!" << endl;
			return 0;
		}
		for (int index = 1; index + 1 < arrSplit.size(); index = index + 2) {
			if (checkInteger(arrSplit[index])) {
				int id = atoi(arrSplit[index].c_str());
				if (id > 1) {
					myServer->requestDownload(id - 1, arrSplit[index + 1]);
				} else {
					cout
							<< "Permission denied: cannot download file from the server"
							<< endl;
				}
			} else {
				cout << "ID should be a number! error in " << arrSplit[index]
						<< "!!" << endl;
			}
		}
		return 0;
	}
	if (arrSplit[0].compare("connect") == 0) {
		if (!bRegistered) {
			cout << "Please register before performing this action!" << endl;
			return 0;
		}
		if (arrSplit.size() < 3) {
			cout << "Not enough Parameters (See help for options)" << endl;
			return 0;
		}
		if (!checkInteger(arrSplit[2])) {
			cout << "The port number should be a number" << endl;
			return 0;
		}
		if (myServer->oServerIpList.size() > MAXCLIENTCONNECTION) {
			cout << "Cannot connect to more than " << MAXCLIENTCONNECTION<<" hosts"
					<< endl;
			return 0;
		}

		int port = atoi((char*) (arrSplit[2]).c_str());
		if ((*myServer).validateClient(oServersList,
				(char *) arrSplit[1].c_str(), port)) {
			if (myServer->validateClient(myServer->oServerIpList,
					(char *) arrSplit[1].c_str(), port)) {
				cout << "You are already connected to the host!!" << endl;
			} else
				int connect_fd = establishConnection(arrSplit[1], port,
						myServer);
		} else {
			cout << "Host not found in the network!!" << endl;
		}
		return 0;
	}
	if (arrSplit[0].compare("terminate") == 0) {
		if (!bRegistered) {
			cout << "Please register before performing this action!" << endl;
			return 0;
		}
		if (checkInteger(arrSplit[1])) {
			int id = atoi(arrSplit[1].c_str());
			if (id > 1) {
				myServer->removeClient(id - 1);
			} else {
				cout << "You cannot terminate connection with the server!!"
						<< endl;
			}
		} else {
			cout << "ID should be a string!!" << endl;
		}
		return 0;
	}

	cout << "please enter a valid command; enter HELP for list of commands"
			<< endl;

	return 0;
}

/*
 * establishes connection to either the server
 * or other clients in the network
 * */
int Client::establishConnection(string ipAddress, int serverPort,
		Server *myServer) {

	char buf[MAXDATASIZE];
	struct addrinfo *res;
	int status = getAddress(ipAddress, serverPort, &res);
	if (status != 0) {
		cout << "error getting address" << endl;
		return -1;
	}

	int connection_fd = openSocketAndConnect(res, serverPort, myServer);
	if (status != 0) {
		cout << "error connecting to server" << endl;
		return -1;
	}
	return connection_fd;
}

/*
 * gets address information for client
 * */
int Client::getAddress(std::string ipAddress, int serverPort,
		struct addrinfo** res) {
	struct addrinfo hints;
	char *chIpAddress, *portChar;
	chIpAddress = (char*) ipAddress.c_str();
	string sPort = intToString(serverPort);

	portChar = (char*) sPort.c_str();
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo((char*) ipAddress.c_str(), portChar, &hints, res);

	return status;
}

/*
 * opens socket and connects to the server
 * before adding the client to its own server-ip list
 * */
int Client::openSocketAndConnect(struct addrinfo *res, int currPortNo,
		Server *myServer) {
	struct addrinfo *p;
	char s[INET_ADDRSTRLEN];
	int status;
	int connection_fd;

	for (p = res; p != NULL; p = p->ai_next) {
		if ((connection_fd = ::socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if ((status = ::connect(connection_fd, p->ai_addr, p->ai_addrlen))
				== -1) {
			close(connection_fd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}

	sockaddr *sa = (struct sockaddr *) p->ai_addr;

	inet_ntop(p->ai_family, &(((struct sockaddr_in*) sa)->sin_addr), s,
			sizeof s);

	struct hostent *he;
	struct in_addr ipv4addr = ((struct sockaddr_in*) p->ai_addr)->sin_addr;
	inet_pton(AF_INET, s, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	if (strcmp(s, strMyIpAddress.c_str()) == 0 && currPortNo == myport) {
		cout << "Please avoid connecting to yourself! :|" << endl;
		close(connection_fd);
		return -1;
	}

	string strIP = s;
	string strPort = intToString(myport);
	string strMessage = strPort + " " + oMyInfo.ip_address + " "
			+ oMyInfo.host_name;
	(*myServer).sendMessage(connection_fd, (char *) strMessage.c_str());
	int i = 0;
	char buffer[MAXDATASIZE];
	myServer->getClientMessage(connection_fd, buffer);

	if (!bRegistered) {
		if (strcmp(buffer, "CONN_REJECTED") == 0) {
			cout << "Destination host is not a server!!" << endl;
			close(connection_fd);
			return -1;
		}
		strcpy(serverInfo.host_name, he->h_name);
		serverInfo.host_name[strlen(he->h_name)] = '\0';
		strcpy(serverInfo.ip_address, strIP.c_str());
		serverInfo.port_no = currPortNo;
		serverInfo.sock_fd = connection_fd;

		myServer->addClient(serverInfo);
	} else {

		if (strcmp(buffer, "CONN_REJECTED") == 0) {
			cout
					<< "Destination host rejected request(Max connections reached)."
					<< endl;
			close(connection_fd);
			return -1;
		}
		char c_ip[INET_ADDRSTRLEN];
		strcpy(c_ip, strIP.c_str());
		(*myServer).addClient(connection_fd, c_ip, currPortNo, he->h_name);
	}

	return connection_fd;
}

/*
 * gets clients own host name
 * */
void Client::getMyHostName(struct sock_info *oMyInfo) {
	struct hostent *he;
	struct in_addr ipv4addr;
	inet_pton(AF_INET, oMyInfo->ip_address, &ipv4addr);
	he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
	strcpy(oMyInfo->host_name, he->h_name);
}
