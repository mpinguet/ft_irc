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
				{
					if (handleClientEvent(fds, i))
						i--;
				}
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

bool Server::handleClientEvent(std::vector<struct pollfd> &fds, size_t &index){
	char buff[512];
	int byte = recv(fds[index].fd, buff, sizeof(buff), 0);

	if (byte == 0)
	{
		handleDisconnection(fds, index);
		return true;
	}
	else if (byte > 0)
		return (handleData(buff, byte, fds, index));
	else
		std::cout << "recv() failed" << std::endl;
	return false;
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

bool Server::handleData(char *buff, int byte, std::vector<struct pollfd> &fds, size_t index)
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
		bool quit = parseCommand(client, line, fds);
		if (quit)
			return true;
	}
	return false;
}

bool Server::parseCommand(Client &client, const std::string &line, std::vector<struct pollfd> &fds)
{
	if (line.empty())
		return false;

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
	else if (cmd == "KICK")
		handleKick(client, arg);
	else if (cmd == "TOPIC" && client.isRegistered())
		handleTopic(client, arg);
	else if (cmd == "PART" && client.isRegistered())
		handlePart(client, arg);
	else if (cmd == "INVITE" && client.isRegistered())
		handleInvite(client, arg);
	else if (cmd == "QUIT" && client.isRegistered())
	{
		handleQuit(client, arg, fds);
		return true;
	}
	else
	{
		if (!client.isRegistered())
			sendMsg(client.getFd(), "451 :You have not registered\r\n");
		else
			sendMsg(client.getFd(), "421 " + cmd + " :Unknown command\r\n");
	}
	return false;
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

void	Server::handleQuit(Client &client, const std::string &arg, std::vector<struct pollfd> &fds)
{
	std::string reason = arg.empty() ? "Leaving" : arg;
	if (reason[0] == ':')
		reason = reason.substr(1);
	std::string msg = ":" + client.getNick() + "!" + client.getUser() + "@localhost QUIT :" + reason + "\r\n";

   for (std::map<std::string, Channel>::iterator it = _Channels.begin(); it != _Channels.end(); )
    {
        if (it->second.isMember(client.getFd()))
        {
            it->second.broadcast(msg);
            it->second.removeMember(client.getFd());
        }
        if (it->second.isEmpty())
            _Channels.erase(it++);
        else
            ++it;
    }

    // Retirer de _fds
    for (size_t i = 0; i < fds.size(); i++)
    {
        if (fds[i].fd == client.getFd())
        {
            fds.erase(fds.begin() + i);
            break;
        }
    }
	close(client.getFd());
    clients.erase(client.getFd());
}

void Server::handleTopic(Client& client, const std::string &arg){
	std::istringstream iss(arg);
	std::string channelName, topic;

	iss >> channelName;
	std::getline(iss, topic);
	std::map<std::string, Channel>::iterator chIt = _Channels.find(channelName);
	if (chIt == _Channels.end())
		return (sendMsg(client.getFd(), "403 " +client.getNick() + " " + channelName + " :No such channel\r\n"));

	Channel& channel = _Channels[channelName];

	if (!channel.isMember(client.getFd()))
		return (sendMsg(client.getFd(), ":ircserv 442 " + client.getNick() + " " + channelName + " :You're not on that channel\r\n"));
	else if (channel.isTopicProtected()){
		if (!channel.isOperator(client.getFd()))
			return (sendMsg(client.getFd(), ":ircserv 482 " + client.getNick() + " " + channelName + " :You're not channel operator\r\n"));
	}
	if (topic.empty()){
		if (channel.getTopic().empty())
			return (sendMsg(client.getFd(), ":ircserv 331 " + client.getNick() + " " + channelName + " :No topic is set\r\n"));
		sendMsg(client.getFd(), ":ircserv 332 " + client.getNick() + " " + channelName + " :" + channel.getTopic() + "\r\n");
	}
	if (topic[1] == ':')
		topic = topic.substr(2);
	else
		topic = topic.substr(1);
	channel.setTopic(topic);
	channel.broadcast(":" + client.getNick() + "!" + client.getUser() + "@localhost TOPIC " + channelName + " :" + topic + "\r\n");
}

void Server::handleKick(Client &client, const std::string &arg)
{
	if (!client.isRegistered())
	{
		sendMsg(client.getFd(), "451 :You have not registered\r\n");
		return;
	}

	std::vector<std::string> vec;
	int nbWord = countWordPart(vec, arg);

	if (nbWord < 2)
	{
		sendMsg(client.getFd(), ":ircserv 461 " + client.getNick() + " KICK :Not enough parameters\r\n");
		return;
	}

	std::string channelName = vec[0];
	std::string targetNick = vec[1];

	// Channel existe ?
	std::map<std::string, Channel>::iterator chIt = _Channels.find(channelName);
	if (chIt == _Channels.end())
	{
		sendMsg(client.getFd(), ":ircserv 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
		return;
	}

	Channel &channel = chIt->second;

	// L'émetteur est membre ?
	if (!channel.isMember(client.getFd()))
	{
		sendMsg(client.getFd(), ":ircserv 442 " + client.getNick() + " " + channelName + " :You're not on that channel\r\n");
		return;
	}

	// L'émetteur est opérateur ?
	if (!channel.isOperator(client.getFd()))
	{
		sendMsg(client.getFd(), ":ircserv 482 " + client.getNick() + " " + channelName + " :You're not channel operator\r\n");
		return;
	}

	// La cible existe sur le serveur ?
	int targetFd = find_client(clients, targetNick);
	if (targetFd < 0)
	{
		sendMsg(client.getFd(), ":ircserv 401 " + client.getNick() + " " + targetNick + " :No such nick/channel\r\n");
		return;
	}

	// La cible est membre du channel ?
	if (!channel.isMember(targetFd))
	{
		sendMsg(client.getFd(), ":ircserv 441 " + client.getNick() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
		return;
	}

	// Récupérer la raison (tout ce qui suit après channel + nick)
	std::string reason = client.getNick(); // raison par défaut
	size_t firstSpace = arg.find(' ');
	size_t secondSpace = arg.find(' ', firstSpace + 1);
	if (secondSpace != std::string::npos)
	{
		std::string rest = arg.substr(secondSpace + 1);
		size_t i = 0;
		while (i < rest.size() && (rest[i] == ' ' || rest[i] == '\t'))
			i++;
		if (i < rest.size())
		{
			if (rest[i] == ':')
				reason = rest.substr(i + 1);
			else
				reason = rest.substr(i);
		}
	}

	// Broadcast du KICK à tout le monde, y compris la cible
	std::string msg = ":" + client.getNick() + "!" + client.getUser() + "@localhost KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
	channel.broadcast(msg, -1);

	// Retirer la cible du channel (membre + opérateur si applicable)
	channel.removeOperator(targetFd);
	channel.removeMember(targetFd);

	// Supprimer le channel s'il devient vide
	if (channel.isEmpty())
		_Channels.erase(channelName);
}


void Server::handlePart(Client &client, const std::string &arg)
{
    std::vector<std::string> vec;
    int nbWord = countWordPart(vec, arg);

    if (nbWord == 0)
    {
        sendMsg(client.getFd(), ":ircserv 461 " + client.getNick() + " PART :Not enough parameters\r\n");
        return;
    }

    std::string channelName = vec[0];

    // Chercher le channel
    if (_Channels.find(channelName) == _Channels.end())
    {
        sendMsg(client.getFd(), ":ircserv 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
        return;
    }

    Channel &channel = _Channels[channelName];

    // Vérifier que le client est membre
    if (!channel.isMember(client.getFd()))
    {
        sendMsg(client.getFd(), ":ircserv 442 " + client.getNick() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // Construire le message de départ
    std::string reason;
	if (nbWord > 1)
	{
		for (size_t i = 1; i < vec.size(); i++)
			reason += vec[i] + " ";
	}
	else
		reason = ":" + client.getNick();
    std::string msg = ":" + client.getNick() + "!" + client.getUser() + "@localhost PART " + channelName + " " + reason + "\r\n";

    // Envoyer à tous les membres y compris celui qui part
    channel.broadcast(msg, -1);

    // Retirer le membre
    channel.removeMember(client.getFd());

    // Supprimer le channel s'il est vide
    if (channel.isEmpty())
        _Channels.erase(channelName);
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

void Server::handleInvite(Client &client, const std::string &arg)
{
	if (!client.isRegistered())
	{
		sendMsg(client.getFd(), "451 :You have not registered\r\n");
		return;
	}

	std::vector<std::string> vec;
	int nbWord = countWordPart(vec, arg);

	if (nbWord < 2)
	{
		sendMsg(client.getFd(), ":ircserv 461 " + client.getNick() + " INVITE :Not enough parameters\r\n");
		return;
	}

	std::string targetNick = vec[0];
	std::string channelName = vec[1];

	// Channel existe ?
	std::map<std::string, Channel>::iterator chIt = _Channels.find(channelName);
	if (chIt == _Channels.end())
	{
		sendMsg(client.getFd(), ":ircserv 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
		return;
	}

	Channel &channel = chIt->second;

	// L'émetteur est membre ?
	if (!channel.isMember(client.getFd()))
	{
		sendMsg(client.getFd(), ":ircserv 442 " + client.getNick() + " " + channelName + " :You're not on that channel\r\n");
		return;
	}

	// Si +i, l'émetteur doit être opérateur
	if (channel.isInviteOnly() && !channel.isOperator(client.getFd()))
	{
		sendMsg(client.getFd(), ":ircserv 482 " + client.getNick() + " " + channelName + " :You're not channel operator\r\n");
		return;
	}

	// La cible existe sur le serveur ?
	int targetFd = find_client(clients, targetNick);
	if (targetFd < 0)
	{
		sendMsg(client.getFd(), ":ircserv 401 " + client.getNick() + " " + targetNick + " :No such nick/channel\r\n");
		return;
	}

	// La cible n'est pas déjà membre ?
	if (channel.isMember(targetFd))
	{
		sendMsg(client.getFd(), ":ircserv 443 " + client.getNick() + " " + targetNick + " " + channelName + " :is already on channel\r\n");
		return;
	}

	// Ajouter la cible aux invités
	channel.addInvited(&clients.find(targetFd)->second);

	// Confirmation à l'émetteur
	sendMsg(client.getFd(), ":ircserv 341 " + client.getNick() + " " + targetNick + " " + channelName + "\r\n");

	// Notification à la cible
	Client &target = clients.find(targetFd)->second;
	std::string msg = ":" + client.getNick() + "!" + client.getUser() + "@localhost INVITE " + target.getNick() + " :" + channelName + "\r\n";
	sendMsg(targetFd, msg);
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

void Server::handlePrivmsg(Client &client, const std::string &arg)
{
	if (arg.empty())
	{
		sendMsg(client.getFd(), ":ircserv 411 " + client.getNick() + " :No recipient given (PRIVMSG)\r\n");
		return;
	}

	// Séparer la cible (nick ou channel) du reste
	size_t space = arg.find(' ');
	std::string target = (space == std::string::npos) ? arg : arg.substr(0, space);

	if (space == std::string::npos)
	{
		sendMsg(client.getFd(), ":ircserv 412 " + client.getNick() + " :No text to send\r\n");
		return;
	}

	std::string rest = arg.substr(space + 1);

	// Sauter les espaces/tabs supplémentaires entre la cible et le texte
	size_t i = 0;
	while (i < rest.size() && (rest[i] == ' ' || rest[i] == '\t'))
		i++;

	if (i >= rest.size())
	{
		sendMsg(client.getFd(), ":ircserv 412 " + client.getNick() + " :No text to send\r\n");
		return;
	}

	// Le texte commence après ':' s'il y en a un, sinon c'est le reste tel quel
	std::string mess;
	if (rest[i] == ':')
		mess = rest.substr(i + 1);
	else
		mess = rest.substr(i);

	std::string fullMsg = ":" + client.getNick() + "!" + client.getUser() + "@localhost PRIVMSG " + target + " :" + mess + "\r\n";

	// --- Cas 1 : la cible est un channel ---
	if (!target.empty() && target[0] == '#')
	{
		std::map<std::string, Channel>::iterator chIt = _Channels.find(target);
		if (chIt == _Channels.end())
		{
			sendMsg(client.getFd(), ":ircserv 403 " + client.getNick() + " " + target + " :No such channel\r\n");
			return;
		}

		Channel &channel = chIt->second;

		if (!channel.isMember(client.getFd()))
		{
			sendMsg(client.getFd(), ":ircserv 442 " + client.getNick() + " " + target + " :You're not on that channel\r\n");
			return;
		}

		// broadcast à tous les membres sauf l'émetteur
		channel.broadcast(fullMsg, client.getFd());
		return;
	}

	// --- Cas 2 : la cible est un nick (message privé) ---
	int target_fd = find_client(clients, target);
	if (target_fd < 0)
	{
		sendMsg(client.getFd(), ":ircserv 401 " + client.getNick() + " " + target + " :No such nick/channel\r\n");
		return;
	}

	sendMsg(target_fd, fullMsg);
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
		return sendMsg(client.getFd(), ":ircserv 461 " + client.getNick() + " MODE :Not enough parameters\r\n");

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
		return sendMsg(client.getFd(), ":ircserv 472 " + client.getNick() + " " + mode + " :Unknown mode\r\n");  // a voir
	
	if (_Channels.find(channelName) == _Channels.end())
		return sendMsg(client.getFd(), ":ircserv 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");

	Channel &channel = _Channels[channelName];

	if(!channel.isOperator(client.getFd()))
		return sendMsg(client.getFd(), ":ircserv 482 " + client.getNick() + " " + channelName + " :You're not channel operator\r\n");

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
		sendMsg(client.getFd(), ":ircserv 472 :Unknown Mode\r\n"); //472 ERR_UNKNOWNMODE

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


