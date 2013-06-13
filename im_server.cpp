#include "netlib.h"
#include "util.h"
#include "imconn.h"

#define IM_SERVER_PORT		8008
#define ROUTE_SERVER_PORT	8000

// this callback will be replaced by imconn_callback() in OnConnect()
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
	if (argc != 3) {
		printf("usage: im_server listen_ip route_server_ip\n");
		return -1;
	}

	char* im_server_ip = argv[1];
	char* route_server_ip = argv[2];

	/*
	 * Write to a socket that have received RST signal will cause SIGPIPE signal,
	 * the default action of this signal is exit process, so the process is just disappear
	 * without any trace(actually echo $? will print 141(128+13)).
	 * So we change the signal action to ignore the SIGPIPE.
	 */
	signal(SIGPIPE, SIG_IGN);
	//InitImConn();

	int ret = netlib_init();

	if (ret == NETLIB_ERROR)
		return ret;

	ret = netlib_listen(im_server_ip, IM_SERVER_PORT, serv_callback, NULL);
	if (ret == NETLIB_ERROR)
		return ret;

	printf("server start listen on: %s:%d\n", im_server_ip, IM_SERVER_PORT);

	CImConn* pConn = new CImConn();
	conn_handle_t handle = pConn->Connect(route_server_ip, ROUTE_SERVER_PORT, 0, NULL, NULL);
	if (handle == NETLIB_INVALID_HANDLE) {
		printf("wrong route server ip\n");
	} else {
		printf("connecting to route_server...\n");
	}

	printf("now enter the event loop...\n");

	netlib_eventloop();

	return 0;
}
