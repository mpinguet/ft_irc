#include "../incs/Server.hpp"

Server::Server(int _port, std::string _password)
{
	this->port = _port;
	this->password = _password;
	this->server_fd = -1;
}

Server::~Server()
{
	if (server_fd != -1)
		close(server_fd);
}

void Server::init()
{
	this->server_fd = socket(AF_INET, SOCK_STREAM, 0); // socket(AdressFromTheInternet(IPV4), TCP/IP)
	if (server_fd == -1)
		throw (std::runtime_error("socket() failed"));

	// If the server crash/been closed, the port can be reatribuate instantly
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw (std::runtime_error("setsockopt() failed"));

	// Disable the server to lock on 1 client (so it can listen on multiple clients)
	if (fcntl(server_fd, F_SETFL, O_NONBLOCK) == -1)
		throw (std::runtime_error("fcntl() failed"));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family	  = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; //listen all ip
	addr.sin_port		= htons(port);

	//connect ip + port at the socket
	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		throw (std::runtime_error("bind() failed"));
	if (listen(server_fd, SOMAXCONN) == -1) //The maximum number of pending connection
		throw std::runtime_error("listen() failed");
	std::cout << "Server listening on port " << this->port << std::endl;
}

void Server::run()
{
	std::vector<struct pollfd> fds;
	static int client_nb = 0;

	struct pollfd server_pollfd;
	server_pollfd.fd = server_fd;
	server_pollfd.events = POLLIN;
	fds.push_back(server_pollfd);
	while(true)
	{
		int ret = poll(fds.data(), fds.size(), -1);
		if (ret == -1)
			throw (std::runtime_error("poll() failed"));
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == server_fd)
					handleServerEvent(client_nb, fds);
				else
					handleClientEvent(fds, i);
			}
		}
	}
}

// ------ HANDLE SERVER EVENTS ------ \\.
void Server::handleServerEvent(int &client_nb, std::vector<struct pollfd> &fds){
	int client_fd = accept(server_fd, NULL, NULL);

	if (client_fd == -1)
	{
		std::cout << "accept() failed" << std::endl;
		return;
	}
	newClient(client_nb, fds, client_fd);
}

void Server::newClient(int &client_nb, std::vector<struct pollfd> &fds, int client_fd){
	struct pollfd client_pollfd;
	client_pollfd.fd = client_fd;
	client_pollfd.events = POLLIN;
	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	fds.push_back(client_pollfd);
	client_nb++;
	clients.insert(std::make_pair(client_fd, Client(client_fd))); // On ajoute le client à la map
	std::cout << "New client #" << client_nb << " connected on " << client_fd << " fd" << std::endl;
	sendMsg(client_fd, "Welcome! Please authenticate:\r\n");
	sendMsg(client_fd, " PASS <password>\r\n");
	sendMsg(client_fd, " NICK <nickname>\r\n");
	sendMsg(client_fd, " USER <username>\r\n");
}
// ---------------------------------- \\.

// ------ HANDLE CLIENT EVENT ------ \\.

void Server::handleClientEvent(std::vector<struct pollfd> &fds, size_t &index){
	char buff[512];
	int byte = recv(fds[index].fd, buff, sizeof(buff), 0);

	if (byte == 0)
	{
		handleDisconnection(fds, index);
		--index;
	}
	else if (byte > 0)
		handleData(buff, byte, fds, index);
	else
		std::cout << "recv() failed" << std::endl;
}
void Server::handleDisconnection(std::vector<struct pollfd> &fds, size_t index)
{
    int fd = fds[index].fd;
    Client &client = clients.find(fd)->second;

    // Prévenir tous les channels où il était + le retirer
    for (std::map<std::string, Channel>::iterator it = _Channels.begin(); it != _Channels.end(); )
    {
        if (it->second.isMember(fd))
        {
            it->second.broadcast(":" + client.getNick() + "!" + client.getUser() + "@localhost QUIT :Connection closed\r\n");
            it->second.removeMember(fd);
        }
        if (it->second.getMemberCount() == 0)
            _Channels.erase(it++);
        else
            ++it;
    }

    std::cout << "Deconnection of client #" << fd << std::endl;
    close(fd);
    clients.erase(fd);
    fds.erase(fds.begin() + index);
}

// ----------------------------------

void Server::handleData(char *buff, int byte, std::vector<struct pollfd> &fds, size_t index)
{
	buff[byte] = '\0';
	std::cout << "Received from client fd=" << fds[index].fd << ": " << buff << std::endl;

	std::map<int, Client>::iterator it = clients.find(fds[index].fd);
	Client &client = it->second;
	client.appendBuffer(buff);

	size_t pos;
	while (true)
	{
		pos = client.getBuffer().find("\r\n");
		if (pos == std::string::npos)
			pos = client.getBuffer().find("\n");
		if (pos == std::string::npos)
			break;

		std::string line = client.getBuffer().substr(0, pos);
		// +2 si \r\n, +1 si \n seul
		size_t trim = (client.getBuffer()[pos] == '\r') ? pos + 2 : pos + 1;
		client.trimBuffer(trim);
		parseCommand(client, line);
	}
}

void Server::parseCommand(Client &client, const std::string &line)
{
	if (line.empty())
		return;

	// Séparer commande et argument
	std::string cmd, arg;
	size_t space = line.find(' ');
	if (space == std::string::npos)
		cmd = line;
	else
	{
		cmd = line.substr(0, space);
		arg = line.substr(space + 1);
	}

	if (cmd == "PASS")
		handlePass(client, arg);
	else if (cmd == "NICK")
		handleNick(client, arg);
	else if (cmd == "USER")
		handleUser(client, arg);
	else if (cmd == "PRIVMSG" && client.isRegistered())
		handlePrivmsg(client, arg);
	else if (cmd == "JOIN")
		handleJoin(client, arg);
	else if (cmd == "MODE")
		handleModes(client, arg);
	else if (cmd == "PART" && client.isRegistered())
		handlePart(client, arg);
	else
	{
		if (!client.isRegistered())
			sendMsg(client.getFd(), "451 :You have not registered\r\n");
		else
			sendMsg(client.getFd(), "421 " + cmd + " :Unknown command\r\n");
	}
}

int countWords(const std::string& str) {
	if (str.empty())
		return (0);
    std::istringstream iss(str);
    std::string word;
    int count = 0;

    while (iss >> word) {
        count++;
    }
    return count;
}

int countWordPart(std::vector<std::string> &vec, const std::string &str)
{
	if (str.empty())
		return (0);
    std::istringstream iss(str);

    std::string word;
    int count = 0;

    while (iss >> word) {
		vec.push_back(word);
        count++;
    }
    return count;
}

void Server::handlePart(Client &client, const std::string &arg)
{
	std::vector<std::string> vec;
	int nbWord = countWordPart(vec, arg);
	if (nbWord == 0)
	{
		sendMsg(client.getFd(), ":ircserv 461 " + client.getNick() + " PART :Not enough parameters\r\n";)
		return ;
	}
	else if (nbWord == 1)
	{

	}
	else
	{

	}

	
}

void Server::handlePass(Client &client, const std::string &arg)
{
	if (client.isRegistered())
		return sendMsg(client.getFd(), "462 :You may not reregister\r\n");
	if (arg.empty())
		return sendMsg(client.getFd(), "461 PASS :Not enough parameters\r\n");

	if (arg == this->password)
		client.setPassOk(true);
	else
	{
		sendMsg(client.getFd(), "464 :Password incorrect\r\n");
		// On laisse trois chance ? Ou kick ?
	}
}

void Server::handleNick(Client &client, const std::string &arg)
{
	if (!client.isPassOk())
		return sendMsg(client.getFd(), "464 :Password required\r\n");
	if (arg.empty())
		return sendMsg(client.getFd(), "431 :No nickname given\r\n");

	for (size_t i = 0; i < arg.size(); i++)
	{
		if (!isalnum(arg[i]) && arg[i] != '-' && arg[i] != '_')
			return sendMsg(client.getFd(), "432 " + arg + " :Erroneous nickname\r\n");
	} // Caractere valides

	std::string nick = arg.substr(0, 9);

	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		if (it->second.getNick() == nick && it->second.getFd() != client.getFd())
			return sendMsg(client.getFd(), "433 " + nick + " :Nickname is already in use\r\n");
	} // check for unique nickname

	client.setNick(nick);
	client.setNickOk(true);

	if (client.isRegistered())
		sendWelcome(client);
}
//ERR_ERRONEUSNICKNAME (432), ERR_NICKNAMEINUSE (433), ERR_NICKCOLLISION (436)


void Server::handleUser(Client &client, const std::string &arg)
{
	if (!client.isPassOk())
		return sendMsg(client.getFd(), "464 :Password required\r\n");
		
	int param = countWords(arg);
	if (arg.empty() || param != 4)
		return sendMsg(client.getFd(), "461 USER :Not enough parameters\r\n");

	std::string username = arg.substr(0, arg.find(' '));
	client.setUser(username);
	client.setUserOk(true);

	if (client.isRegistered())
		sendWelcome(client);
}

int find_client(std::map<int, Client> &clients, std::string &name)
{
	std::map<int, Client>::const_iterator it;
	for (it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->second.getNick() == name)
			return (it->first);
	}
	return (-1);
}

void Server::handlePrivmsg(Client &client, const std::string &arg)
{
	size_t pos = 0;
	std::istringstream iss(arg);
	std::string name;
	iss >> name;
	int client_nb = find_client(clients, name);
	if (client_nb < 0)
	{
		std::string err_name = ":ircserv 401 " + client.getNick() + " " + name + " :No such nick/channel\n";
		sendMsg(client.getFd(), err_name);
		return ;
	}
	for(int i = name.size(); arg[i] != ':'; i++)
	{
		if (arg[i + 1] == ':')
			pos = i + 1;
		if (arg[i] == 32 || arg[i] == 9)
			continue;
		else
		{
			std::string err_text = ":ircserv 412 " + client.getNick() + " :No text to send\n";
			sendMsg(client.getFd(), err_text);
			return ;
		}
	}
	std::string mess;
	for (pos += 1; pos < arg.size(); pos++)
		mess += arg[pos];
	
	std::map<int, Client>::iterator it = clients.find(client_nb);
	Client& target = it->second;

	sendMsg(target.getFd(), ":" + client.getNick() + "!" + client.getUser() + "@localhost PRIVMSG " + name + " :" + mess + "\r\n");
	return ;
}

void Server::handleJoin(Client& client, const std::string& name)
{
	if(!client.isRegistered())
	{
		sendMsg(client.getFd(), "451 :You have not registered\r\n");
		return;
	}

	std::string channelName = name;

	std::string key;
	size_t space = name.find(' ');

	if (space == std::string::npos) //verifie si mdp for channel +k et parse
		channelName = name;
	else
	{
		channelName = name.substr(0, space);
		key = name.substr(space + 1);
	}

	if (channelName[0] != '#'){
		sendMsg(client.getFd(), "ERROR: channel name must start with '#'\r\n");
		return;
	}

	if (_Channels.find(channelName) == _Channels.end())
	{
		_Channels[channelName] = Channel(channelName);
		_Channels[channelName].addOperator(&clients.find(client.getFd())->second);
		std::cout << "MESS DEBUG: CHANNEL CREATED" << std::endl;
	}

	Channel &channel = _Channels[channelName];

	//evite de mettre deux fois la meme personne 
	if (channel.isMember(client.getFd()))
		return;

	//invite only
	if (channel.isInviteOnly() && !channel.isInvited(client.getFd()))
		return sendMsg(client.getFd(), ":ircserv 473 " + client.getNick() + " " + channelName + " :Cannot join channel (+i)\r\n");

	//key protected
	if (!channel.getKey().empty() && channel.getKey() != key)
		return sendMsg(client.getFd(), ":ircserv 475 " + client.getNick() + " " + channelName + " :Cannot join channel (+k)\r\n");

	//user limit
	if (channel.getUserLimit() != -1 && channel.getMemberCount() >= channel.getUserLimit())
		return sendMsg(client.getFd(), ":ircserv 471 " + client.getNick() + " " + channelName + " :Cannot join channel (+l)\r\n");

	channel.addMember(&clients.find(client.getFd())->second);

	std::string joinMsg = ":" + client.getNick() + "!" + client.getUser() + "@localhost JOIN " + channelName + "\r\n";
	channel.broadcast(joinMsg);

	//mess
	sendMsg(client.getFd(), ":ircserv 353 " + client.getNick() + " = " + channelName + " :" + channel.getMemberList() + "\r\n");
	sendMsg(client.getFd(), ":ircserv 366 " + client.getNick() + " " + channelName + " :End of /NAMES list\r\n");

}

void Server::handleModes(Client& client, const std::string& arg)
{
	size_t space = arg.find(' ');
	if (space == std::string::npos)
		return sendMsg(client.getFd(), "461 USER :Not enough parameters\r\n"); // a verif l'erreur exact pour ce cas

	std::string channelName = arg.substr(0, space);
	std::string rest = arg.substr(space + 1);

	size_t space2 = rest.find(' ');
	std::string mode = rest.substr(0, space2);

	std::string third;
	if (space2 == std::string::npos)
		third = "";
	else
		third = rest.substr(space2 + 1);

	if(mode.size() < 2)
		return sendMsg(client.getFd(), "VOIR CODE ERREUR\r\n");  // a voir
	
	if (_Channels.find(channelName) == _Channels.end())
		return sendMsg(client.getFd(), "403 :No such channel\r\n"); // same

	Channel &channel = _Channels[channelName];

	if(!channel.isOperator(client.getFd()))
		return sendMsg(client.getFd(), "482 :User is not an Administrator\r\n"); //482 ERR_CHANOPRIVSNEEDED

	char sign = mode[0];
	char action = mode[1];

	if(action == 'i') //invite
	{
		if (sign == '+')
			channel.setInviteOnly(true);
		else
			channel.setInviteOnly(false);
		channel.broadcast(":ircserv MODE " + channelName + " " + mode + "\r\n");
	}
	else if(action == 't') //topic
	{
		if (sign == '+')
			channel.setTopicProtected(true);
		else
			channel.setTopicProtected(false);
		channel.broadcast(":ircserv MODE " + channelName + " " + mode + "\r\n");
	}
	else if(action == 'k') //password
	{
		if (sign == '+')
		{
			if (third.empty())
				return sendMsg(client.getFd(), ":ircserv 461 MODE :Not enough parameters\r\n");
			channel.setKey(third);
			channel.broadcast(":ircserv MODE " + channelName + " " + mode + " " + third + "\r\n");
		}
		else
		{
			channel.setKey("");
			channel.broadcast(":ircserv MODE " + channelName + " " + mode + "\r\n");
		}
	}
	else if(action == 'o') //give or take channel priviledge
	{
		if (third.empty())
			return sendMsg(client.getFd(), ":ircserv 461 MODE :Not enough parameters\r\n");

		Client *target = NULL;
		for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if (it->second.getNick() == third)
			{
				target = &it->second;
				break;
			}
		}

		if (!target)
			return sendMsg(client.getFd(), ":ircserv 401 " + client.getNick() + " " + third + " :No such nick\r\n");
		
		if (!channel.isMember(target->getFd()))
			return sendMsg(client.getFd(), ":ircserv 441 " + third + " " + channelName + " :They aren't on that channel\r\n");

		if (sign == '+')
			channel.addOperator(target);
		else
			channel.removeOperator(target->getFd());

		channel.broadcast(":ircserv MODE " + channelName + " " + mode + " " + third + "\r\n");
	}
	else if(action == 'l') //user limit
	{
		if (sign == '+')
		{
			if (third.empty())
				return sendMsg(client.getFd(), ":ircserv 461 MODE :Not enough parameters\r\n");
			channel.setUserLimit(atoi(third.c_str()));
			channel.broadcast(":ircserv MODE " + channelName + " " + mode + " " + third + "\r\n");
		}
		else
		{
			channel.setUserLimit(-1);
			channel.broadcast(":ircserv MODE " + channelName + " " + mode + "\r\n");
		}
	}
	else 
		sendMsg(client.getFd(), "472 :Unknown Mode\r\n"); //472 ERR_UNKNOWNMODE

}

//TEST DONE :
//Can add administrator role and take it out
//only administrator can MODE
//USER limit and invite only work, invite not sent but cannot join if the channel is invite only
//Password works too



//MODE
//i t k o l avec + et - a chaque fois donc 10 retour a faire.
// a verifier MODE marque les options maybe done with hexchat

void Server::sendWelcome(Client &client)
{
	std::string nick = client.getNick();
	sendMsg(client.getFd(), ":ircserv 001 " + nick + " :Welcome to the IRC server " + nick + "\r\n");
	sendMsg(client.getFd(), ":ircserv 002 " + nick + " :Your host is ircserv\r\n");
	sendMsg(client.getFd(), ":ircserv 003 " + nick + " :This server was created today\r\n");
}

void Server::sendMsg(int fd, const std::string &msg)
{
	send(fd, msg.c_str(), msg.size(), 0);
}


//parsing channel name 
//mode only for ops
// channel only created zithout mdp and need to mode after to add pass


