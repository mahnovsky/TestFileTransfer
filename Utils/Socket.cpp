#include "Socket.h"

#ifdef _WINSOCK2API_

void InitSockets()
{
	WORD sockVer = MAKEWORD(2, 2);;
	WSADATA wsaData;

	WSAStartup(sockVer, &wsaData);
}

void FreeSockets()
{
	WSACleanup();
}

#endif

Socket::Socket()
	: m_type(SocketType::Unknown)
	, m_sock(INVALID_SOCKET)
{}

Socket::Socket(SocketType type)
	: m_type(type)
	, m_sock(INVALID_SOCKET)
{}


Socket::~Socket()
{
	if (m_sock != INVALID_SOCKET)
		closesocket(m_sock);
}

void Socket::FillAddr(sockaddr_in* addr, const char* ip, short port)
{
	memset(addr, 0, sizeof(addr));
	addr->sin_family = PF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
}


void Socket::Init(bool noBlock)
{
	const int sockType = m_type == Tcp ? SOCK_STREAM : SOCK_DGRAM;
	m_sock = socket(PF_INET, sockType, (int)m_type);

	if (m_sock == INVALID_SOCKET)
	{
		throw std::runtime_error("Error: socket is invalid");
	}
	if (noBlock)
	{
		unsigned long mode = 1;  // 1 to enable non-blocking socket
		ioctlsocket(m_sock, FIONBIO, &mode);
	}
}

void Socket::Bind(const char* address, short port)
{
	sockaddr_in addr;
	FillAddr(&addr, address, port);

	int retVal = bind(m_sock, (LPSOCKADDR)&addr, sizeof(addr));
	if (retVal == SOCKET_ERROR)
	{
		std::string lastError = std::to_string(GetLastError());
		throw std::runtime_error(("Error: " + lastError).c_str());
	}
}

void Socket::Listen(int backlog)
{
	if (m_type == Udp)
		return;

	int retVal = listen(m_sock, backlog);

	if (retVal == SOCKET_ERROR)
	{
		std::string lastError = std::to_string(GetLastError());
		throw std::runtime_error(("Error: " + lastError).c_str());
	}
}

void Socket::Connect(const char* address, short port)
{
	sockaddr_in addr;
	FillAddr(&addr, address, port);

	int retVal = connect(m_sock, (sockaddr*)&addr, sizeof(addr));

	if (retVal == SOCKET_ERROR)
	{
		throw std::runtime_error("Error: unable to connect");
	}
}

bool Socket::Accept(std::unique_ptr<Socket>* sock)
{
	if (m_type == Udp)
	{
		return false;
	}

	Socket* accepted(new Socket);
	int nameLen = sizeof(accepted->m_sin);
	accepted->m_sock = accept(m_sock, (sockaddr*)&accepted->m_sin, &nameLen);

	if (accepted->m_sock == INVALID_SOCKET)
	{
		delete accepted;
		return false;
	}
	sock->reset(accepted);

	return true;
}

void Socket::Send(const char* buffer, size_t count)
{
	assert(buffer != NULL);
	assert(count > 0);

	int retVal = send(m_sock, buffer, count, 0);

	if (retVal == SOCKET_ERROR)
	{
		throw std::runtime_error("Error: unable to send");
	}
}

void Socket::Read(char* buffer, size_t count)
{
	assert(buffer != NULL);
	assert(count > 0);

	int retVal = recv(m_sock, buffer, count, 0);

	if (retVal == SOCKET_ERROR)
	{
		throw std::runtime_error("Error: unable to send");
	}
}

void Socket::SendTo(const char* buffer, int len, const sockaddr_in* to = nullptr)
{
	assert(buffer != NULL);
	assert(len > 0);

	
	int retVal = sendto(m_sock, buffer, len, 0, (LPSOCKADDR)to, sizeof(sockaddr_in));

	if (retVal == SOCKET_ERROR)
	{
		int code = WSAGetLastError();
		throw std::runtime_error("Error: unable to send");
	}
}

int Socket::ReadFrom(char* buffer, int len, sockaddr_in* from)
{
	assert(buffer != NULL);
	assert(len > 0);

	int size = sizeof(sockaddr_in);
	int retVal = recvfrom(m_sock, buffer, len, 0, (LPSOCKADDR)from, &size);

	if (retVal == SOCKET_ERROR)
	{
		int lastError = WSAGetLastError();
		std::string msg = "Error: unable to read " + std::to_string(lastError);
		throw std::runtime_error(msg.c_str());
	}

	return retVal;
}