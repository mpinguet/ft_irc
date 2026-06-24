#include "../incs/Client.hpp"

Client::Client(int fd) : fd(fd), passOk(false), nickOk(false), userOk(false) {}

Client::Client() {}

Client::~Client() {}

int	Client::getFd()	const { return fd; }
std::string	Client::getPass()	const { return pass; }
std::string	Client::getNick()	const { return nick; }
std::string	Client::getUser()	const { return user; }
std::string	Client::getBuffer()	const { return buffer; }
bool	Client::isPassOk() const { return passOk; }
bool	Client::isNickOk() const { return nickOk; }
bool	Client::isUserOk() const { return userOk; }
bool	Client::isRegistered() const { return passOk && nickOk && userOk; }

void	Client::setPass(const std::string &p) { pass = p; }
void	Client::setNick(const std::string &n) { nick = n; }
void	Client::setUser(const std::string &u) { user = u; }
void	Client::setPassOk(bool b) { passOk = b; }
void	Client::setNickOk(bool b) { nickOk = b; }
void	Client::setUserOk(bool b) { userOk = b; }

void	Client::appendBuffer(const std::string &data)
{
	buffer += data;
}
void	Client::clearBuffer()
{
	buffer.clear();
}

void	Client::trimBuffer(size_t n)
{
	buffer.erase(0, n);
}
