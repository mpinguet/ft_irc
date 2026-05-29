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
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY; //listen all ip
	addr.sin_port        = htons(port);

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
	std::cout << "New client #" << client_nb << " connected on " << client_fd << " fd" << std::endl;
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
		handleData(buff, byte, index);
	else
		std::cout << "recv() failed" << std::endl;
}

void Server::handleDisconnection(std::vector<struct pollfd> &fds, size_t index){
	std::cout << "Deconnection of client #" << fds[index].fd << std::endl;
	std::cout << "Client deleted. Total Client is now: " << fds.size() - 2 << std::endl;
	close(fds[index].fd);
	fds.erase(fds.begin() + index);
}

void Server::handleData(char *buff, int byte, int i){
	buff[byte] = '\0';
	std::cout << "Received from client #" << i << ": " << buff << std::endl;
}
// ---------------------------------- \\.

