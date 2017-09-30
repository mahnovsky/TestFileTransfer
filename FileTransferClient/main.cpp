#include <TransferClient.h>

class ArgParser
{
public:
	enum ParserState
	{
		File,
		Address,
		Port
	};

	ArgParser(int argc, char ** argv)
		: m_argc(argc)
		, m_argv(argv)
		, m_address(ADDRESS)  // default ip address
		, m_port(PORT)            // default port
		, m_state(File)
	{}

	void Parse(std::vector<std::string>& fileList)
	{
		if (m_argc <= 1)
		{
			throw std::runtime_error("Error: not file name in arguments");
		}

		std::string addressFlag = "-a";
		std::string portFlag = "-p";

		for (int i = 1; i < m_argc; ++i)
		{
			if (m_state == File)
			{
				m_state = m_argv[i] == addressFlag ? Address :
					(m_argv[i] == portFlag ? Port : File);
			}
			else
			{
				continue;
			}

			switch (m_state)
			{
			case File: fileList.push_back(m_argv[i]); break;
			case Address: m_address = m_argv[i]; break;
			case Port: m_port = atoi(m_argv[i]); break;
			}
		}
	}

	const char* GetIpAddress() const
	{
		return m_address.c_str();
	}

	short GetPort() const
	{
		return m_port;
	}

private:
	int         m_argc;
	char **     m_argv;
	std::string m_address;
	short       m_port;
	ParserState m_state;
};

int main(int argc, char ** argv)
{
#ifdef _WINSOCK2API_
	InitSockets();
#endif

	int appCode = EXIT_SUCCESS;
	try
	{
		std::vector<std::string> files;

		ArgParser parser(argc, argv);

		parser.Parse(files);

		std::unique_ptr<FileTransferClient>
			transfer(FileTransferClient::MakeClient(PROTOCOL, ADDRESS, PORT));

		transfer->Init();
		transfer->Transfer(files);
	}
	catch (const std::exception& exc)
	{
		std::cout << exc.what() << std::endl;

		appCode = EXIT_FAILURE;
	}

#ifdef _WINSOCK2API_
	FreeSockets();
#endif

	system("PAUSE");

	return appCode;
}