/*
 * imconn.h
 *
 *  Created on: 2013-6-5
 *      Author: jianqingdu
 */

#ifndef IMCONN_H_
#define IMCONN_H_

#include "netlib.h"
#include "util.h"
#include "impdu.h"

#define IMCONN_KEEPALIVE_TIMEOUT	30000	// 30 seconds
#define IMCONN_TIMEOUT				120000	// 120 seconds
#define READ_BUF_SIZE				1024

enum {
	CONN_STATE_IDLE,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_OPEN,
	CONN_STATE_CLOSE
};

typedef enum {
	CONN_TYPE_IM_CLIENT = 1,
	CONN_TYPE_IM_SERVER,
	CONN_TYPE_ROUTE_SERVER
} e_conntype_t;

class CImConn : public CRefObject
{
public:
	CImConn();
	virtual ~CImConn();

	conn_handle_t Connect(
			const char*		server_ip,
			const uint16_t	port,
			const uint32_t	userId,
			callback_t		callback,
			void*			callback_data);

	int Send(void* data, int len);

	void Close();

	void OnConnect(net_handle_t handle);
	void OnConfirm();
	void OnRead();
	void OnWrite();
	void OnClose();
	void OnTimer(uint32_t curr_tick);

	void HandleOnlineRequest(CImPduOnlineRequest* pPdu);
	void HandleMsg(CImPduMsg* pPdu);
private:
	void _SetState(uint8_t state);

private:
	uint8_t			m_state;
	string			m_server_ip;
	uint16_t		m_server_port;
	uint32_t		m_userId;
	e_conntype_t	m_conn_type;

	callback_t		m_callback;
	void*			m_callback_data;

	net_handle_t	m_handle;
	bool			m_busy;
	CSimpleBuffer	m_in_buf;
	CSimpleBuffer	m_out_buf;

	uint32_t		m_last_send_tick;
	uint32_t		m_last_recv_tick;
};


void InitImConn(int src_min_id, int src_max_id, int dst_min_id, int dst_max_id, int pkt_per_second, int pkt_size);

#endif /* IMCONN_H_ */
