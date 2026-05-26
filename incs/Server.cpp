#include "Server.hpp"

Server::Server(int _port, std::string _password)
{
	this->port = _port;
	this->password = _password;
	this->server_fd = -1;
}