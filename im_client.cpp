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
	if (argc != 8) {
		printf("usage: im_client im_server_ip src_start_id src_end_id dst_start_id dst_end_id pkt_per_second pkt_size\n");
		printf("\t [src_start_id, src_end_id] is the start and end id for concurrent connections\n");
		printf("\t [dst_start_id, dst_end_id] is the start and end id for send the packet to that connections\n");
		printf("\t pkt_per_second is how many packet to send in one second\n");
		printf("\t pkt_size is the packet size\n");
		return -1;
	}

	signal(SIGPIPE, SIG_IGN);

	char* server_ip = argv[1];
	int src_start_id = atoi(argv[2]);
	int src_end_id = atoi(argv[3]);
	int dst_start_id = atoi(argv[4]);
	int dst_end_id = atoi(argv[5]);
	int pkt_per_second = atoi(argv[6]);
	int pkt_size = atoi(argv[7]);

	int ret = netlib_init();

	if (ret == NETLIB_ERROR)
		return ret;

	CImConn* pConn = NULL;
	for (int i = src_start_id; i <= src_end_id; i++) {
		pConn = new CImConn();
		pConn->Connect(server_ip, SERVER_PORT, i, NULL, NULL);

		util_sleep(2);
	}

	InitImConn(src_start_id, src_end_id, dst_start_id, dst_end_id, pkt_per_second, pkt_size);

	netlib_eventloop();

	return 0;
}
