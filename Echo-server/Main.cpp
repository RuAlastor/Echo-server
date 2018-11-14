#define _WINSOCK_DEPRECATED_NO_WARNINGS // needed to use inet_ntoa
#define DEFAULT_PORT "12321"
#define DEFAULT_BUFFLEN 4096 // The biggest amount of bytes allowed to send in a single message

#include <iostream>
#include <WS2tcpip.h>
#include <vector>
#include <string>

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
		client_len = sizeof(client_addr);
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

namespace Errors {

	class Custom : public std::exception {
	public:
		virtual void closeServer() = 0;
	};

	class WSAStartup : public Errors::Custom {
		std::string message;
	public:
		WSAStartup() : message("WSAStartup() failed!\n") {}

		const char *what() const {
			return message.c_str();
		}

		void closeServer() {}
	};

	class Getaddrinfo : public Errors::Custom {
		std::string message;
	public:
		Getaddrinfo() : message("getaddrinfo() failed!\n") {}

		const char *what() const {
			return message.c_str();
		}

		void closeServer() {
			WSACleanup();
		}
	};

	class Socket : public Errors::Custom {
		std::string message;
		addrinfo* result;
	public:
		Socket(addrinfo* _result, const int& _errorNumber) : message("socket() failed: " + _errorNumber + '\n') {
			result = _result;
		}

		const char *what() const {
			return message.c_str();
		}

		void closeServer() {
			freeaddrinfo(result);
			WSACleanup();
		}
	};

	class Bind : public Errors::Custom {
		std::string message;
		addrinfo* result;
		SOCKET* socket;
	public:
		Bind(addrinfo* _result, SOCKET* _socket) : message("bind() failed!\n") {
			result = _result;
			socket = _socket;
		}

		const char *what() const {
			return message.c_str();
		}

		void closeServer() {
			freeaddrinfo(result);
			closesocket(*(socket));
			WSACleanup();
		}
	};

	class Listen : public Errors::Custom {
		std::string message;
		SOCKET* socket;
	public:
		Listen(SOCKET* _socket, const int& _errorNumber) : message("listen() failed: " + _errorNumber + '\n') {
			socket = _socket;
		}

		const char *what() const {
			return message.c_str();
		}

		void closeServer() {
			closesocket(*(socket));
			WSACleanup();
		}
	};

	class Connection : public Errors::Custom {
		std::string message;
		SOCKET* socket;
	public:
		Connection(SOCKET* _socket, const int& _errornumber) : message("Connection failed: " + _errornumber + '\n') {
			socket = _socket;
		}

		const char *what() const {
			return message.c_str();
		}

		void closeServer() {
			closesocket(*(socket));
			WSACleanup();
		}
	};

	class Send : public Errors::Custom {
		Params* params;
		std::string message;
	public:
		Send(Params* _params) : message("send() to client " + (std::string)inet_ntoa(params->currentClient->client_addr.sin_addr) + " failed\n"){
			params = _params;
		}

		const char *what() const {
			return message.c_str();
		}

		void closeServer() {
			std::vector<Client*>::iterator iter;
			closesocket(params->currentClient->ClientSocket);
			for (iter = params->allClients->begin(); iter != params->allClients->end(); iter++) {
				if ((*iter)->Id == params->currentClient->Id) {
					delete ((*iter));
					params->allClients->erase(iter);
					break;
				}
			}
		}
	};
}

void WINAPI ConnectNewClient(void*);

int main() {
	
	try {
		// Initializing Socket
		WSADATA wsaData;
		int initResult;

		initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (initResult != 0) {
			throw Errors::WSAStartup();
		}

		// Creating Socket
		struct addrinfo *result = NULL, hints;

		// Hints to the type of Socket you need
		ZeroMemory(&hints, sizeof(hints)); // memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET; // Using IPv4 address family
		hints.ai_socktype = SOCK_STREAM; // type of used service
		hints.ai_protocol = IPPROTO_TCP; // Using TCP protocol
		hints.ai_flags = AI_PASSIVE; // Socket purpose

		initResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
		if (initResult != 0) {
			throw Errors::Getaddrinfo();
		}

		SOCKET listeningSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
									 // AF_INET            SOCK_STREAM          0
		if (listeningSocket == INVALID_SOCKET) {
			throw Errors::Socket(result, WSAGetLastError());
		}

		// Binding IP address and Port to created socket
		initResult = bind(listeningSocket, result->ai_addr, (int)result->ai_addrlen);
		if (initResult != 0) {
			throw Errors::Bind(result, &listeningSocket);
		}
		freeaddrinfo(result);

		// Set socket to listening
		initResult = listen(listeningSocket, SOMAXCONN);
		if (initResult == SOCKET_ERROR) {
			throw Errors::Listen(&listeningSocket, WSAGetLastError());
		}

		// Accepting connection
		std::vector<HANDLE> clientThreads;
		std::vector<Client*> clients;

		clients.push_back(new Client());
		std::cout << "Waiting for connection...\n";
		while (clients.back()->ClientSocket = accept(listeningSocket, (sockaddr*)&clients.back()->client_addr, &clients.back()->client_len)) {
			if (clients.back()->ClientSocket == INVALID_SOCKET) {
				throw Errors::Connection(&listeningSocket, WSAGetLastError());
			}
			// Creating a thread for a new client
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
	catch (Errors::Custom& _ERROR) {
		std::cout << _ERROR.what() << '\n';
		system("pause");
		_ERROR.closeServer();
		return 1;
	}
}

// Send back received data to the client
void WINAPI ConnectNewClient(void* _params) {
	Params* params = (Params*)_params;
	std::cout << "New client connected " << inet_ntoa(params->currentClient->client_addr.sin_addr) << ". Client Id: " << params->currentClient->Id << '\n';
	std::vector<Client*>::iterator iter;

	int iResult, iSendResult;
	do {
		iResult = recv(params->currentClient->ClientSocket, params->currentClient->response, sizeof(params->currentClient->response) - 1, 0);
		stopper = true;
		if (iResult > 0) {
			iSendResult = send(params->currentClient->ClientSocket, params->currentClient->response, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				throw Errors::Send(params);
			}
		}
		else if (iResult == 0) {
			std::cout << "Connection with client #" << params->currentClient->Id << " closed\n"; 
			for (iter = params->allClients->begin(); iter != params->allClients->end(); ++iter)
			{
				if ((*iter)->Id == params->currentClient->Id)
				{
					delete((*iter));
					params->allClients->erase(iter);
					break;
				}
			}
			break;
		}
		else {
			throw Errors::Send(params);
		}
	} while (iResult > 0);
}