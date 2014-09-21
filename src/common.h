/*
 * common.h
 *
 *  Created on: Feb 17, 2014
 *      Author: manoj
 */

#ifndef COMMON_H_
#define COMMON_H_
#include <string>
#define MAXDATASIZE 1024
#define MAXCLIENTCONNECTION 3
#define strDisplayMsg "Enter command-> "
#include <string>
#include <vector>

std::vector<std::string> splitString(std::string s, std::string delimiters);
bool checkInteger(std::string input);
std::string getMyIpAddress();
void displayFunctions(bool bServer, bool bRegister);
void displayCreatorInfo();
std::string intToString(int number);

#endif /* COMMON_H_ */
