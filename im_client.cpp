/*
 * im_client.cpp
 *
 *  Created on: 2013-6-5
 *      Author: jianqingdu
 */

#include "netlib.h"
#include "util.h"
#include "imconn.h"

#define SERVER_PORT		8008

// this callback will be replaced by servconn_callback() in OnConnect()
void client_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	printf("client_cb, msg=%d\n", msg);
}


int main(int argc, char* argv[])
{
	if (argc != 4) {
		printf("im_client server_ip start_id max_conn_num\n");
		return -1;
	}

	char* server_ip = argv[1];
	int start_id = atoi(argv[2]);
	int max_conn_num = atoi(argv[3]);

	int ret = netlib_init();

	if (ret == NETLIB_ERROR)
		return ret;

	CImConn* pConn = NULL;
	for (int i = 0; i < max_conn_num; i++) {
		pConn = new CImConn();
		pConn->Connect(server_ip, SERVER_PORT, i + start_id, NULL, NULL);

		util_sleep(2);
	}

	InitImConn();

	netlib_eventloop();

	return 0;
}
