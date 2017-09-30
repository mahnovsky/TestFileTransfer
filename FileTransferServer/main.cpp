#include <TransferServer.h>

int main()
{
#ifdef _WINSOCK2API_
	InitSockets();
#endif

	int appCode = EXIT_SUCCESS;
	try
	{
		std::unique_ptr<FileTransferServer> 
			transfer(FileTransferServer::MakeServer(PROTOCOL, ADDRESS, PORT));

		transfer->Init();

		transfer->Run();
	}
	catch (const std::exception& exc)
	{
		std::cout << exc.what() << std::endl;

		appCode = EXIT_FAILURE;
	}

#ifdef _WINSOCK2API_
	FreeSockets();
#endif

	return appCode;
}