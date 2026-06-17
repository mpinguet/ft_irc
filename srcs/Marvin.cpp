#include "../incs/Marvin.hpp"

Marvin::Marvin(){
	_Name = "MARVIN";
	_Facts = dataFacts();
}

Marvin::~Marvin() {}

std::vector<std::string> Marvin::dataFacts(){
	std::vector<std::string> marvin(9);
	marvin[0] = "The idea came to him while hitchhiking across Europe in 1971, feeling broke and depressed, lying drunk in a field near Innsbruck, Austria, staring at the stars. He had with him a stolen copy of Ken Welsh's \"Hitch-hiker's Guide to Europe\".";
	marvin[1] = "Adams's obsession with towels supposedly traces back to a trip to Greece, where he kept losing his towel every morning, making his friends wait around for him — hence the running joke about how essential a towel is for any intergalactic hitchhiker.";
	marvin[2] = "By age 12, Adams was already 6 feet tall, a detail biographers often mention to illustrate how imposing his stature was from a young age.";
	marvin[3] = "His parents divorced in 1957, and he, his sister, and their mother ended up living at an RSPCA animal shelter in Essex run by his grandparents — some point to this as a possible root of the recurring theme of hidden animal intelligence in the series (dolphins, for instance).";
	marvin[4] = "In 1994, he climbed Mount Kilimanjaro dressed as a rhino to support the charity Save the Rhino International, helping raise around £100,000.";
	marvin[5] = "His funeral service in London was the first religious service ever live-streamed on the web by the BBC, following his sudden death from a heart attack in 2001.";
	marvin[6] = "Publisher Pan's 1984 press kit joked that morning prayers were held in the editorial department asking God to grant Adams the gift of inspiration along with his daily bread, so he could finally deliver the manuscript on time — he was famously notorious for missing deadlines.";
	marvin[7] = "In 1979, he was offered $50,000 for a film adaptation of the radio series but turned it down because the director wanted to make \"Star Wars with jokes\".";
	marvin[8] = "Adams himself wasn't particularly fond of the last two books in the series, \"So Long, and Thanks for All the Fish\" (1984) and \"Mostly Harmless\" (1992), and admitted he was going through a rough year while writing the latter's bleak ending.";

	return marvin;
}

std::vector<std::string> Marvin::getFacts() const{
	return _Facts;
}

std::string Marvin::getName() const{
	return _Name;
}
