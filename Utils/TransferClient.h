#pragma once

#include "Transfer.h"

class MessageData;

class FileTransferClient
{
public:

	virtual ~FileTransferClient();

	virtual void Init() = 0;

	void Transfer(const std::vector<std::string>& files);

	static FileTransferClient* MakeClient(Socket::SocketType type, const char* address, short port);

protected:

	FileTransferClient(Socket::SocketType type, const char* address, short port);

	void CheckAnswer();

	void FileTransferBegin(const char* fileName);

	void FileTransferData(const char* fileName);

	void FileTransferDone();

	virtual void Send(const MessageData& data) = 0;

	virtual void Read(MessageData& data) = 0;

	virtual void SendFile(FILE* file) = 0;

protected:
	std::string m_address;
	short       m_port;
	Socket      m_socket;
};