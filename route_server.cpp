/*
 * route_server.cpp
 *
 *  Created on: 2013-6-13
 *      Author: jianqingdu
 */

#include "netlib.h"
#include "util.h"
#include "RouteConn.h"

#define SERVER_IP	"0.0.0.0"
#define SERVER_PORT	8000

// this callback will be replaced by routeconn_callback() in OnConnect()
void serv_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	NOTUSED_ARG(callback_data);
	NOTUSED_ARG(uParam);
	NOTUSED_ARG(pParam);

	if (msg == NETLIB_MSG_CONNECT)
	{
		CRouteConn* pConn = new CRouteConn();
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

	/*
	 * Write to a socket that have received RST signal will cause SIGPIPE signal,
	 * the default action of this signal is exit process, so the process is just disappear
	 * without any trace(actually echo $? will print 141(128+13)).
	 * So we change the signal action to ignore the SIGPIPE.
	 */
	signal(SIGPIPE, SIG_IGN);

	int ret = netlib_init();

	if (ret == NETLIB_ERROR)
		return ret;

	ret = netlib_listen(SERVER_IP, SERVER_PORT, serv_callback, NULL);
	if (ret == NETLIB_ERROR)
		return ret;

	printf("route_server start listen on: %s:%d\n", SERVER_IP, SERVER_PORT);
	printf("now enter the event loop...\n");

	netlib_eventloop();

	return 0;
}
