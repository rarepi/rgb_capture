//https://docs.microsoft.com/en-us/windows/desktop/winsock/complete-client-code
#include <../client.h>
#include <stdio.h> 
#include <winsock2.h>
#include <ws2tcpip.h>
#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "7777"

SOCKET s;

int sendData(unsigned char* data, int size) {
	char* dataBytes = reinterpret_cast<char*>(data);
	int result = send(s, dataBytes, size, 0);
	if (result == SOCKET_ERROR) {
		//printf("TCPClient: send failed with error: %d\n", WSAGetLastError());
		closesocket(s);
		WSACleanup();
		return 1;
	}
	printf("TCPClient: Sent %ld bytes.\n", result);
	return 0;
}

int connect() {
		WSADATA wsaData;
		s = INVALID_SOCKET;
		struct addrinfo *result = NULL,
			*ptr = NULL,
			hints;
		int iResult;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("TCPClient: WSAStartup failed with error: %d\n", iResult);
			return 1;
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
		iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
		if (iResult != 0) {
			printf("TCPClient: getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		// Attempt to connect to an address until one succeeds
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Create a SOCKET for connecting to server
			s = socket(ptr->ai_family, ptr->ai_socktype,
				ptr->ai_protocol);
			if (s == INVALID_SOCKET) {
				printf("TCPClient: socket failed with error: %ld\n", WSAGetLastError());
				WSACleanup();
				return 1;
			}

			// Connect to server.
			iResult = connect(s, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				closesocket(s);
				s = INVALID_SOCKET;
				continue;
			}
			break;
		}

		freeaddrinfo(result);

		if (s == INVALID_SOCKET) {
			printf("TCPClient: Unable to connect to server!\n");
			WSACleanup();
			return 1;
		}
		return 0;
}

int disconnect() {
	char recvbuf[DEFAULT_BUFLEN];
	int iResult = shutdown(s, SD_SEND);
	int recvbuflen = DEFAULT_BUFLEN;

	if (iResult == SOCKET_ERROR) {
		printf("TCPClient: shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(s);
		WSACleanup();
		return 1;
	}

	do {
		iResult = recv(s, recvbuf, recvbuflen, 0);
		if (iResult > 0)
			printf("TCPClient: Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("TCPClient: Connection closed\n");
		else
			printf("TCPClient: recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);

	// cleanup
	closesocket(s);
	WSACleanup();
}