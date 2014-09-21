/*
 * common.cpp
 *
 *  Created on: Feb 17, 2014
 *      Author: manoj
 */

#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <cstdio>
#include <sstream>

using namespace std;

std::vector<std::string> splitString(std::string s, std::string delimiters) {

	std::vector<std::string> arrString;
	size_t current;
	size_t next = -1;
	int i = 0;

	do {
		current = next + 1;
		next = s.find_first_of(delimiters, current);
		string strTemp = s.substr(current, next - current);

		if (strTemp.compare("") != 0) {
			arrString.push_back(strTemp);
		}
	} while (next != std::string::npos && i < 5);
//	for (int x = 0; x < arrString.size(); x++)
//		cout << arrString[x] << endl;
	return arrString;
}

bool checkInteger(string input) {
	char * p;
	strtol(input.c_str(), &p, 10);

	return (*p == 0);
}

string getMyIpAddress() {
	char ip_address[INET_ADDRSTRLEN];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int sock_fd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo("8.8.8.8", "53", &hints, &servinfo)) != 0) {
		cout << "Error getting Google Address" << endl;
		return "";
	}
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			perror("Error getting UDP Socket!");
			continue;
		}
		if (connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock_fd);
			perror("Error getting Connect Socket!");
			continue;
		} else {
			break;
		}
	}

	if (p == NULL) {
		cout << "p=null" << endl;
		exit(0);
	}

	sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	int i = getsockname(sock_fd, (sockaddr*) &addr, &addr_len);
	if (i == -1) {
		cout << "Error getting address" << endl;
		exit(0);
	}

	const char* ip = inet_ntop(AF_INET, &addr.sin_addr, ip_address,
			sizeof ip_address);

	close(sock_fd);
	return ip_address;
}

void displayCreatorInfo() {

	cout << "\n\n/******************************************************/\n";
	cout << "\tFull Name: Manoj Mariappan" << endl;
	cout << "\tUBIT Name: manojmar" << endl;
	cout << "\tUB mail address: manojmar@buffalo.edu" << endl;
	cout << "/******************************************************/\n\n";
}

void displayFunctions(bool bServer, bool bRegister) {
	int i = 1;
	cout << endl;
	cout << "Following are the list of available commands:" << endl;
	cout << i++ << "> HELP\n";
	cout << i++ << "> MYIP\n";
	cout << i++ << "> MYPORT\n";
	if (!bServer && !bRegister) {
		cout << i++ << "> REGISTER <server IP> <port_no>\n";
	}
	if (!bServer && bRegister) {
		cout << i++ << "> CONNECT <destination> <port no>\n";
	}
	cout << i++ << "> LIST\n";
	if (!bServer && bRegister) {
		cout << i++ << "> TERMINATE <connection id>\n";
	}
	cout << i++ << "> EXIT\n";
	if (!bServer && bRegister) {
		cout << i++ << "> UPLOAD <connection id> <file name>\n";
		cout << i++
				<< "> DOWNLOAD <connection id 1> <file1> <connection id 2> <file2> <connection id 3> <file3>\n";
	}
	cout << i++ << "> CREATOR\n";

	cout << endl;
}

string intToString(int number) {
	std::stringstream ss;
	ss << number;
	return ss.str();
}
