#include "../incs/Channel.hpp"
#include "../incs/Server.hpp"
#include "../incs/Client.hpp"

Channel::Channel() {}

Channel::Channel(std::string name) : _Name(name), _inviteOnly(false), _userLimit(0) {}

Channel::~Channel() {}
