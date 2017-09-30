#include "TransferServer.h"


FileTransferServer::FileTransferServer(Socket::SocketType type, const char* address, short port)
	: m_address(address)
	, m_port(port)
	, m_socket(type)
	, m_state(TransferState::Idle)
{}

FileTransferServer::~FileTransferServer()
{}

void FileTransferServer::HandleFileBegin(const MessageData& data)
{
	if (m_state != TransferState::Idle || m_currentFile.is_open())
	{
		throw std::runtime_error("Error: [HandleFileBegin] file opened or transfer state not idle");
	}

	m_currentFile.open(data.data, std::ios::trunc | std::ios::binary);

	if (!m_currentFile.is_open())
	{
		throw std::runtime_error("Error: [HandleFileBegin] failed open file");
	}

	m_state = TransferState::LoadFile;

	std::cout << "Load new file: " << data.data << std::endl;
}

void FileTransferServer::HandleFileData(const MessageData& data)
{
	if (m_state != TransferState::LoadFile || !m_currentFile.is_open())
	{
		throw std::runtime_error("Error: [HandleFileData] file not opened or transfer state not LoadFile");
	}

	m_currentFile.write(data.data, data.dataSize);
}

void FileTransferServer::HandleFileEnd(const MessageData& data)
{
	if (m_state != TransferState::LoadFile || !m_currentFile.is_open())
	{
		throw std::runtime_error("Error: [HandleFileEnd] file not opened or transfer state not LoadFile");
	}

	m_currentFile.close();

	m_state = TransferState::Idle;
}

void FileTransferServer::HandleDone(const MessageData& data)
{
	if (m_state != TransferState::Idle || m_currentFile.is_open())
	{
		throw std::runtime_error("Error: [HandleDone] file opened or transfer state not idle");
	}

	m_state = TransferState::LoadEnd;
}

void FileTransferServer::RegistryHandler(Protocol pr, Handler handler)
{
	m_handlers.insert(std::make_pair(pr, handler));
}

void FileTransferServer::InvokeHandler(const MessageData& data)
{
	HandlerMap::iterator iter = m_handlers.find(data.protocol);

	if (iter != m_handlers.end())
	{
		(this->*iter->second)(data);
	}
}

class TcpServer :public FileTransferServer
{
public:
	TcpServer(const char* address, short port)
		:FileTransferServer(Socket::Tcp ,address, port)
	{}

	void Run() override
	{
		std::unique_ptr<Socket> client;

		if (!m_socket.Accept(&client))
		{
			throw std::runtime_error("Error: failed accept");
		}

		try
		{
			while (m_state != TransferState::LoadEnd)
			{
				MessageData data;
				client->Read((char*)&data, sizeof(data));

				InvokeHandler(data);

				Answer(client.get(), Protocol::Accepted, "Data accepted.");
			}
		}
		catch (const std::runtime_error& error)
		{
			std::cout << error.what() << std::endl;
			Answer(client.get(), Protocol::FatalError, error.what());
		}
	}

	void Init() override
	{
		m_socket.Init();

		m_socket.Bind(m_address.c_str(), m_port);

		m_socket.Listen(5);

		FileTransferServer::Init();
	}

	void Answer(Socket* client, Protocol pr, const std::string& message)
	{
		MessageData accepted(Protocol::Accepted, message);
		client->Send((char*)&accepted, sizeof(accepted));
	}
};

class UdpServer :public FileTransferServer
{
public:
	UdpServer(const char* address, short port)
		:FileTransferServer(Socket::Udp ,address, port)
	{}

	void HandleChunk(const MessageData& data) 
	{
		int pos = data.dataIndex * MAX_LENGTH;
		lastIndex = data.dataIndex;
		memcpy(buff + pos, data.data, data.dataSize);
	}

	void Run() override
	{
		sockaddr_in tmp;

		while (m_state != TransferState::LoadEnd)
		{
			try
			{
				MessageData data;

				m_socket.ReadFrom((char*)&data, sizeof(data), &tmp);

				if (data.protocol == Protocol::Chunk)
				{
					HandleChunk(data);
				}
				else if (data.protocol == Protocol::ChunkEnd)
				{
					m_currentFile.write(buff, data.dataSize);
				}
				else
				{
					InvokeHandler(data);
				}

				Answer(&tmp, Protocol::Accepted, "Data accepted.");
			}
			catch (const std::runtime_error& error)
			{
				std::cout << error.what() << std::endl;
				Answer(&tmp, Protocol::FatalError, error.what());
			}
		}
	}

	void Init() override
	{
		m_socket.Init();

		m_socket.Bind(m_address.c_str(), m_port);

		FileTransferServer::Init();
	}

	void Answer(const sockaddr_in* client, Protocol pr, const std::string& message)
	{
		MessageData accepted(pr, message);
		accepted.dataIndex = lastIndex;
		m_socket.SendTo((char*)&accepted, sizeof(accepted), client);
	}

private:
	char buff[MAX_LENGTH * CHUNK_LENGTH];
	int lastIndex;
};

FileTransferServer* FileTransferServer::MakeServer(Socket::SocketType protocol, const char* address, short port)
{
	if (protocol == Socket::Tcp)
		return new TcpServer(address, port);
	else if(protocol == Socket::Udp)
		return new UdpServer(address, port);

	return nullptr;
}