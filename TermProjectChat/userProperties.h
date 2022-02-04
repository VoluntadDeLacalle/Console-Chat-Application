#pragma once
#include <string>
#include <iostream>

class UserProperties
{
public:
	std::string name;
	int currentSocketID;
	std::pair<bool, std::string> roomPair;

	UserProperties() {
		name = "";
		currentSocketID = -1;
		std::pair<bool, std::string> temp(false, "");
	}

	UserProperties(std::string n, int currentID, std::pair<bool, std::string> room) {
		name = n;
		currentSocketID = currentID;
		roomPair = room;
	}

	void PrintProperties();
};