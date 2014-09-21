/*
 * main.cpp
 *
 *  Created on: Feb 13, 2014
 *      Author: manoj
 */
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "client.h"
#include "server.h"
#include "common.h"
using namespace std;

Client callClient(int port) {
	Client client(port);
	return client;
}

Server callServer(int port) {
	Server oServer(port, true, 4);
	oServer.establishConnection();
	return oServer;
}

int main(int argc, char* argv[]) {

	if (argc != 3) {
		cout << "Not enough arguments: proj1 [c/s] [Port Number]" << endl;
		return 1;
	}
	string pType = argv[1];
	if (!checkInteger(argv[2])) {
		cout << "enter a valid port number" << endl;
		return 0;
	}
	int port = atoi(argv[2]);
	if (port < 1024) {
		cout << "port number should not be less than 1024" << endl;
		return 0;
	}
	if (pType.compare("c") == 0) {
		Client client = callClient(port);
	} else if (pType.compare("s") == 0) {
		Server server = callServer(port);
	} else {
		cout << "please specify client or server" << endl;
	}
	return 0;
}

