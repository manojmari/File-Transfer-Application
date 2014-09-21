/*
 * server.cpp
 *
 *  Created on: Feb 15, 2014
 *      Author: manoj
 */
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

Server::Server(sock_info oMyInfo, int clientCount) {
	oMySockInfo = oMyInfo;
	serverInit(oMyInfo.port_no, false, clientCount);
}

Server::Server(int port, bool server, int clientCount) {
	serverInit(port, server, clientCount);
}

void Server::serverInit(int port, bool server, int clientCount) {
	myport = port;
	isServer = server;

	fdmax = 0;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(STDIN, &master);

	int status = setAddress();
	if (status != 0) {
		cout << "error getting address";
		return;
	}
	status = openSocketAndListen();
	if (status != 0) {
		cout << "error listening";
		return;
	}

}

/*
 * displays its own server-ip list
 * */
void Server::displayList() {
	displayList(oServerIpList);
}

/*
 * displays the specified server-ip list
 * */
void Server::displayList(std::multiset<sock_info, comp> oList) {
	if (oList.size() == 0) {
		cout << "!!The list is currently empty!!" << endl;
		return;
	}

	std::multiset<sock_info, comp>::iterator it = oList.begin();

	int i = 0;
	cout << "ID\tHost Name\t\t\tIP Address\tPort No" << endl;

	while (it != oList.end()) {
		struct sock_info oTemp = *(it++);

		cout << ++i << "\t" << oTemp.host_name << "\t";
		if (strlen(oTemp.host_name) < 24)
			cout << "\t";
		cout << oTemp.ip_address << "\t" << oTemp.port_no << "\t\t" << endl;
	}
}

/*
 * server waits for user input and
 * listens to connection requests
 * */
void Server::establishConnection() {
	cout << "server: waiting for connections...\n";
	strMyIpAddress = getMyIpAddress();
	while (1) {
		read_fds = master;
		cout << strDisplayMsg;
		cout.flush();
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}
		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i,&read_fds)) {
				if (i == STDIN) {
					//user enters a key
					userInput();
				} else {
					cout << string(strlen(strDisplayMsg), '\b');
					cout << string(strlen(strDisplayMsg), ' ') << endl;
					if (i == sockfd) {
						int new_fd = acceptClient(oServerIpList);
						sendServerIpList();
					} else if (isClient(i)) {
						char buf[MAXDATASIZE];
						if (getClientMessage(i, buf) != 0) {
							sendServerIpList();
						} else {
							//this is an unexpected case- won't reach here
							cout << "Unknown client said something!!" << endl;
						}
					}
				}
			}
		}
	}
}

/*
 * Sends server-ip list to it's clients
 * used only by the Server when a user leaves or a new user enters
 * */
void Server::sendServerIpList() {
	std::multiset<sock_info, comp>::iterator it = oServerIpList.begin();
	while (it != oServerIpList.end()) {
		struct sock_info oReciever = *(it++);

		std::multiset<sock_info, comp>::iterator it_2 = oServerIpList.begin();
		while (it_2 != oServerIpList.end()) {

			struct sock_info oTemp = *(it_2++);
			if (oReciever.sock_fd != oTemp.sock_fd) {

				char buffer[sizeof(oTemp)];
				memcpy(buffer, &oTemp, sizeof(sock_info));
				sendMessage(oReciever.sock_fd, buffer);
				//exit(0);
			}
		}
		sendMessage(oReciever.sock_fd, "EOF");
	}
}

/*
 * used by the peers own server to handle request for a download
 * @params id of the receiving client and the file name
 * */
void Server::requestDownload(int id, string strFileName) {
	if (id >= oServerIpList.size()) {
		cout << "Host not found!" << endl;
		return;
	}
	string message = "RequestingFile " + strFileName;
	char *buffer = (char*) message.c_str();
	int clientSockFd = getSocketById(id);

	file_info oFileInfo;
	oFileInfo.name = strFileName;
	oFileInfo.bDownloadStarted = false;
	oDownload[clientSockFd] = oFileInfo;


	sendMessage(clientSockFd, buffer);

}

/*
 * handle incoming files by a client
 * socket id through which the message came
 * */
void Server::download(int clientSockFd) {
	char buffer[MAXDATASIZE];
	if(getClientMessage(clientSockFd, buffer)!=0)
		return;

	std::map<int, file_info>::iterator it = oDownload.find(clientSockFd);
	file_info oFileInfo = (*it).second;

	if (oFileInfo.bDownloadStarted) {
		if (strcmp(buffer, "EOF") == 0) {
			oFileInfo.bDownloadStarted = false;
			timeval endTime;

			gettimeofday(&(endTime), NULL);
			int timeTaken = (endTime.tv_sec - oFileInfo.startTime.tv_sec)
					* 1000000 + (endTime.tv_usec - oFileInfo.startTime.tv_usec);
			float timeTakenSec = float(timeTaken) / 1000000.0;
			float downloadRate = float(oFileInfo.size * 8)
					/ float(timeTakenSec);
			cout << string(strlen(strDisplayMsg), ' ') << endl;
			cout << "\nFinished downloading " << oFileInfo.name << endl;
			string strHostName = getHostNameBySocket(clientSockFd);
			cout << "Rx (" << oMySockInfo.host_name << "): " << strHostName
					<< " -> " << oMySockInfo.host_name << ", File Size: "
					<< oFileInfo.size << " Bytes, Time Taken: " << timeTakenSec
					<< " seconds, Rx Rate: " << int(downloadRate)
					<< " bits/second\n\n";

			close(oFileInfo.fileDesc);
		} else {

			int byteRemaining = oFileInfo.size - oFileInfo.transfered;
			oFileInfo.transfered = oFileInfo.transfered + MAXDATASIZE;
			int err = write(oFileInfo.fileDesc, buffer,
					MAXDATASIZE < byteRemaining ? MAXDATASIZE : byteRemaining);
		}
	} else {
		std::vector<std::string> arrSplit = splitString(buffer, " ");
		/*
		 * format of file
		 * 1- type of information (SendingFile)
		 * 2- name of file
		 * 3- size of file
		 * */
		if (arrSplit[0].compare("SendingFile") == 0) {
			oFileInfo.transfered = 0;
			oFileInfo.name = arrSplit[1];
			oFileInfo.size = atoi(arrSplit[2].c_str());
			oFileInfo.bDownloadStarted = true;
			oFileInfo.fileChar = "";
			gettimeofday(&(oFileInfo.startTime), NULL);
			cout << string(strlen(strDisplayMsg), ' ') << endl;
			cout << "\n\nStarted downloading " << oFileInfo.name << endl;

			oFileInfo.fileDesc = open((char*) oFileInfo.name.c_str(),
					O_CREAT | O_WRONLY, 0664);
		}

		if (arrSplit[0].compare("RequestingFile") == 0) {
			upload(clientSockFd, arrSplit[1], true);
		}

		if (arrSplit[0].compare("Error") == 0) {
			cout << string(strlen(strDisplayMsg), ' ') << endl;
			cout << "Unable to fetch file: " << oFileInfo.name << endl;
		}
	}
	oDownload[clientSockFd] = oFileInfo;
}

/*
 *writes file to the system based on filename
 *and contents recieved through file_info
 * */
void Server::writeFile(file_info oFileInfo) {
	ofstream f;
	f.open(oFileInfo.name.c_str(), ios::out);
	f.write(oFileInfo.fileChar.c_str(), oFileInfo.fileChar.length());
	f << flush;
	f.close();
}

/*
 * sends file data in chunks of data of 256 bytes
 * to the client at specified socket
 * */
void Server::upload(int clientSockFd, string strFileLoc, bool wasRequested) {
	ifstream fin(strFileLoc.c_str());
	if (fin.is_open()) {
		cout << "Please wait while the file is being uploaded...";
		cout.flush();
		vector<string> arrSplit = splitString(strFileLoc, "/");
		string strFileName = arrSplit[arrSplit.size() - 1];
		fin.seekg(0, std::ifstream::end);
		int fileSize = fin.tellg();
		string strMessage = "SendingFile " + strFileName + " "
				+ intToString(fileSize);
		fin.seekg(0, std::ios::beg);

		sendMessage(clientSockFd, (char *) strMessage.c_str());
		int position = 0;
		char buffer[MAXDATASIZE - 1];

		timeval startTime;

		gettimeofday(&(startTime), NULL);
		while (!fin.eof()) {
			fin.get(buffer[position++]);
			if (position == MAXDATASIZE) {
				position = 0;
				sendMessage(clientSockFd, buffer);
				memset(buffer, 0, MAXDATASIZE - 1);

			}
		}
		if (position != MAXDATASIZE) {
			buffer[position] = '\0';

			sendMessage(clientSockFd, buffer);
		}

		timeval endTime;

		gettimeofday(&(endTime), NULL);
		int timeTaken = (endTime.tv_sec - startTime.tv_sec) * 1000000
				+ (endTime.tv_usec - startTime.tv_usec);
		float timeTakenSec = float(timeTaken) / 1000000.0;
		float downloadRate = float(fileSize * 8) / float(timeTakenSec);
		if (wasRequested) {
			cout << string(strlen(strDisplayMsg), ' ') << endl;
		}
		string strHostName = getHostNameBySocket(clientSockFd);
		cout << "\nFinished uploading " << strFileName << endl;
		cout << "Tx (" << oMySockInfo.host_name << "): "
				<< oMySockInfo.host_name << " -> " << strHostName
				<< ", File Size: " << fileSize << " Bytes, Time Taken: "
				<< timeTakenSec << " seconds, Tx Rate: " << int(downloadRate)
				<< " bits/second\n\n";

		sendMessage(clientSockFd, "EOF");
	} else {
		if (wasRequested) {
			char *message = "Error";
			sendMessage(clientSockFd, message);
		} else {
			cout << "File could not be opened." << endl;
		}
	}
}

/*
 * processes request from the user to upload
 * file to the specified client
 * */
void Server::upload(int id, string strFileName) {
	if (id >= oServerIpList.size()) {
		cout << "Host not found!" << endl;
		return;
	}
	upload(getSocketById(id), strFileName, false);
}

/*
 * sends message through specified socket
 * */
void Server::sendMessage(int current_fd, char* message) {
//	cout << "sending:" << current_fd << endl;
	if (send(current_fd, message, MAXDATASIZE, 0) == -1)
		perror("send");
}

/*
 * recieves message through specified socket
 * */
int Server::getClientMessage(int i, char *buf) {
	int numbytes;
	if ((numbytes = recv(i, buf, MAXDATASIZE, 0)) == -1) {
		perror("recv");
		closeConnection(i);
		return -1;
	} else {
		if (numbytes == 0) {
			closeConnection(i);
			return 1;
		}
		buf[numbytes] = '\0';
		return 0;
	}
}

/*
 * adds client to its own server-ip list
 * and to master to start listening from the socket
 * */
void Server::addClient(struct sock_info oSockInfo) {
	oServerIpList.insert(oSockInfo);
	file_info oFileInfo;
	oFileInfo.bDownloadStarted = false;
	oDownload[oSockInfo.sock_fd] = oFileInfo;

	FD_SET(oSockInfo.sock_fd, &master);
	fdmax = fdmax < oSockInfo.sock_fd ? oSockInfo.sock_fd : fdmax;
}

/*
 * receives all the information, creates sock_info
 * object and calls addclient to add the client to list
 * */
void Server::addClient(int new_fd, char* ip, int clientPort, char *hostName) {
	struct sock_info oSockInfo;
	strcpy(oSockInfo.host_name, hostName);
	strcpy(oSockInfo.ip_address, ip);
	//oSockInfo.host_name[strlen(hostName)] = '\0';
	oSockInfo.port_no = clientPort;
	oSockInfo.sock_fd = new_fd;
	addClient(oSockInfo);
	if (isServer) {
		cout << "A new Peer has entered the network:" << endl;
	} else {
		cout << string(strlen(strDisplayMsg), ' ') << endl;
		cout << "\n\nYou have a new connection:" << endl;
	}
	displayList();
	cout << "\n\n";
}

/*
 * searches for client by specified ID and
 * returns the sock_info of the client
 * */
std::multiset<sock_info, comp>::iterator Server::getClientById(int id) {
	std::multiset<sock_info, comp>::iterator it = oServerIpList.begin();
	int i = 0;
	while (it++ != oServerIpList.end()) {
		if (i++ == id)
			return it;
	}
	return oServerIpList.end();
}

/*
 * terminates and removes client on user request
 * */
void Server::removeClient(int id) {

	if (id >= oServerIpList.size()) {
		cout << "Please specify a valid ID!!" << endl;
		return;
	}
	int clientSocket = getSocketById(id);
	deleteFromSets(id);
	FD_CLR(clientSocket, &master);
	cout << "\nConnection to host " << (id + 1) << "  terminated.\n";
	close(clientSocket);
	displayList();
	cout << "\n\n";
}

/*
 * terminates connection when the client on the other end terminates
 * */
void Server::closeConnection(int clientSockFd) {
	int id = getIdBySocket(clientSockFd);
	if (id == -1)
		return;
	if (id == 0 && !isServer)
		return;
	deleteFromSets(id);
	FD_CLR(clientSockFd, &master);

	close(clientSockFd);

	std::map<int, file_info>::iterator it = oDownload.find(clientSockFd);
	file_info oFileInfo = (*it).second;

	if (oFileInfo.bDownloadStarted) {
		cout << "Download of " << oFileInfo.name << " terminated by "
				<< (id + 1) << endl;
	}
	cout << string(strlen(strDisplayMsg), ' ') << endl;
	cout << "Connection terminated by " << (id + 1) << "\n";
	displayList();
	cout << "\n\n";
}

/*
 * Removes specified user from the server-ip list
 * */
void Server::deleteFromSets(int id) {
	std::multiset<sock_info, comp>::iterator it = oServerIpList.begin();
	int i = 0;
	while (it != oServerIpList.end()) {
		if (i == id) {
			oServerIpList.erase(it);
			return;
		}
		it++;
		i++;
	}
}

/*
 * Returns ID of a client based on the socket number
 * */
int Server::getIdBySocket(int clientSockFd) {
	int i = 0;
	std::multiset<sock_info, comp>::iterator it = oServerIpList.begin();
	while (it != oServerIpList.end()) {
		if (clientSockFd == (*it).sock_fd) {
			return i;
		}
		i++;
		it++;
	}
	return -1;
}

/*
 * Returns socket number of a client based on the ID
 * */
int Server::getSocketById(int id) {
	int i = 0;
	std::multiset<sock_info, comp>::iterator it = oServerIpList.begin();
	while (it != oServerIpList.end()) {
		if (id == i++) {
			return (*it).sock_fd;
		}
		it++;
	}

	return -1;
}

string Server::getHostNameBySocket(int clientSockFd) {
	std::multiset<sock_info, comp>::iterator it = oServerIpList.begin();
	while (it != oServerIpList.end()) {
		if (clientSockFd == (*it).sock_fd) {
			string strHostName = it->host_name;
			return strHostName;
		}
		it++;
	}

	return "";
}

/*
 * Checks if a client is present in the network
 * based on the server-ip list sent by the server
 * */
bool Server::validateClient(multiset<sock_info, comp> oAvailList,
		char *clientIp, int portNo) {
	std::multiset<sock_info, comp>::iterator it = oAvailList.begin();
	it++;
	while (it != oAvailList.end()) {
		struct sock_info oTemp = *(it++);
		if (strcmp(clientIp, oTemp.host_name) == 0
				|| strcmp(clientIp, oTemp.ip_address) == 0) {
			if (portNo == oTemp.port_no)
				return true;
		}
	}
	return false;
}

/*
 * checks if a specified port is in its own server-ip list
 * */
bool Server::isClient(int clientSockFd) {
	std::multiset<sock_info, comp>::iterator it = oServerIpList.begin();

	while (it != oServerIpList.end()) {
		if (clientSockFd == (*it).sock_fd) {
			return true;
		}
		it++;
	}
	return false;
}

/*
 * accepts connection request from a client
 * */
int Server::acceptClient(std::multiset<sock_info, comp> oClientList) {

	char s[INET_ADDRSTRLEN];
	int new_fd;
	socklen_t sin_size;
	struct sockaddr_storage their_addr;
	sin_size = sizeof their_addr;

	new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);

	sockaddr* sa = (sockaddr *) &their_addr;
	inet_ntop(their_addr.ss_family, &(((struct sockaddr_in*) sa)->sin_addr), s,
			sizeof s);

	if (new_fd != -1) {
		char buff[MAXDATASIZE];

		getClientMessage(new_fd, buff);
		vector<string> arrSplit = splitString(buff, " ");
		string strClientPort = buff;

		int port = atoi(arrSplit[0].c_str());
		if (isServer) {

			sendMessage(new_fd, "CONN_ACCEPTED");
		} else {
			if (oServerIpList.size() <= MAXCLIENTCONNECTION
					&& validateClient(oClientList, (char *) arrSplit[1].c_str(),
							port)) {
				sendMessage(new_fd, "CONN_ACCEPTED");
			} else {
				sendMessage(new_fd, "CONN_REJECTED");
				close(new_fd);
				return -1;
			}
		}

		addClient(new_fd, (char *) arrSplit[1].c_str(), port,
				(char *) arrSplit[2].c_str());
	}

	return new_fd;
}

/*
 * accept user input for server
 * */
int Server::userInput() {

	string uInput = "help";
	getline(cin, uInput);

	std::transform(uInput.begin(), uInput.end(), uInput.begin(), ::tolower);
	vector<string> arrSplit = splitString(uInput, " ");
	if (arrSplit.size() == 0) {
		cout << "Enter a valid input! (HELP for options)" << endl;
		return 1;
	}
	if (arrSplit[0].compare("exit") == 0) {
		cout << "bye!" << endl;
		exit(1);
	}
	if (arrSplit[0].compare("myip") == 0) {
		cout << "Your IP address is " << strMyIpAddress << endl;
		return 1;
	}
	if (arrSplit[0].compare("myport") == 0) {
		cout << "Your Port number is " << myport << endl;
		return 1;
	}

	if (arrSplit[0].compare("help") == 0) {
		displayFunctions(true, false);
		return 0;
	}
	if (arrSplit[0].compare("creator") == 0) {
		displayCreatorInfo();
		return 0;
	}
	if (arrSplit[0].compare("list") == 0) {
		cout << "List of hosts connected to you:" << endl;
		displayList();
		return 0;
	}
	if (arrSplit[0].compare("clear") == 0) {
		system("clear");
		return 0;
	}
	if (arrSplit[0].compare("sendlist") == 0) {
		sendServerIpList();
		return 0;
	}

	cout << "please enter a valid command; enter HELP for list of commands"
			<< endl;

	return 0;
}

/*
 * sets the addressinfo for listening
 * */
int Server::setAddress() {
	struct addrinfo hints, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	string sPort = intToString(myport);
	char *portChar = (char*) sPort.c_str();

	int status = getaddrinfo(NULL, portChar, &hints, &res);

	return status;
}

/*
 * Creates a socket and starts listening for connection requests
 * */
int Server::openSocketAndListen() {
	struct addrinfo *p;
	int yes = 1;

	for (p = res; p != NULL; p = res->ai_next) {
		if ((sockfd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			perror("setsockopt");
			exit(1);
		}

		if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			cout << strerror(errno) << ". Choose a different Port Number!"
					<< endl;
			exit(0);
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(res);
	int status = listen(sockfd, BACKLOG);

	FD_SET(sockfd, &master);
	fdmax = fdmax < sockfd ? sockfd : fdmax;

	return status;
}
