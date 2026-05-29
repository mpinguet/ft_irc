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

		std::string getNick() const;
		std::string getPass() const;
		std::string getUser() const;
		int			getFd() const;

		void	setPass(const std::string &p);
		void	setNick(const std::string &n);
		void	setUser(const std::string &u);

};