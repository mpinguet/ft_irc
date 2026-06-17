#pragma once

#include "Server.hpp"
#include <iostream>

class Marvin {
private:
	std::string _Name;
	std::vector<std::string> _Facts;
	std::vector<std::string> dataFacts();
public:
	Marvin();
	~Marvin();
	Marvin(const Marvin& copy);
	Marvin& operator=(const Marvin& other);
	std::vector<std::string> getFacts() const;
	std::string getName() const;
};
