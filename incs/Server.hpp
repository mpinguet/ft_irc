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
#include <map>
#include <sstream>
#include "Client.hpp"
#include "Channel.hpp"

class Channel;

class Server
{
private:
	int port;
	std::string password;
	int server_fd;

	std::map<int, Client>	clients; //map pour stocker les clients connectés, nickname etc
	std::map<std::string, Channel> _Channels;

public:
	Server(int , std::string );
	~Server();
	void init();
	void run();

	void newClient(int &client_nb, std::vector<struct pollfd> &fds, int client_fd);
	void handleServerEvent(int &client_nb, std::vector<struct pollfd> &fds);
	void handleClientEvent(std::vector<struct pollfd> &fds, size_t &index);
	void handleDisconnection(std::vector<struct pollfd> &fds, size_t index);
	void handleData(char *buff, int byte, std::vector<struct pollfd> &fds, size_t index);
	void handleJoin(Client& client, const std::string& name);
	void handlePart(Client &client, const std::string &arg);


	// Parsing line
	void	parseCommand(Client &client, const std::string &line);
	void	handlePass(Client &client, const std::string &arg);
	void	handleNick(Client &client, const std::string &arg);
	void	handleUser(Client &client, const std::string &arg);
	void	handlePrivmsg(Client &client, const std::string &arg);


	void	sendWelcome(Client &client);
	void	sendMsg(int fd, const std::string &msg);

	//Modes
	void handleModes(Client &client, const std::string &arg);

};
