#include "Client.hpp"

Client::Client(int _fd)
{
	this->fd = _fd;
}

Client::~Client() {}