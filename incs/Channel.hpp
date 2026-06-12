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
	bool	_inviteOnly;
	bool	topicProtected;
	int		_userLimit;

	std::map<int, Client*> members;
	std::map<int, Client*> operators;
	std::map<int, Client*> invited;

	public:
		Channel();
		Channel(std::string name);
		~Channel();

		const std::string &getName() const;
		const std::string &getKey() const ;
		const std::string &getTopic() const;
		bool	isInviteOnly() const ;
		bool	isTopicProtected()const ;
        int		getUserLimit()const ;

		void	setKey(const std::string &key);
		void	setTopic(const std::string &topic);
		void	setInviteOnly(bool value);
		void	setTopicProtected(bool val);
		void	setUserLimit(int limit);

		void	addMember(Client *client);
		void	removeMember(int fd);
		bool	isMember(int fd) const;
		bool	isEmpty() const;
		int		getMemberCount() const;
		const	std::map<int, Client*> &getMembers() const;

		void	addOperator(Client *client); //+o 
		void	removeOperator(int fd); //-o
		bool	isOperator(int fd) const;

		void	addInvited(Client *client);
		bool	isInvited(int fd) const;

		std::string	getMemberList() const;
		void		broadcast(const std::string &msg, int excludeFd = -1); //envoye un mess a tout les membres du channel
};
