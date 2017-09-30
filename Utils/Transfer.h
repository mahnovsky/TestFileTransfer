#pragma once

#include "Socket.h"
#include <map>

enum Protocol
{
	FileBegin,
	FileData,
	FileEnd,
	Chunk,
	ChunkEnd,
	Done,
	FatalError,
	Accepted
};

#define TRANSPORT_UDP 1
#define TRANSPORT_TCP 0

#define PORT 5500
#define ADDRESS "127.0.0.1"

#if TRANSPORT_UDP
	#define PROTOCOL Socket::Udp

	#define MAX_LENGTH 500
	#define CHUNK_LENGTH 20
#endif

#if TRANSPORT_TCP
	#define PROTOCOL Socket::Udp

	#define MAX_LENGTH 10000
	#define CHUNK_LENGTH 1
#endif

struct MessageData
{
	Protocol	protocol;
	size_t		dataSize;
	int			dataIndex;
	char		data[MAX_LENGTH];

	MessageData() {}

	MessageData(Protocol pr, const std::string& message)
		: protocol(pr)
		, dataSize(message.size())
		, dataIndex(0)
	{
		sprintf_s(data, MAX_LENGTH, "%s", message.c_str());
	}
};
