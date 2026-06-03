#pragma once

#include <string>

class Client 
{
	private:
		std::string	pass;
		std::string	nick;
		std::string	user;
		int			fd;
		bool		passOk;
		bool		nickOk;
		bool		userOk;
		std::string	buffer;

	public:
		Client(int );
		Client();
		~Client();

		//les getters
		std::string getNick() const;
		std::string getPass() const;
		std::string getUser() const;
		int			getFd() const;
		std::string getBuffer() const;
 		bool		isPassOk()  const;
		bool		isNickOk()  const;
		bool		isUserOk()  const;
		bool		isRegistered() const;

		//les setters
		void setPass(const std::string &p);
		void setNick(const std::string &n);
		void setUser(const std::string &u);
		void setPassOk(bool b);
		void setNickOk(bool b);
		void setUserOk(bool b);

		//parsing line
		void appendBuffer(const std::string &data);
		void clearBuffer();
		void trimBuffer(size_t n);

};