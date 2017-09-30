#include "TransferClient.h"
#include "Transfer.h"
#include <functional>
#include <memory>

#define PROGRESS_LENGTH 256

class ProgressPrinter
{
public:
	ProgressPrinter(int dataCount, int base = 10)
		: frames(0)
		, m_dt(dataCount / base)
	{
		memset(progressBar, 0, PROGRESS_LENGTH);
		memset(progress, ' ', 10);
		progress[10] = '\0';
	}

	void Update(int i)
	{
		if (m_dt == 0)
			return;
		if ((i / m_dt) > frames)
		{
			progress[frames] = '=';
			const int persent = i / m_dt * 10;

			sprintf_s(progressBar, PROGRESS_LENGTH, "[%s] %d %% %s",
				progress, persent, m_fileName.c_str());

			++frames;
		}
	}

	void Print()
	{
		system("cls");
		std::cout << progressBar;
	}

	void SetFileName(const std::string& fileName)
	{
		m_fileName = fileName;
	}

private:
	char        progressBar[PROGRESS_LENGTH];
	char        progress[11];
	int         frames;
	std::string m_fileName;
	const int   m_dt;
};

FileTransferClient::FileTransferClient(Socket::SocketType type, const char* address, short port)
	: m_address(address)
	, m_port(port)
	, m_socket(type)
{}

FileTransferClient::~FileTransferClient()
{}

void FileTransferClient::CheckAnswer()
{
	MessageData serverAnswer;
	Read(serverAnswer);
	if (serverAnswer.protocol == Protocol::FatalError)
	{
		throw std::runtime_error(serverAnswer.data);
	}
}

void FileTransferClient::FileTransferBegin(const char* fileName)
{
	std::string name = fileName;
	size_t pos = name.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		name.erase(0, pos + 1);
	}

	MessageData header(Protocol::FileBegin, name);
	Send(header);

	CheckAnswer();
}

void FileTransferClient::FileTransferData(const char* fileName)
{
	std::unique_ptr<FILE, std::function<void(FILE*)>> file(fopen(fileName, "rb"), [](FILE* f) { fclose(f); });

	if (file == nullptr)
	{
		char buff[256];
		sprintf_s(buff, 256, "Error: failed load file, name %s", fileName);
		throw std::runtime_error(buff);
	}

	SendFile(file.get());

	std::cout << std::endl;
	MessageData data;
	data.protocol = Protocol::FileEnd;
	Send(data);
}

void FileTransferClient::FileTransferDone()
{
	MessageData data(Protocol::Done, "Done.");
	Send(data);

	CheckAnswer();
}

void FileTransferClient::Transfer(const std::vector<std::string>& files)
{
	for (size_t i = 0; i < files.size(); ++i)
	{
		FileTransferBegin(files[i].c_str());

		FileTransferData(files[i].c_str());
	}

	FileTransferDone();
}

class TcpClient : public FileTransferClient
{
public:
	TcpClient(const char* address, short port)
		:FileTransferClient(Socket::Tcp, address, port)
	{
	}

	void Init() override
	{
		m_socket.Init();

		m_socket.Connect(m_address.c_str(), m_port);
	}

	void Send(const MessageData& data) override
	{
		m_socket.Send((const char*)&data, sizeof(MessageData));
	}

	void Read(MessageData& data) override
	{
		m_socket.Read((char*)&data, sizeof(data));
	}

	void SendFile(FILE* file) override
	{
		MessageData data;

		while (!feof(file))
		{
			data.dataSize = fread(data.data, 1, MAX_LENGTH, file);

			Send(data);

			CheckAnswer();
		}
	}
};

class UdpClient : public FileTransferClient
{
public:
	UdpClient(const char* address, short port)
		:FileTransferClient(Socket::Udp, address, port)
	{
		Socket::FillAddr(&m_serverAddr, address, port);
	}

	void Init() override
	{
		m_socket.Init();
	}

	void Send(const MessageData& data) override
	{
		m_socket.SendTo((const char*)&data, sizeof(data), &m_serverAddr);
	}

	void Read(MessageData& data) override
	{
		m_socket.ReadFrom((char*)&data, sizeof(data), &m_serverAddr);
	}

	void SendFile(FILE* file) override
	{
		fseek(file, 0, SEEK_END);
		const size_t fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		ProgressPrinter printer(fileSize / MAX_LENGTH);

		MessageData data;
		data.protocol = Protocol::Chunk;

		char buff[MAX_LENGTH * CHUNK_LENGTH];
		bool done[CHUNK_LENGTH] = {0};
		int reTry = 0;
		int size = 0;
		int chunks = 0;

		while (!feof(file))
		{
			if(reTry == 0)
				size = fread(buff, 1, MAX_LENGTH * CHUNK_LENGTH, file);

			int count = ceil(size / MAX_LENGTH);
			for (int i = 0; i < count; ++i)
			{
				if (done[i])
					continue;

				const int pos = i * MAX_LENGTH;
				data.dataIndex = i;
				data.dataSize = size - pos;
				if (data.dataSize > MAX_LENGTH) 
					data.dataSize = MAX_LENGTH;

				memcpy(data.data, buff + pos, data.dataSize);

				Send(data);
			}

			const DWORD begin = timeGetTime();
			int i = 0;
			while (true)
			{
				MessageData md;
				Read(md);

				if (md.protocol == Protocol::Accepted && 
					md.dataIndex >= 0 && 
					md.dataIndex < CHUNK_LENGTH &&
					!done[md.dataIndex])
				{
					done[md.dataIndex] = true;
					
					++i;
					printer.Update(i + chunks * CHUNK_LENGTH);
				}

				if (i >= count)
				{
					for (int j = 0; j < CHUNK_LENGTH; ++j)
						done[j] = false;

					reTry = 0;
					ChunkEnd(size);
					++chunks;
					break;
				}

				if (timeGetTime() - begin > 300)
				{
					++reTry;
				}
			}

			if (reTry > 5)
				break;

			printer.Print();
		}
	}

	void ChunkEnd(int size)
	{
		MessageData data;
		data.protocol = Protocol::ChunkEnd;
		data.dataSize = size;

		int i = 0;

		do
		{
			Send(data);

			MessageData md;
			Read(md);

			if (md.protocol == Protocol::Accepted)
			{
				break;
			}
	
			Sleep(100);
			++i;
		} 
		while (i < 5);
	}

private:
	sockaddr_in m_serverAddr;
};


FileTransferClient* FileTransferClient::MakeClient(Socket::SocketType type, const char* address, short port)
{
	if (type == Socket::Tcp)
	{
		return  new TcpClient(address, port);
	}
	else if (type == Socket::Udp)
	{
		return new UdpClient(address, port);
	}

	return  nullptr;
}
