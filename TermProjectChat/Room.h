#pragma once
#include <string>
#include <vector>

class Room
{
public:
	std::string roomName;
	std::vector<int> currentLoggedClients;
	
	int roomCapacity;
	bool isRoomCapped;
	
	Room(std::string name, int maxRoomCap) {
		roomName = name;
		maxRoomCapacity = maxRoomCap;
		roomCapacity = 0;
		isRoomCapped = false;
	}

	std::string PrintRoomNameAndCap();
	std::string RoomNameLower();

	void IncreaseCurrentCap();
	void DecreaseCurrentCap();

private:
	int maxRoomCapacity;
};

