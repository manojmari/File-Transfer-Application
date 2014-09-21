/*
 * server.h
 *
 *  Created on: Feb 15, 2014
 *      Author: manoj
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <string>
#include <set>
#include <map>
#include <sys/time.h>
#include <netinet/in.h>
#include "common.h"

struct sock_info {
	char ip_address[INET_ADDRSTRLEN];
	int port_no;
	char host_name[60];
	int sock_fd;
};
struct file_info {
	std::string fileChar;
	std::string name;
	int size;
	bool bDownloadStarted;
	struct timeval startTime;
	int fileDesc;
	int transfered;
};

struct comp {
	inline bool operator()(const sock_info& left, const sock_info& right) {
		return false;
	}
};
class Server {
private:
	sock_info oMySockInfo;
	bool isServer;
	int address, myport;
	std::string strMyIpAddress;
	std::map<int, file_info> oDownload;

	static const int STDIN = 0;
	static const int BACKLOG = 10;

	struct addrinfo *res;

	void serverInit(int port, bool server, int clientCount);
	int userInput();
	void sendServerIpList();
	void closeConnection(int clientSockFd);
	std::multiset<sock_info, comp>::iterator getClientById(int id);
	int getIdBySocket(int clientSockFd);
	int getSocketById(int id);
	void deleteFromSets(int id);
	void upload(int clientSockFd, std::string strFileName, bool wasRequested);
	void writeFile(file_info oFileInfo);
	std::string getHostNameBySocket(int clientSockFd);

public:
	int sockfd;
	int fdmax;

	fd_set master; // master file descriptor list
	fd_set read_fds; // temp file descriptor list for select()

	std::multiset<sock_info, comp> oServerIpList;

	Server(sock_info oMyInfo, int clientCount);
	Server(int sock, bool server, int clientCount);
	void establishConnection();
	int setAddress();
	int openSocketAndListen();

	void removeClient(int id);
	int acceptClient(std::multiset<sock_info, comp> oClientList);
	bool isClient(int sockId);
	bool validateClient(std::multiset<sock_info, comp> oAvailList,
			char *clientIp, int portNo);

	int getClientMessage(int i, char *buf);
	void sendMessage(int current_fd, char* message);
	void addClient(struct sock_info oSockInfo);
	void addClient(int new_fd, char* ip, int clientPort, char *hostName);

	void displayList();
	void displayList(std::multiset<sock_info, comp> oList);

	void download(int clientSockFd);
	void upload(int id, std::string strFileName);
	void requestDownload(int id, std::string strFileName);
}
;

#endif /* SERVER_H_ */
