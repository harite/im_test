/*
 * RouteConn.h
 *
 *  Created on: 2013-6-13
 *      Author: jianqingdu
 */

#ifndef ROUTECONN_H_
#define ROUTECONN_H_

#include "netlib.h"
#include "util.h"
#include "impdu.h"

#define ROUTECONN_KEEPALIVE_TIMEOUT	30000	// 30 seconds
#define ROUTECONN_TIMEOUT			120000	// 120 seconds
#define READ_BUF_SIZE				1024

class CRouteConn : public CRefObject
{
public:
	CRouteConn();
	virtual ~CRouteConn();

	int Send(void* data, int len);

	void Close();

	void OnConnect(net_handle_t handle);
	void OnRead();
	void OnWrite();
	void OnClose();
	void OnTimer(uint32_t curr_tick);

	void HandleOnlineRequest(CImPduOnlineRequest* pPdu);
	void HandleOfflineRequest(CImPduOfflineRequest* pPdu);
	void HandleMsg(CImPduMsg* pPdu);

private:
	net_handle_t	m_handle;
	bool			m_busy;
	CSimpleBuffer	m_in_buf;
	CSimpleBuffer	m_out_buf;

	uint32_t		m_last_send_tick;
	uint32_t		m_last_recv_tick;
};


#endif /* ROUTECONN_H_ */
