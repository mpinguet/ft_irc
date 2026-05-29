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
	void newClient(int &client_nb, std::vector<struct pollfd> &fds, int client_fd);
	void handleServerEvent(int &client_nb, std::vector<struct pollfd> &fds);
	void handleClientEvent(std::vector<struct pollfd> &fds, size_t &index);
	void handleDisconnection(std::vector<struct pollfd> &fds, size_t index);
	void handleData(char *buff, int byte, int i);

};
