#pragma once

#include "Server.hpp"
#include "Client.hpp"
#include <iostream>

class Client;

class Channel {
private:
	std::string _Name;
	std::string _Key;
	std::string _Topic;
	bool _isPrivate;
	bool _inviteOnly;
	int _userLimit;

	// std::map<int, Client*> client;

	public:
	Channel();
	Channel(std::string name);
	~Channel();

	std::string getName();
	std::string getKey();
	std::string getTopic();

	std::string setName();
	std::string setKey();
	std::string setTopic();
	void setUserLimit(int limit);

	void inviteClient(Client clients);
	void kickClients();
};
