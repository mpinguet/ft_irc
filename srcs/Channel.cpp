#include "../incs/Channel.hpp"
#include "../incs/Server.hpp"
#include "../incs/Client.hpp"

Channel::Channel() : _inviteOnly(false), topicProtected(false), _userLimit(-1) {};

Channel::Channel(std::string name) : _Name(name), _inviteOnly(false), topicProtected(false), _userLimit(-1) {}

Channel::~Channel() {}

const std::string &Channel::getName() const { return _Name; }
const std::string &Channel::getKey() const { return _Key; }
const std::string &Channel::getTopic() const { return _Topic; }
bool Channel::isInviteOnly() const { return _inviteOnly; }
bool Channel::isTopicProtected() const { return topicProtected; }
int Channel::getUserLimit() const { return _userLimit; }

void Channel::setKey(const std::string &key) { _Key = key; }
void Channel::setTopic(const std::string &topic) { _Topic = topic; }
void Channel::setInviteOnly(bool value) { _inviteOnly = value; }
void Channel::setTopicProtected(bool val) { topicProtected = val; }
void Channel::setUserLimit(int limit) { _userLimit = limit; }

void Channel::addMember(Client *client)
{
	members[client->getFd()] = client;
}

void Channel::removeMember(int fd)
{
	members.erase(fd);
	operators.erase(fd);
	invited.erase(fd);
}

bool Channel::isMember(int fd) const
{
	std::map<int, Client*>::const_iterator it = members.find(fd);
	return it != members.end();
}

bool Channel::isEmpty() const
{
	return members.empty();
}

int Channel::getMemberCount() const
{
	return (int)members.size();
}

const std::map<int, Client*> &Channel::getMembers() const
{
	return members;
}

void Channel::addOperator(Client *client)
{
	operators[client->getFd()] = client;
}

void Channel::removeOperator(int fd)
{
	operators.erase(fd);
}

bool Channel::isOperator(int fd) const
{
	std::map<int, Client*>::const_iterator it = operators.find(fd);
	return it != operators.end();
}

void Channel::addInvited(Client *client)
{
	invited[client->getFd()] = client;
}

bool Channel::isInvited(int fd) const
{
	std::map<int, Client*>::const_iterator it = invited.find(fd);
	return it != invited.end();
}

std::string Channel::getMemberList() const
{
	std::string list;
	for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		if (!list.empty())
			list += " ";
		if (isOperator(it->first))
			list += "@";
		list += it->second->getNick();
	}
	return list;
}

void Channel::broadcast(const std::string &msg, int excludeFd)
{
	for (std::map<int, Client*>::iterator it = members.begin(); it != members.end(); ++it)
	{
		if (it->first != excludeFd)
			send(it->first, msg.c_str(), msg.size(), 0);
	}
}
