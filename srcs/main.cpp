#include "../incs/Server.hpp"


bool parse_first_command(int argc, char **argv)
{
	if (argc != 3 || argv[1][0] == '\0' || argv[2][0] == '\0')
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return false;
	}
	for (size_t i = 0; i < strlen(argv[1]); i++)
	{
		if (i == 0 && (argv[1][i] == '-' || argv[1][i] == '+'))
			continue;
		if (!std::isdigit(argv[1][i]))
		{
			std::cerr << "Error: port must be numeric" << std::endl;
			return false;
		}
	}
	int port = atoi(argv[1]);
	if (port < 1024 || port > 65535)
	{
		std::cerr << "Error: port must be between 1024 and 65535" << std::endl;
		return false;
	}
	return true;
}
int main(int argc, char **argv)
{
	if (!parse_first_command(argc, argv))
		return (1);
	int _port = atoi(argv[1]);
	std::string _password =  argv[2];
	Server server(_port, _password);
	try
	{
		server.init();
		server.run();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (1);
	}
}