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
	this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		throw (std::runtime_error("socket() failed"));
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw (std::runtime_error("setsockopt() failed"));
	if (fcntl(server_fd, F_SETFL, O_NONBLOCK) == -1)
		throw (std::runtime_error("fcntl() failed"));
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(port);
	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		throw (std::runtime_error("bind() failed"));
	if (listen(server_fd, SOMAXCONN) == -1)
    	throw std::runtime_error("listen() failed");
	std::cout << "Server listening on port " << this->port << std::endl;
}

void Server::run()
{
	std::vector<struct pollfd> fds;

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
				{
					int client_fd = accept(server_fd, NULL, NULL);
					if (client_fd == -1)
    					throw std::runtime_error("accept() failed");
					struct pollfd client_pollfd;
					client_pollfd.fd = client_fd;
					client_pollfd.events = POLLIN;
					fcntl(client_fd, F_SETFL, O_NONBLOCK);
					fds.push_back(client_pollfd);
					std::cout << "New client connected: " << client_fd << std::endl;
				}
				else
				{
					char buff[512];
					int byte = recv(fds[i].fd, buff, sizeof(buff), 0);
					if (byte == 0)
					{
						std::cout << "Deconnection of client #" << fds[i].fd << std::endl;
						std::cout << "Client deleted. Total Client is now: " << fds.size() - 2 << std::endl;
						close(fds[i].fd);
						fds.erase(fds.begin() + i);
						i--;
					}
					else if (byte > 0)
					{
						buff[byte] = '\0';
						std::cout << "Received: " << buff << std::endl;
					}
					else
						throw(std::runtime_error("recv() failed"));
				}
			}
		}
	}
}