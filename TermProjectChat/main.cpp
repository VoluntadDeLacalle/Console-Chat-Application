#include<stdio.h>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<ws2def.h>
#include<iostream>
#include<tchar.h>
#include<stack>
#include <map>

#include "UserProperties.h"
#include "Room.h"

using namespace std;

void InitRooms(vector<Room>& rooms) {
	Room room1 = Room("Alpha", 4);
	rooms.push_back(room1);
	
	Room room2 = Room("Beta", 4);
	rooms.push_back(room2);
	
	Room room3 = Room("Gamma", 4);
	rooms.push_back(room3);
	
	Room room4 = Room("Delta", 4);
	rooms.push_back(room4);
}

int main(int argc, char* argv[])
{
	WSADATA wsa;
	SOCKET master, newSocket, clientSocket[30], s;
	struct sockaddr_in server, address;
	int maxClients = 30, activity, addrlen, valread;
	string message = "\n       ~~~~~Street Side Communications Server v1.0~~~~~\nWelcome to the program! To continue, please enter your name below!\nIf you would like to exit the program, type \"quit\" at any time!\n";

	int MAXRECV = 1024;
	fd_set readfds;
	char* buffer;
	buffer = (char*)malloc((MAXRECV + 1) * sizeof(char));

	for (int i = 0; i < maxClients; i++)
	{
		clientSocket[i] = 0;
	}

	bool quit = false;
	map<int, UserProperties> clientSocketConnections;

	vector<Room> rooms;
	InitRooms(rooms);

	cout << "\nInitialising Winsock...";
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cout << "Failed. Error Code: " << WSAGetLastError();
		exit(EXIT_FAILURE);
	}
	cout << "Initialised.\n";

	if ((master = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		cout << "Could not create socket: " << WSAGetLastError() << endl;
		exit(EXIT_FAILURE);
	}
	cout << "Socket created." << endl;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);

	if (bind(master, (struct sockaddr*) & server, sizeof(server)) == SOCKET_ERROR)
	{
		cout << "Bind failed with error code: " << WSAGetLastError() << endl;
		exit(EXIT_FAILURE);
	}
	puts("Bind done.");

	listen(master, 3);
	puts("Waiting for incoming connections...");

	addrlen = sizeof(struct sockaddr_in);

	while (!quit)
	{
		FD_ZERO(&readfds);

		FD_SET(master, &readfds);

		for (int i = 0; i < maxClients; i++)
		{
			s = clientSocket[i];
			if (s > 0)
			{
				FD_SET(s, &readfds);
			}
		}

		activity = select(0, &readfds, NULL, NULL, NULL);

		if (activity == SOCKET_ERROR)
		{
			cout << "Select call failed with error code: " << WSAGetLastError() << endl;
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(master, &readfds))
		{
			if ((newSocket = accept(master, (struct sockaddr*) & address, (int*)&addrlen)) < 0)
			{
				perror("Accept");
				exit(EXIT_FAILURE);
			}
			char ipStr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(address.sin_addr), ipStr, INET_ADDRSTRLEN);

			cout << "New connection, socket fd is " << newSocket << ", ip is: " << ipStr
				<< ", port: " << ntohs(address.sin_port) << endl;

			if (send(newSocket, message.c_str(), message.length(), 0) != strlen(message.c_str()))
			{
				perror("Send failed");
			}
			puts("Welcome message sent successfully");

			//This is where the client is initially added.
			for (int i = 0; i < maxClients; i++)
			{
				if (clientSocket[i] == 0)
				{
					clientSocket[i] = newSocket;
					cout << "Adding to list of sockets at index: " << i << endl;

					UserProperties user = UserProperties("", i, pair<bool, string>(false, ""));
					clientSocketConnections[i] = user;
					break;
				}
			}
		}

		//Checks for current Clients
		for (int i = 0; i < maxClients; i++)
		{
			s = clientSocket[i];
			if (FD_ISSET(s, &readfds))
			{
				getpeername(s, (struct sockaddr*) & address, (int*)&addrlen);

				valread = recv(s, buffer, MAXRECV, 0);
				if (valread == SOCKET_ERROR)
				{
					int errorCode = WSAGetLastError();
					if (errorCode == WSAECONNRESET)
					{
						char ipStr[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, &(address.sin_addr), ipStr, INET_ADDRSTRLEN);

						cout << "Host disconnected unexpectedly, ip " << ipStr
							<< ", port " << ntohs(address.sin_port) << endl;

						if (clientSocketConnections[i].roomPair.first != false) {
							int currentRoomIndex = -1;

							for (int j = 0; j < rooms.size(); j++) {
								if (clientSocketConnections[i].roomPair.second == rooms[j].roomName) {
									currentRoomIndex = j;
									break;
								}
							}

							string leavingMessage = "\n" + clientSocketConnections[i].name + " has disconnected.\n\n";

							clientSocketConnections[i].roomPair.first = false;
							clientSocketConnections[i].roomPair.second = "";

							rooms[currentRoomIndex].DecreaseCurrentCap();

							int indexOfClient = -1;
							for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
								if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
									indexOfClient = j;
								}
							}

							if (indexOfClient != -1) {
								rooms[currentRoomIndex].currentLoggedClients.erase(rooms[currentRoomIndex].currentLoggedClients.begin() + indexOfClient);
							}

							for (int i = 0; i < leavingMessage.length(); i++) {
								buffer[i] = leavingMessage[i];
							}
							buffer[leavingMessage.length()] = '\0';

							for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
								if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
									continue;
								}
								send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, leavingMessage.length(), 0);
							}

							for (int k = 0; k < clientSocketConnections.size(); k++) {
								if (clientSocketConnections[k].roomPair.first == false && clientSocketConnections[k].name != "") {
									if (k == i) {
										continue;
									}

									string roomListings = "\n\nHere are our current rooms and their occupancies: \n------------------------------";

									for (int q = 0; q < rooms.size(); q++) {
										roomListings += "\n" + rooms[q].PrintRoomNameAndCap() + "\n";
									}

									roomListings += "------------------------------\nEnter the name of the room you would like to join: ";

									for (int q = 0; q < roomListings.length(); q++)
									{
										buffer[q] = roomListings[q];
									}
									buffer[roomListings.length()] = '\0';

									send(clientSocket[clientSocketConnections[k].currentSocketID], buffer, roomListings.length(), 0);
								}
							}
						}

						clientSocketConnections.erase(i);
						closesocket(s);
						clientSocket[i] = 0;
					}
					else
					{
						cout << "recv failed with error code: " << errorCode;
					}
				}

				if (valread == 0)
				{
					char ipStr[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(address.sin_addr), ipStr, INET_ADDRSTRLEN);

					cout << "Host disconnected, ip " << ipStr
						<< ", port " << ntohs(address.sin_port) << endl;

					if (clientSocketConnections[i].roomPair.first != false) {
						int currentRoomIndex = -1;

						for (int j = 0; j < rooms.size(); j++) {
							if (clientSocketConnections[i].roomPair.second == rooms[j].roomName) {
								currentRoomIndex = j;
								break;
							}
						}

						string leavingMessage = "\n" + clientSocketConnections[i].name + " has disconnected.\n\n";

						clientSocketConnections[i].roomPair.first = false;
						clientSocketConnections[i].roomPair.second = "";

						rooms[currentRoomIndex].DecreaseCurrentCap();

						int indexOfClient = -1;
						for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
							if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
								indexOfClient = j;
							}
						}

						if (indexOfClient != -1) {
							rooms[currentRoomIndex].currentLoggedClients.erase(rooms[currentRoomIndex].currentLoggedClients.begin() + indexOfClient);
						}

						for (int i = 0; i < leavingMessage.length(); i++) {
							buffer[i] = leavingMessage[i];
						}
						buffer[leavingMessage.length()] = '\0';

						for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
							if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
								continue;
							}
							send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, leavingMessage.length(), 0);
						}

						for (int k = 0; k < clientSocketConnections.size(); k++) {
							if (clientSocketConnections[k].roomPair.first == false && clientSocketConnections[k].name != "") {
								if (k == i) {
									continue;
								}

								string roomListings = "\n\nHere are our current rooms and their occupancies: \n------------------------------";

								for (int q = 0; q < rooms.size(); q++) {
									roomListings += "\n" + rooms[q].PrintRoomNameAndCap() + "\n";
								}

								roomListings += "------------------------------\nEnter the name of the room you would like to join: ";

								for (int q = 0; q < roomListings.length(); q++)
								{
									buffer[q] = roomListings[q];
								}
								buffer[roomListings.length()] = '\0';

								send(clientSocket[clientSocketConnections[k].currentSocketID], buffer, roomListings.length(), 0);
							}
						}
					}

					clientSocketConnections.erase(i);
					closesocket(s);
					clientSocket[i] = 0;
					break;
				}
				else
				{
					char ipStr[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(address.sin_addr), ipStr, INET_ADDRSTRLEN);

					buffer[valread] = '\0';

					string commandCheck, outputString;
					
					for (int i = 0; i < valread; i++)
					{
						if (!(buffer[i] >= 0 && buffer[i] <= 31))
						{
							commandCheck.push_back(tolower(buffer[i]));
							outputString.push_back(buffer[i]);
						}
					}

					if (clientSocketConnections[i].name == "") {

						if (commandCheck.compare("quit") == 0 || commandCheck.compare("q") == 0) {
							cout << "\nHost disconnected, ip " << ipStr
								<< ", port " << ntohs(address.sin_port) << endl;

							if (clientSocketConnections[i].roomPair.first != false) {
								int currentRoomIndex = -1;

								for (int j = 0; j < rooms.size(); j++) {
									if (clientSocketConnections[i].roomPair.second == rooms[j].roomName) {
										currentRoomIndex = j;
										break;
									}
								}

								string leavingMessage = "\n" + clientSocketConnections[i].name + " has disconnected.\n\n";

								clientSocketConnections[i].roomPair.first = false;
								clientSocketConnections[i].roomPair.second = "";

								rooms[currentRoomIndex].DecreaseCurrentCap();

								int indexOfClient = -1;
								for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
									if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
										indexOfClient = j;
									}
								}

								if (indexOfClient != -1) {
									rooms[currentRoomIndex].currentLoggedClients.erase(rooms[currentRoomIndex].currentLoggedClients.begin() + indexOfClient);
								}

								for (int i = 0; i < leavingMessage.length(); i++) {
									buffer[i] = leavingMessage[i];
								}
								buffer[leavingMessage.length()] = '\0';

								for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
									if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
										continue;
									}
									send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, leavingMessage.length(), 0);
								}

								for (int k = 0; k < clientSocketConnections.size(); k++) {
									if (clientSocketConnections[k].roomPair.first == false && clientSocketConnections[k].name != "") {
										if (k == i) {
											continue;
										}

										string roomListings = "\n\nHere are our current rooms and their occupancies: \n------------------------------";

										for (int q = 0; q < rooms.size(); q++) {
											roomListings += "\n" + rooms[q].PrintRoomNameAndCap() + "\n";
										}

										roomListings += "------------------------------\nEnter the name of the room you would like to join: ";

										for (int q = 0; q < roomListings.length(); q++)
										{
											buffer[q] = roomListings[q];
										}
										buffer[roomListings.length()] = '\0';

										send(clientSocket[clientSocketConnections[k].currentSocketID], buffer, roomListings.length(), 0);
									}
								}
							}

							clientSocketConnections.erase(i);
							closesocket(s);
							clientSocket[i] = 0;
							break;
						}
						else if (commandCheck.compare("") == 0) {
							clientSocketConnections[i].name = "Anonymous";
						}
						else {
							clientSocketConnections[i].name = outputString;
						}

						string nameDeclaration = "\nThank you " + clientSocketConnections[i].name + ". Please select a room from our list below.";
						string roomListings = "\nHere are our current rooms and their occupancies: \n------------------------------";
						
						for (int j = 0; j < rooms.size(); j++) {
							roomListings += "\n" + rooms[j].PrintRoomNameAndCap() + "\n";
						}

						string finalBufferedString = nameDeclaration + roomListings + "------------------------------\nEnter the name of the room you would like to join: ";

						for (int i = 0; i < finalBufferedString.length(); i++)
						{
							buffer[i] = finalBufferedString[i];
						}
						buffer[finalBufferedString.length()] = '\0';

						send(s, buffer, finalBufferedString.length(), 0);
					}
					else if (clientSocketConnections[i].roomPair.first == false) {
						
						if (commandCheck.compare("quit") == 0 || commandCheck.compare("q") == 0) {
							cout << "\nHost disconnected, ip " << ipStr
								<< ", port " << ntohs(address.sin_port) << endl;

							if (clientSocketConnections[i].roomPair.first != false) {
								int currentRoomIndex = -1;

								for (int j = 0; j < rooms.size(); j++) {
									if (clientSocketConnections[i].roomPair.second == rooms[j].roomName) {
										currentRoomIndex = j;
										break;
									}
								}

								string leavingMessage = "\n" + clientSocketConnections[i].name + " has disconnected.\n\n";

								clientSocketConnections[i].roomPair.first = false;
								clientSocketConnections[i].roomPair.second = "";

								rooms[currentRoomIndex].DecreaseCurrentCap();

								int indexOfClient = -1;
								for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
									if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
										indexOfClient = j;
									}
								}

								if (indexOfClient != -1) {
									rooms[currentRoomIndex].currentLoggedClients.erase(rooms[currentRoomIndex].currentLoggedClients.begin() + indexOfClient);
								}

								for (int i = 0; i < leavingMessage.length(); i++) {
									buffer[i] = leavingMessage[i];
								}
								buffer[leavingMessage.length()] = '\0';

								for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
									if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
										continue;
									}
									send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, leavingMessage.length(), 0);
								}

								for (int k = 0; k < clientSocketConnections.size(); k++) {
									if (clientSocketConnections[k].roomPair.first == false && clientSocketConnections[k].name != "") {
										if (k == i) {
											continue;
										}

										string roomListings = "\n\nHere are our current rooms and their occupancies: \n------------------------------";

										for (int q = 0; q < rooms.size(); q++) {
											roomListings += "\n" + rooms[q].PrintRoomNameAndCap() + "\n";
										}

										roomListings += "------------------------------\nEnter the name of the room you would like to join: ";

										for (int q = 0; q < roomListings.length(); q++)
										{
											buffer[q] = roomListings[q];
										}
										buffer[roomListings.length()] = '\0';

										send(clientSocket[clientSocketConnections[k].currentSocketID], buffer, roomListings.length(), 0);
									}
								}
							}

							clientSocketConnections.erase(i);
							closesocket(s);
							clientSocket[i] = 0;
							break;
						}
						else {
							int currentRoomIndex = -1;

							for (int j = 0; j < rooms.size(); j++) {
								if (rooms[j].isRoomCapped) {
									continue;
								}

								if (commandCheck.compare(rooms[j].RoomNameLower()) == 0) {
									clientSocketConnections[i].roomPair.first = true;
									clientSocketConnections[i].roomPair.second = rooms[j].roomName;

									rooms[j].IncreaseCurrentCap();
									rooms[j].currentLoggedClients.push_back(clientSocketConnections[i].currentSocketID);
									currentRoomIndex = j;
									break;
								}
							}

							if (clientSocketConnections[i].roomPair.first != false) {
								for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
									string welcomeMessage;
									if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
										welcomeMessage = "\n" + clientSocketConnections[i].name 
											+ " has entered the chat! Type \"leave\" at any time to return to room select!\n"
											+ "Current connected users: ";

										for (int k = 0; k < rooms[currentRoomIndex].currentLoggedClients.size(); k++) {
											if (rooms[currentRoomIndex].currentLoggedClients[k] == i) {
												continue;
											}

											welcomeMessage += clientSocketConnections[rooms[currentRoomIndex].currentLoggedClients[k]].name + ", ";
										}

										if (rooms[currentRoomIndex].currentLoggedClients.size() > 1) {
											welcomeMessage.pop_back();
											welcomeMessage.pop_back();
										}

										welcomeMessage += "\n\n";
									}
									else {
										welcomeMessage = "\n" + clientSocketConnections[i].name + " has entered the chat!\n\n";
									}

									for (int k = 0; k < welcomeMessage.length(); k++) {
										buffer[k] = welcomeMessage[k];
									}
									buffer[welcomeMessage.length()] = '\0';

									send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, welcomeMessage.length(), 0);

									for (int k = 0; k < clientSocketConnections.size(); k++) {

										if (clientSocketConnections[k].roomPair.first == false && clientSocketConnections[k].name != "") {
											string roomListings = "\n\nHere are our current rooms and their occupancies: \n------------------------------";

											for (int q = 0; q < rooms.size(); q++) {
												roomListings += "\n" + rooms[q].PrintRoomNameAndCap() + "\n";
											}

											roomListings += "------------------------------\nEnter the name of the room you would like to join: ";

											for (int q = 0; q < roomListings.length(); q++)
											{
												buffer[q] = roomListings[q];
											}
											buffer[roomListings.length()] = '\0';

											send(clientSocket[clientSocketConnections[k].currentSocketID], buffer, roomListings.length(), 0);
										}
									}
								}
							}
							else {
								string roomListings = "\nHere are our current rooms and their occupancies: \n------------------------------";

								for (int j = 0; j < rooms.size(); j++) {
									roomListings += "\n" + rooms[j].PrintRoomNameAndCap() + "\n";
								}

								string finalBufferedString = roomListings + "------------------------------\nEnter the name of the room you would like to join: ";

								for (int i = 0; i < finalBufferedString.length(); i++)
								{
									buffer[i] = finalBufferedString[i];
								}
								buffer[finalBufferedString.length()] = '\0';

								send(s, buffer, finalBufferedString.length(), 0);
							}

						}
					}
					else {
						int currentRoomIndex = -1;

						for (int j = 0; j < rooms.size(); j++) {
							if (clientSocketConnections[i].roomPair.second == rooms[j].roomName) {
								currentRoomIndex = j;
								break;
							}
						}

						if (commandCheck.compare("leave") == 0) {
							string leavingMessage = "\n" + clientSocketConnections[i].name + " has disconnected.\n\n";
							
							clientSocketConnections[i].roomPair.first = false;
							clientSocketConnections[i].roomPair.second = "";

							rooms[currentRoomIndex].DecreaseCurrentCap();

							int indexOfClient = -1;
							for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
								if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
									indexOfClient = j;
								}
							}

							if (indexOfClient != -1) {
								rooms[currentRoomIndex].currentLoggedClients.erase(rooms[currentRoomIndex].currentLoggedClients.begin() + indexOfClient);
							}

							for (int i = 0; i < leavingMessage.length(); i++) {
								buffer[i] = leavingMessage[i];
							}
							buffer[leavingMessage.length()] = '\0';

							for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
								if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
									continue;
								}
								send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, leavingMessage.length(), 0);
							}

							for (int k = 0; k < clientSocketConnections.size(); k++) {
								if (clientSocketConnections[k].roomPair.first == false && clientSocketConnections[k].name != "") {
									string roomListings = "\n\nHere are our current rooms and their occupancies: \n------------------------------";

									for (int q = 0; q < rooms.size(); q++) {
										roomListings += "\n" + rooms[q].PrintRoomNameAndCap() + "\n";
									}

									roomListings += "------------------------------\nEnter the name of the room you would like to join: ";

									for (int q = 0; q < roomListings.length(); q++)
									{
										buffer[q] = roomListings[q];
									}
									buffer[roomListings.length()] = '\0';

									send(clientSocket[clientSocketConnections[k].currentSocketID], buffer, roomListings.length(), 0);
								}
							}
						}
						else if (commandCheck.compare("quit") == 0 || commandCheck.compare("q") == 0) {
							cout << "\nHost disconnected, ip " << ipStr
								<< ", port " << ntohs(address.sin_port) << endl;

							if (clientSocketConnections[i].roomPair.first != false) {
								string leavingMessage = "\n" + clientSocketConnections[i].name + " has disconnected.\n\n";

								clientSocketConnections[i].roomPair.first = false;
								clientSocketConnections[i].roomPair.second = "";

								rooms[currentRoomIndex].DecreaseCurrentCap();

								int indexOfClient = -1;
								for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
									if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
										indexOfClient = j;
									}
								}

								if (indexOfClient != -1) {
									rooms[currentRoomIndex].currentLoggedClients.erase(rooms[currentRoomIndex].currentLoggedClients.begin() + indexOfClient);
								}

								for (int i = 0; i < leavingMessage.length(); i++) {
									buffer[i] = leavingMessage[i];
								}
								buffer[leavingMessage.length()] = '\0';

								for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
									if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
										continue;
									}
									send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, leavingMessage.length(), 0);
								}

								for (int k = 0; k < clientSocketConnections.size(); k++) {
									if (clientSocketConnections[k].roomPair.first == false && clientSocketConnections[k].name != "") {
										if (k == i) {
											continue;
										}

										string roomListings = "\n\nHere are our current rooms and their occupancies: \n------------------------------";

										for (int q = 0; q < rooms.size(); q++) {
											roomListings += "\n" + rooms[q].PrintRoomNameAndCap() + "\n";
										}

										roomListings += "------------------------------\nEnter the name of the room you would like to join: ";

										for (int q = 0; q < roomListings.length(); q++)
										{
											buffer[q] = roomListings[q];
										}
										buffer[roomListings.length()] = '\0';

										send(clientSocket[clientSocketConnections[k].currentSocketID], buffer, roomListings.length(), 0);
									}
								}
							}

							clientSocketConnections.erase(i);
							closesocket(s);
							clientSocket[i] = 0;
							break;
						}
						else {
							string clientMessage = "\n" + clientSocketConnections[i].name
								+ ": " + outputString + "\n\n";

							for (int i = 0; i < clientMessage.length(); i++) {
								buffer[i] = clientMessage[i];
							}
							buffer[clientMessage.length()] = '\0';

							for (int j = 0; j < rooms[currentRoomIndex].currentLoggedClients.size(); j++) {
								if (rooms[currentRoomIndex].currentLoggedClients[j] == i) {
									continue;
								}

								send(clientSocket[rooms[currentRoomIndex].currentLoggedClients[j]], buffer, clientMessage.length(), 0);
							}
						}
					}

					cout << ipStr << ":" << ntohs(address.sin_port) << " - " << buffer << endl;
				}
			}
		}
	}

	closesocket(s);
	WSACleanup();

	return 0;
}