#include "Room.h"

std::string Room::PrintRoomNameAndCap() {
	std::string roomInfo = roomName + " room, " + std::to_string(roomCapacity) + "/" + std::to_string(maxRoomCapacity) + " currently online.";

	return roomInfo;
}

std::string Room::RoomNameLower() {
	std::string lowerRoomName;
	
	for (int i = 0; i < roomName.length(); i++) {
		lowerRoomName.push_back(tolower(roomName[i]));
	}

	return lowerRoomName;
}

void Room::IncreaseCurrentCap() {
	if (!isRoomCapped || roomCapacity + 1 < maxRoomCapacity) {
		roomCapacity++;

		if (roomCapacity == maxRoomCapacity) {
			isRoomCapped = true;
		}
	}
}

void Room::DecreaseCurrentCap() {
	if (isRoomCapped || roomCapacity - 1 > -1) {
		roomCapacity--;

		if (roomCapacity < maxRoomCapacity) {
			isRoomCapped = false;
		}
	}
}