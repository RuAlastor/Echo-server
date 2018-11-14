#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define DEFAULT_PORT "12321"
#define DEFAULT_BUFFLEN 4096

#include <iostream>
#include <WS2tcpip.h>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")

static unsigned int currentId = 0;
bool stopper = false;

struct Client {
	unsigned int Id;
	SOCKET ClientSocket;
	sockaddr_in client_addr;
	char response[DEFAULT_BUFFLEN];
	int client_len;
	Client() {
		Id = currentId++;
		ZeroMemory(&this->client_addr.sin_zero, sizeof(this->client_addr.sin_zero));
		this->client_len = sizeof(client_addr);
	}
};
struct Params {
	Client* currentClient;
	std::vector<Client*>* allClients;
	Params(Client*& _currentClient, std::vector<Client*>* _allClients) {
		currentClient = _currentClient;
		allClients = _allClients;
	}
};

void WINAPI ConnectNewClient(void*);

int main() {

	// Initializing Socket
	WSADATA wsaData;
	int initResult;

	initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (initResult != 0) {
		std::cout << "WSAStartup() failed:" << initResult << '\n';
		system("pause");
		return 1;
	}

	// Creating Socket
	// Example 1. From docs.microsoft
	struct addrinfo *result = NULL, hints;

	// Hints to the type of Socket you need
	ZeroMemory(&hints, sizeof(hints)); // memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // Using IPv4 address family
	hints.ai_socktype = SOCK_STREAM; // type of used service
	hints.ai_protocol = IPPROTO_TCP; // Using TCP protocol
	hints.ai_flags = AI_PASSIVE; // Socket purpose

	initResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (initResult != 0) {
		std::cout << "getaddrinfo() failed: " << initResult << '\n';
		system("pause");
		WSACleanup();
		return 1;
	}

	SOCKET listeningSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
								 // AF_INET            SOCK_STREAM          0
	if (listeningSocket == INVALID_SOCKET) {
		std::cout << "socket() failed: " << WSAGetLastError() << '\n';
		system("pause");
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Binding IP address and Port to created socket
	initResult = bind(listeningSocket, result->ai_addr, (int)result->ai_addrlen);
	if (initResult != 0) {
		std::cout << "bind() failed: " << initResult << '\n';
		system("pause");
		freeaddrinfo(result);
		closesocket(listeningSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	// Set socket to listening
	initResult = listen(listeningSocket, SOMAXCONN);
	if (initResult == SOCKET_ERROR) {
		std::cout << "listen() failed: " << WSAGetLastError() << '\n';
		closesocket(listeningSocket);
		WSACleanup();
	}

	// -------------------------------------------------------------------------------------

	// Accepting connection
	std::vector<HANDLE> clientThreads;
	std::vector<Client*> clients;

	clients.push_back(new Client());
	std::cout << "Waiting for connection...\n";
	while (clients.back()->ClientSocket = accept(listeningSocket, (sockaddr*)&clients.back()->client_addr, &clients.back()->client_len)) {
		if (clients.back()->ClientSocket == INVALID_SOCKET) {
			std::cout << "accept() failed: " << WSAGetLastError() << '\n';
			system("pause");
			closesocket(listeningSocket);
			WSACleanup();
			return 1;
		}
		clientThreads.push_back(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ConnectNewClient, new Params(clients.back(), &clients), 0, NULL));
		stopper = false;
		while (true) {
			if (stopper == true) {
				clients.push_back(new Client());
				break;
			}
		}
	}

	WSACleanup();
	system("pause");
	return 0;
}

void WINAPI ConnectNewClient(void* _params) {
	Params* params = (Params*)_params;
	std::cout << "New client connected " << inet_ntoa(params->currentClient->client_addr.sin_addr) << '\n';
	std::vector<Client*>::iterator iter;

	int iResult, iSendResult;
	do {
		iResult = recv(params->currentClient->ClientSocket, params->currentClient->response, sizeof(params->currentClient->response) - 1, 0);
		stopper = true;
		if (iResult > 0) {
			iSendResult = send(params->currentClient->ClientSocket, params->currentClient->response, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				std::cout << "send() with client " << inet_ntoa(params->currentClient->client_addr.sin_addr) << " failed: " << WSAGetLastError << '\n';
				closesocket(params->currentClient->ClientSocket);
				for (iter = params->allClients->begin(); iter != params->allClients->end(); iter++) {
					if ((*iter)->Id == params->currentClient->Id) {
						delete ((*iter));
						params->allClients->erase(iter);
						break;
					}
				}
			}
		}
		else if (iResult == 0) {
			std::cout << "Connection with client " << inet_ntoa(params->currentClient->client_addr.sin_addr) << " closed\n";
		}
		else {
			std::cout << "Receiving from client " << inet_ntoa(params->currentClient->client_addr.sin_addr) << " failed: " << WSAGetLastError() << '\n';
			closesocket(params->currentClient->ClientSocket);
			for (iter = params->allClients->begin(); iter != params->allClients->end(); iter++) {
				if ((*iter)->Id == params->currentClient->Id) {
					delete ((*iter));
					params->allClients->erase(iter);
					break;
				}
			}
		}
	} while (iResult > 0);
}