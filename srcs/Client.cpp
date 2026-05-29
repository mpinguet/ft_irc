#include "Client.hpp"

Client::Client(int _fd)
{
	this->fd = _fd;
}

Client::~Client() {}

std::string Client::getNick() const { return nick; }
std::string Client::getPass() const { return pass; }
std::string Client::getUser() const { return user; }
int Client::getFd() const {return fd;}

void Client::setPass(const std::string &p) { pass = p; }
void Client::setNick(const std::string &n) { nick = n; }
void Client::setUser(const std::string &u) { user = u; }

