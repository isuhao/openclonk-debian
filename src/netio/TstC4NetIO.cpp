#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
* Copyright (c) 2005  Günther Brammer
* Copyright (c) 2006  Peter Wortmann

#include <C4NetIO.h>

#include <iostream>
#include <sstream>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#else
#include <arpa/inet.h>
#endif

using namespace std;

bool Log(char const * text) { std::cout << text << std::endl; return true; }

bool fHost;
int iCnt = 0, iSize = 0;
char DummyData[1024 * 1024];

#define ASYNC_CONNECT

// TCP otherwise
#define USE_UDP

class MyCBClass : public C4NetIOMan
{
	unsigned int iTime, iPcks;
public:
	virtual bool OnConn(const C4NetIO::addr_t &addr, const C4NetIO::addr_t &addr2, C4NetIO *pNetIO)
	{
		cout << "got connection from " << inet_ntoa(addr.sin_addr) << endl;
		iTime = timeGetTime(); iPcks = 0;

#ifdef ASYNC_CONNECT
		if (!fHost)
		{
			DummyData[0] = 0;
			if (!pNetIO->Send(C4NetIOPacket(DummyData, iSize, true, addr)))
				cout << " Fehler: " << (pNetIO->GetError() ? pNetIO->GetError() : "bla") << endl;
		}
#endif
		return true;
	}
	virtual void OnPacket(const class C4NetIOPacket &rPacket, C4NetIO *pNetIO)
	{
		if (timeGetTime() > iTime + 1000)
		{
			cout << iPcks << " packets in " << timeGetTime() - iTime << " ms (" << iPcks * 1000 / (timeGetTime() - iTime) << " per second, " << (iPcks ? (timeGetTime() - iTime) * 1000 / iPcks : -1u) << "us per packet)" << endl;
			iTime = timeGetTime(); iPcks = 0;
		}
		if (!rPacket.getStatus())
		{
			// dummys
			DummyData[0] = 1;
			C4NetIOPacket Dummy(DummyData, iSize, true, rPacket.getAddr());
			for (int i = 0; i < iCnt; i++)
			{
				if (!pNetIO->Send(Dummy))
					cout << " Fehler: " << (pNetIO->GetError() ? pNetIO->GetError() : "bla") << endl;
			}
			// pong
			pNetIO->Send(rPacket);
			iPcks++;
		}
		// cout << "got " << rPacket.GetSize() << " bytes of data (" << int(*rPacket.GetData()) << ")" << endl;
	}
	virtual void OnDisconn(const C4NetIO::addr_t &addr, C4NetIO *pNetIO, const char *szReason)
	{
		cout << "client from " << inet_ntoa(addr.sin_addr) << " disconnected (" << szReason << ")" << endl;
	}
	virtual void OnError(const char *strError, C4NetIO *pNetIO)
	{
		cout << "network error: " << strError << endl;
	};
};

int main(int argc, char * argv[])
{

	int i;
	for (i = 0; i < sizeof(DummyData); i++)
		DummyData[i] = 'A' + i % 100;

	srand(time(NULL));

#ifdef USE_UDP
	C4NetIOUDP NetIO;
#else
	C4NetIOTCP NetIO;
#endif

	C4NetIO *pNetIO = &NetIO;
	MyCBClass CBClass;
	CBClass.AddIO(pNetIO);

#ifdef HAVE_WINSOCK
	WSADATA wsaData;

	WSAStartup(0x0202, &wsaData);
#endif

	C4NetIO::addr_t addr;
	// Default
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(11111);
	addr.sin_family = AF_INET;
	short iPort = 0;

	for (i = 1; i < argc; ++i)
	{
		std::string arg(argv[i]);
		int n;
		if (arg == "--server")
		{
			fHost = true;
			if (!iPort) iPort = 11111;
		}
		else if ((n = arg.find("--port=")) != -1)
		{
			std::istringstream stream(std::string(arg.begin() + n + sizeof("--port="), arg.end()));
			stream >> iPort;
		}
		else if ((n = arg.find("--size=")) != -1)
		{
			std::istringstream stream(std::string(arg.begin() + n + sizeof("--size="), arg.end()));
			stream >> iSize;
		}
		else
		{
			if (!ResolveAddress(argv[i], &addr, 11111)) cout << "Fehler in ResolveAddress(" << argv[i] << ")" << std::endl;
			if (!iPort) iPort = 11112;
		}
	}
	if (argc == 1)
	{
#ifndef _WIN32
		cout << "Possible usage: " << argv[0] << " [--server] [address[:port]] --port=port --size=size" << std::endl << std::endl;
#endif

		cout << "Server? (j/n)";
		char cChoice;
		do
			cin >> cChoice;
		while (tolower(cChoice) != 'n' && tolower(cChoice) != 'j');

		fHost = (tolower(cChoice) == 'j');

		if (cChoice == 'J' || cChoice == 'N')
		{
			iPort = (cChoice == 'J' ? 11111 : 11112);
			iCnt = 0;
			iSize = 0;
		}
		else
		{
			cout << "Port?";
			cin >> iPort;

			cout << "Dummys?";
			do
				cin >> iCnt;
			while (iCnt < 0);

			cout << "Gr��e?";
			do
				cin >> iSize;
			while (iSize < 1 || iSize > 1024);
		}
		if (!fHost)
		{
			char szAddr[1024];
			int iDPort;
			if (cChoice == 'N')
			{
				strcpy(szAddr, "127.0.0.1");
				iDPort = 11111;
			}
			else
			{
				cout << "Wohin (Adresse)? ";
				cin >> szAddr;

				cout << "Wohin (Port)? ";
				cin >> iDPort;
			}
			addr.sin_addr.s_addr = inet_addr(szAddr);
			addr.sin_port = htons(iDPort);
			addr.sin_family = AF_INET;
		}
	}
	cout << "\nC4NetIO Init...";

	if (!NetIO.Init(iPort))
	{
		cout << " Fehler: " << NetIO.GetError() << endl;
		return 0;
	}

	cout << " ok" << endl;


	if (fHost)
	{

		cout << "   Broadcast...";
		C4NetIO::addr_t addr;
		if (!NetIO.InitBroadcast(&addr))
		{
			cout << " Fehler: " << NetIO.GetError() << endl;
			return 0;
		}

		cout << " ok" << endl;

		cout << "   Thread...";

		if (!CBClass.Start())
		{
			cout << " Fehler!" << endl;
			return 0;
		}

		cout << " ok" << endl;

		cout << "listening... " << endl;

	}
	else
	{
		cout << "   Thread...";

		if (!CBClass.Start())
		{
			cout << " Fehler!" << endl;
			return 0;
		}

		cout << " ok" << endl;

		cout << "verbinden mit " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << "...\n";
#ifdef ASYNC_CONNECT
		NetIO.Connect(addr);
#else
		if (!fHost)
		{
			int iBufLen = 5000;
			char *pBuf = new char [iBufLen];
			for (int i = 0; i < 10; i++)
			{
				*pBuf = i;
				if (!pNetIO->Send(C4NetIOPacket(pBuf, iBufLen, true, addr)))
					cout << " Fehler: " << (pNetIO->GetError() ? pNetIO->GetError() : "bla") << endl;
			}
		}
#endif
	}

	while (std::cin.get() == 10);

	CBClass.Stop();
	NetIO.Close();

	return 1;
}
