#include "stdafx.h"
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

//#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <bitset>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"//port tcp



//zrobic zabespieczenie przed za duzym wynikiem

class serwer {
public:

	bool error = false;


	std::bitset<3> operations = 1;
	std::bitset<32> liczba1 = 000;//otrzymana liczba
	std::bitset<32> liczba2 = 110;//wynik koncowy
	std::bitset<2> status = 11;//ostatni komunikat?
	std::bitset<8> id = 01;//przesyla losowy numer?
	std::bitset<3> padding = 000;//by bylo pozielne przez 8





	WSADATA wsaData;
	int iResult;
	std::string s;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char sendbuf[256];
	unsigned int sendbuflen;
	char recvbuf[256];
	unsigned int recvbuflen = DEFAULT_BUFLEN;




	



	void validation();
	void connectsocket();
	void receive();
	void sending();

	void serwer::odkompresowanie_str_na_bit();// nadanie nowej wartosci bitsetom
	void operacje_na_danych();// zmiana liczba2
	std::string serwer::kompresowanie_bitow_na_str();

};

void serwer::odkompresowanie_str_na_bit()
{
	std::string se;//string bitow
	std::string temp;// jeden 8bitowy char w str

// rozpakowanie bitow z charow i wstawienie ich w jeden, duzy string
	for (auto e : s)
		se+= std::bitset<8>(e).to_string();
	
std::cout <<"odkompresowane dane: ";
	// wpakowanie bitow w odpowiednie bitsety 
	for (int i = 0;i < 48;i++)
	{
		temp += se[i];
		
		if(i==2){
			operations = std::bitset<3>(temp);std::cout << temp << " ";
			temp = "";
		}
		if (i == 32+2)
		{
			liczba1 = std::bitset<32>(temp);std::cout << temp << " ";
			temp = "";
		}
		if (i == 2+32+2) {
			status = std::bitset<2>(temp);std::cout << temp << " ";
			temp = "";
		}
		if (i == 8+2+32+2) {
			id = std::bitset<8>(temp);std::cout << temp << " ";
			temp = "";
		}
		
	}
}

void serwer::operacje_na_danych() 
{
	std::string str;//wynik
	std::string se;//string bitow

	
	//wlasciwe dzialania na danych
	//patrzenie, czy liczba2 nie wyjdzie za okres
	if (operations == 000) //dodawanie
	{	
		int a = liczba1.to_ulong() + liczba2.to_ulong();
		liczba2 = std::bitset<32>(a); 
	}
	if (operations == 001) { liczba2 = liczba1; }//odejmowanie
	{
		int a = liczba1.to_ulong() + liczba2.to_ulong();
		liczba2 = std::bitset<32>(a);
	}
	if (operations == 010) { liczba2 = liczba1; }//mnozenie
	{
		int a = liczba1.to_ulong() + liczba2.to_ulong();
		liczba2 = std::bitset<32>(a);
	}
	if (operations == 011) { liczba2 = liczba1; }//dzielenie
	{
		int a = liczba1.to_ulong() + liczba2.to_ulong();
		liczba2 = std::bitset<32>(a);
	}

	if (operations == 100) { liczba2 = liczba1; }
	if (operations == 101) { liczba2 = liczba1; }
	if (operations == 111) { liczba2 = liczba1; }

	std::cout <<"\nnowa liczba: "<< liczba2<<"\n";
}

std::string serwer::kompresowanie_bitow_na_str() {

	//nowe bitsety w 1 string
	std::string se = operations.to_string() + liczba2.to_string() + status.to_string() + id.to_string() + padding.to_string();
	std::bitset<48> wynik(se);


	std::string str;//string 8bitowych charow
	std::string temp;//zawartosc jednego chara


	for (int i = 0;i < 48;i++) {
		temp += se[i];

		if (i % 8 == 7) {
			auto a = std::bitset<8>(temp);// string na bitset, i ten bitset spakowany jako 1 znak w char		
			str += a.to_ulong();
			temp.clear();
		}

	}

	std::cout << "\nspakowane dane to\n" << str << '\n';

	return str;
}




void serwer::validation() 
{
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		error=true;
	}
	else
		std::cout << "serwer uruchomiony\n";

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		error=true;
	}


	if (!error)printf("validating completed\n\n");
}


void serwer::connectsocket()
{
	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		error=true;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		error=true;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		error=true;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		error=true;
	}
	

	// No longer need server socket
	closesocket(ListenSocket);


	if (!error)printf("connecting completed\n\n");
}


void serwer::receive()
{
	// Receive until the peer shuts down the connection
	do {
		//otrzymanie
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			s = "      ";
			for (int i = 0;i < 6;i++)
			s[i] = recvbuf[i];
	
			std::cout << "Otrzymano slowo: \n" << s << '\n';
			printf("Bytes received: %d\n", iResult);
			


		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			error=true;
		}

	} while (iResult > 0);


	if (!error)printf("receiving completed\n\n");
}


void serwer::sending()
{
	//wysylanie
odkompresowanie_str_na_bit();
operacje_na_danych();
s = kompresowanie_bitow_na_str();


	char sendb[6];
	for (int i = 0;i < 6;i++)
		sendb[i] = s[i];
	sendbuflen = s.size();

	// send the buffer back to the sender
	iSendResult = send(ClientSocket, sendb, sendbuflen, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		error=true;
	}
	std::cout << "\nwyslane slowo: " << s << '\n';
	printf("Bytes sent: %d\n", iSendResult);




	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);



	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		error=true;
	}


	// cleanup
	closesocket(ClientSocket);
	WSACleanup();



	if (!error)printf("sending completed\n\n");
}









int __cdecl main(void)
{
	

	serwer s;

	if (!s.error)
		s.validation();
	if (!s.error)
		s.connectsocket();
	if (!s.error)
		s.receive();
	if (!s.error)
		s.sending();
	

	if (s.error) { std::cout << "error"; }



return 0;
}