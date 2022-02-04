#include "UserProperties.h"

void UserProperties::PrintProperties() {
	std::cout << name << " ID: " << currentSocketID << " Room: " << roomPair.first << ", " << roomPair.second;
}