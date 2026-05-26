#pragma once

#include <string>
#include <cctype>
#include <iostream>
#include <cstring>


class Server
{
private:
	int port;
	std::string password;
	int server_fd;
public:
	Server(int , std::string );
};