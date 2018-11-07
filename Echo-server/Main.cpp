#include <iostream>
#include <WS2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")


int main() {
	WSADATA wsaData;
	int initResult;

	// Initialize Winsock
	initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (initResult != 0) {
		std::cout << "WSAStartup failed:" << initResult << '\n';
		system("pause");
		return 1;
	}

	return 0;
}