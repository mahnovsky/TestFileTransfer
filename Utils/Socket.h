#pragma once

#include "Common.h"

#ifdef _WIN32

#include <WinSock2.h>

void InitSockets();

void FreeSockets();

#endif

class Socket
{
public:
	enum SocketType
	{
		Unknown,
		Tcp = IPPROTO_TCP,
		Udp = IPPROTO_UDP
	};

	Socket(SocketType type);

	~Socket();

	void Init(bool noBlock = false);

	void Listen(int backlog);

	void Connect(const char* address, short port);

	bool Accept(std::unique_ptr<Socket>* sock);

	void Send(const char* buffer, size_t count);

	void Read(char* buffer, size_t count);

	void SendTo(const char* buffer, int len, const sockaddr_in* to);

	int ReadFrom(char* buffer, int len, sockaddr_in* from);

	void Bind(const char* address, short port);

	static void FillAddr(sockaddr_in* addr, const char* ip, short port);

private:
	Socket();

	Socket(const Socket&);

	Socket& operator = (const Socket&);

private:
	SocketType  m_type;
	sockaddr_in m_sin;
	SOCKET      m_sock;
};