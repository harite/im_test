#include "netlib.h"
#include "util.h"
#include "imconn.h"

#define SERVER_IP	"0.0.0.0"
#define SERVER_PORT	8008

// this callback will be replaced by servconn_callback() in OnConnect()
void serv_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	NOTUSED_ARG(callback_data);
	NOTUSED_ARG(uParam);
	NOTUSED_ARG(pParam);

	if (msg == NETLIB_MSG_CONNECT)
	{
		CImConn* pConn = new CImConn();
		pConn->OnConnect(handle);
	}
	else
	{
		log("!!!error msg: %s\n", msg);
	}
}


int main(int argc, char* argv[])
{
	NOTUSED_ARG(argc);
	NOTUSED_ARG(argv);

	//InitImConn();

	int ret = netlib_init();

	if (ret == NETLIB_ERROR)
		return ret;

	ret = netlib_listen(SERVER_IP, SERVER_PORT, serv_callback, NULL);
	if (ret == NETLIB_ERROR)
		return ret;

	printf("server start listen on: %s:%d\n", SERVER_IP, SERVER_PORT);
	printf("now enter the event loop...\n");

	netlib_eventloop();

	return 0;
}
