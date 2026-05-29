#pragma once

#include <string>
#include <cctype>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <vector>

class Server
{
	private:
		int port;
		std::string password;
		int server_fd;
	public:
		Server(int , std::string );
		~Server();
		void init();
		void run();
};