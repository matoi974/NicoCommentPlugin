﻿#include <string>
#include "SimpleSocket.h"
#include "constants.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma once
SimpleSocket::SimpleSocket() {
		TheSocket = INVALID_SOCKET;
		ptr = NULL;
		result = NULL;
		ipv4=L"0.0.0.0";
}
	
SimpleSocket::~SimpleSocket(){
	if (TheSocket != INVALID_SOCKET) CloseSocket();
}

bool SimpleSocket::InitializeSocket(std::wstring server, std::wstring port)	{ //0 for success, 1 for error
	//Initialize the variables...Maybe not neccessary
	TheSocket = INVALID_SOCKET;
	ptr = NULL;
	result = NULL;

	int iResult;
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		onSysMsg(L"WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfoW hints;
	
	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;	
	iResult = GetAddrInfoW(server.c_str(), port.c_str(), &hints, &result);

	if (iResult != 0) {
		onSysMsg(L"GetAddrInfoW failed: %d\n", iResult);
		WSACleanup();
		return 1; 
	}	
		
	// Attempt to connect to the first address returned by
	// the call to GetAddrInfoW
	ptr=result;

	// Create a SOCKET for connecting to server
	TheSocket = socket(ptr->ai_family, ptr->ai_socktype, 
	ptr->ai_protocol);		

	if (TheSocket == INVALID_SOCKET) {
		onSysMsg(L"Error at socket(): %d\n", WSAGetLastError()); 
		FreeAddrInfoW(result);
		WSACleanup();
		return 1; 
	}
	return 0;
}

bool SimpleSocket::ConnectSocket() { //0 for success, 1 for error
	int iResult=SOCKET_ERROR;
	// try the next address returned by GetAddrInfoW if the connect call failed
	for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
		iResult = connect( TheSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		/*ipv4*/
		sockaddr_in* tmp_addr=(sockaddr_in*)(ptr->ai_addr);
		unsigned char i1=(tmp_addr->sin_addr.S_un.S_un_b.s_b1);
		unsigned char i2=(tmp_addr->sin_addr.S_un.S_un_b.s_b2);
		unsigned char i3=(tmp_addr->sin_addr.S_un.S_un_b.s_b3);
		unsigned char i4=(tmp_addr->sin_addr.S_un.S_un_b.s_b4);
		wchar_t buffer[32]=L"";
		swprintf_s(buffer,32,L"%d.%d.%d.%d",i1,i2,i3,i4);
		ipv4=std::wstring(buffer);
		
		if (iResult == SOCKET_ERROR) { //ERROR
			closesocket(TheSocket);
			TheSocket = INVALID_SOCKET;
		} else if (iResult == 0) break;
	} 

	FreeAddrInfoW(result); // after success connection or tried all addrs in result, free "result"

	if (TheSocket == INVALID_SOCKET) {
		onSysMsg(L"Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	else return 0;
}

bool SimpleSocket::CloseSocket() {	
// shutdown the send half of the connection since no more data will be sent
	int iResult = shutdown(TheSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		onSysMsg(L"shutdown failed: %d\n", WSAGetLastError());
		closesocket(TheSocket);
		WSACleanup();
		return 1;
	}
	// cleanup
	closesocket(TheSocket);
	WSACleanup();
	return 0;
}


bool SimpleSocket::send_data(char* buffer, unsigned int bufferlen) {//Send some data
	int iResult;
	iResult = send(TheSocket,buffer,bufferlen,0);
	if (iResult == SOCKET_ERROR) {
		onSysMsg(L"send failed: %d\n", WSAGetLastError()); /*NEED CHANGE*/
		closesocket(TheSocket);
		WSACleanup();
		return 1;
	}
	// shutdown the connection for sending since no more data will be sent
	// the client can still use TheSocket for receiving data
		/*	iResult = shutdown(TheSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
		onDebugMsg(L"shutdown failed: %d\n", WSAGetLastError());
		closesocket(TheSocket);
		WSACleanup();
		}	*/ //do not shutdown if more data will be sent.
	return (iResult == SOCKET_ERROR); //true for Error, false for Normal termination.
}	

bool SimpleSocket::recv_data(char* buffer, unsigned int &bufferlen) {
	int iResult;
	// TCP has an 64kb limit; recv for this size. buffer should be recvbuffer.
	memset(buffer, 0, DEFAULT_BUFLEN);
	iResult = recv(TheSocket, buffer, DEFAULT_BUFLEN, 0);
	//printf("%d\n",iResult);
	//printf(buffer);
	if (iResult > 0) bufferlen=iResult;
	else if (iResult == SOCKET_ERROR) onSysMsg(L"recv failed: %d\n", WSAGetLastError());
	return (iResult == SOCKET_ERROR); //true for Error, false for Normal termination.
}

std::wstring SimpleSocket::SocketIP() {return ipv4;}