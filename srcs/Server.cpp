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

void Server::handleDisconnection(std::vector<struct pollfd> &fds, size_t index){
	std::cout << "Deconnection of client #" << fds[index].fd << std::endl;
	std::cout << "Client deleted. Total Client is now: " << fds.size() - 2 << std::endl;
	close(fds[index].fd);
	clients.erase(fds[index].fd);
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
	else
	{
		if (!client.isRegistered())
			sendMsg(client.getFd(), "451 :You have not registered\r\n");
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
	sendMsg(client.getFd(), "NICK :" + arg + "\r\n");
}
//ERR_ERRONEUSNICKNAME (432), ERR_NICKNAMEINUSE (433), ERR_NICKCOLLISION (436)

void Server::handleUser(Client &client, const std::string &arg)
{
	if (!client.isPassOk())
		return sendMsg(client.getFd(), "464 :Password required\r\n");
	if (arg.empty())
		return sendMsg(client.getFd(), "461 USER :Not enough parameters\r\n");

	std::string username = arg.substr(0, arg.find(' '));
	client.setUser(username);
	client.setUserOk(true);

	if (client.isRegistered())
		sendWelcome(client);
}

void Server::sendWelcome(Client &client)
{
	std::string nick = client.getNick();
	sendMsg(client.getFd(), "001 " + nick + " :Welcome to the IRC server " + nick + "\r\n");
	sendMsg(client.getFd(), "002 " + nick + " :Your host is ircserv\r\n");
	sendMsg(client.getFd(), "003 " + nick + " :This server was created today\r\n");
}

void Server::sendMsg(int fd, const std::string &msg)
{
	send(fd, msg.c_str(), msg.size(), 0);
}


// est ce qu'on lance une erreur si le client envoie une commande avant de s'authentifier ? 