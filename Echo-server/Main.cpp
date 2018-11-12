#include <iostream>
#include <WS2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "12321"
#define DEFAULT_BUFFLEN 4096


int main() {

	// Initialization
	WSADATA wsaData;
	int initResult;

	initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (initResult != 0) {
		std::cout << "WSAStartup() failed:" << initResult << '\n';
		system("pause");
		return 1;
	}

	// Creating Socket
	struct addrinfo *result = NULL, hints;

	ZeroMemory(&hints, sizeof(hints)); // memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // Using IPv4 address family
	hints.ai_socktype = SOCK_STREAM; // type of used service. CHECK THIS
	hints.ai_protocol = IPPROTO_TCP; // Using TCP protocol
	hints.ai_flags = AI_PASSIVE; // NO IDEA
	
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
	if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "listen() failed: " << WSAGetLastError() << '\n';
		closesocket(listeningSocket);
		WSACleanup();
	}

	// Accepting connection 
	SOCKET ClientSocket = accept(listeningSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		std::cout << "accept() failed: " << WSAGetLastError() << '\n';
		system("pause");
		closesocket(listeningSocket);
		WSACleanup();
		return 1;
	}

	// Receiving and sending data
	char buffer[DEFAULT_BUFFLEN];
	int iResult, iSendResult;

	// Receive until connection is shut down
	do {
		iResult = recv(ClientSocket, buffer, DEFAULT_BUFFLEN, 0);
		if (iResult > 0) {
			std::cout << "Bytes recieved: " << iResult << '\n';
			// Echo message to client
			iSendResult = send(ClientSocket, buffer, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				std::cout << "send() failed: " << WSAGetLastError << '\n';
				system("pause");
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			std::cout << "Bytes sent: " << iSendResult << '\n';
		}
		else if (iResult == 0) {
			std::cout << "Connection closed...\n";
		}
		else {
			std::cout << "Recieving failed: " << WSAGetLastError << '\n';
			system("pause");
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iResult > 0);

	// Disconnecting the server
	initResult = shutdown(ClientSocket, SD_SEND);
	if (initResult == SOCKET_ERROR) {
		std::cout << "shutdown() failed: " << WSAGetLastError() << '\n';
		system("pause");
		closesocket(ClientSocket);
		WSACleanup();
	}
	closesocket(ClientSocket);
	WSACleanup();
	system("pause");

	return 0;
}