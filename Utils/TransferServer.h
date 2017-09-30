#pragma once

#include "Transfer.h"

class FileTransferServer
{
public:
	typedef void(FileTransferServer::* Handler) (const MessageData&);

	enum TransferState
	{
		Idle,
		LoadFile,
		LoadEnd
	};
	typedef std::map<Protocol, Handler> HandlerMap;

	static FileTransferServer* MakeServer(Socket::SocketType protocol, const char* address, short port);

	virtual ~FileTransferServer();

	virtual void Init()
	{
		RegistryHandler(Protocol::FileBegin, &FileTransferServer::HandleFileBegin);
		RegistryHandler(Protocol::FileData, &FileTransferServer::HandleFileData);
		RegistryHandler(Protocol::FileEnd, &FileTransferServer::HandleFileEnd);
		RegistryHandler(Protocol::Done, &FileTransferServer::HandleDone);
	}

	virtual void Run() = 0;

protected:
	FileTransferServer(Socket::SocketType type, const char* address, short port);

	void HandleFileBegin(const MessageData& data);

	void HandleFileData(const MessageData& data);

	void HandleFileEnd(const MessageData& data);

	void HandleDone(const MessageData& data);

	void RegistryHandler(Protocol pr, Handler handler);

	void InvokeHandler(const MessageData& data);

protected:
	std::string     m_address;
	short           m_port;
	Socket          m_socket;
	TransferState   m_state;
	std::ofstream   m_currentFile;
	HandlerMap      m_handlers;
};