#pragma once

#include <string>

class Client 
{
private:
	std::string pass;
	std::string nick;
	std::string user;
	int fd;
public:
	Client(int );
	~Client();

};