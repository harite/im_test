#ifndef __NETLIB_H__
#define __NETLIB_H__

#include "ostype.h"

#define NETLIB_OPT_SET_CALLBACK			1	
#define NETLIB_OPT_SET_CALLBACK_DATA	2
#define NETLIB_OPT_GET_REMOTE_IP		3
#define NETLIB_OPT_GET_REMOTE_PORT		4
#define NETLIB_OPT_GET_LOCAL_IP			5
#define NETLIB_OPT_GET_LOCAL_PORT		6

#ifdef __cplusplus
extern "C" {
#endif

int netlib_init();

int netlib_destroy();

int netlib_listen(	
		const char*	server_ip, 
		uint16_t	port,
		callback_t	callback,
		void*		callback_data);

net_handle_t netlib_connect(
		const char*	server_ip,
		uint16_t	port,
		callback_t	callback,
		void*		callback_data);

int netlib_send(net_handle_t handle, void* buf, int len);

int netlib_recv(net_handle_t handle, void* buf, int len);

int netlib_close(net_handle_t handle);

int netlib_option(net_handle_t handle, int opt, void* optval);

int netlib_register_timer(callback_t callback, void* user_data, uint32_t interval);

int netlib_delete_timer(callback_t callback, void* user_data);

void netlib_eventloop();

#ifdef __cplusplus
}
#endif

#endif
