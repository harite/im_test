/*
 * RouteConn.cpp
 *
 *  Created on: 2013-6-13
 *      Author: jianqingdu
 */

#include "RouteConn.h"

typedef hash_map<conn_handle_t, CRouteConn*> RouteConnMap_t;
typedef hash_map<uint32_t, CRouteConn*> RouteUserMap_t;

static uint32_t g_recv_route_pkt_cnt = 0;
static uint32_t g_send_route_pkt_cnt = 0;
RouteConnMap_t	g_route_conn_map;
RouteUserMap_t	g_route_user_map;


CRouteConn* FindRouteConn(conn_handle_t conn_handle)
{
	CRouteConn* pConn = NULL;
	RouteConnMap_t::iterator iter = g_route_conn_map.find(conn_handle);
	if (iter != g_route_conn_map.end())
	{
		pConn = iter->second;
		pConn->AddRef();
	}

	return pConn;
}

CRouteConn* FindRouteConnByUserId(uint32_t userId)
{
	CRouteConn* pConn = NULL;
	RouteUserMap_t::iterator iter = g_route_user_map.find(userId);
	if (iter != g_route_user_map.end())
	{
		pConn = iter->second;
		pConn->AddRef();
	}

	return pConn;
}

void routeconn_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	NOTUSED_ARG(handle);
	NOTUSED_ARG(uParam);
	NOTUSED_ARG(pParam);

	CRouteConn* pConn = FindRouteConn(handle);
	if (!pConn)
		return;

	//log("msg=%d, handle=%d\n", msg, handle);

	switch (msg)
	{
	case NETLIB_MSG_READ:
		pConn->OnRead();
		break;
	case NETLIB_MSG_WRITE:
		pConn->OnWrite();
		break;
	case NETLIB_MSG_CLOSE:
		pConn->OnClose();
		break;
	default:
		log("!!!routeconn_callback error msg: %d\n", msg);
		break;
	}

	pConn->ReleaseRef();
}


//////////////////////////
CRouteConn::CRouteConn()
{
	log("CRouteConn::CRouteConn\n");
	m_busy = false;
	m_handle = NETLIB_INVALID_HANDLE;

	m_last_send_tick = m_last_recv_tick = get_tick_count();
}

CRouteConn::~CRouteConn()
{
	log("CRouteConn::~CRouteConn, handle=%d\n", m_handle);
}

int CRouteConn::Send(void* data, int len)
{
	m_last_send_tick = get_tick_count();

	if (m_busy)
	{
		m_out_buf.Write(data, len);
		return len;
	}

	int ret = netlib_send(m_handle, data, len);
	if (ret < 0)
		ret = 0;

	if (ret < len)
	{
		m_out_buf.Write((char*)data + ret, len - ret);
		m_busy = true;
		//log("not send all, remain=%d\n", m_out_buf.GetWriteOffset());
	}

	return len;
}

void CRouteConn::Close()
{
	log("CRouteConn::Close, handle=%d\n", m_handle);

	g_route_conn_map.erase(m_handle);

	CRouteConn* pConn = NULL;
	RouteUserMap_t::iterator it, it_tmp;
	for (it = g_route_user_map.begin(); it != g_route_user_map.end(); ) {
		it_tmp = it;
		++it;
		pConn = it_tmp->second;
		if (pConn == this) {
			g_route_user_map.erase(it_tmp);
		}
	}

	if (m_handle != NETLIB_INVALID_HANDLE)
	{
		netlib_close(m_handle);
		m_handle = NETLIB_INVALID_HANDLE;
	}

	ReleaseRef();
}

void CRouteConn::OnConnect(net_handle_t handle)
{
	m_handle = handle;

	g_route_conn_map.insert(make_pair(handle, this));

	netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void*)routeconn_callback);
}

void CRouteConn::OnRead()
{
	for (;;)
	{
		uint32_t free_buf_len = m_in_buf.GetAllocSize() - m_in_buf.GetWriteOffset();
		if (free_buf_len < READ_BUF_SIZE)
			m_in_buf.Extend(READ_BUF_SIZE);

		int ret = netlib_recv(m_handle, m_in_buf.GetBuffer() + m_in_buf.GetWriteOffset(), READ_BUF_SIZE);
		if (ret <= 0)
			break;

		m_in_buf.IncWriteOffset(ret);

		m_last_recv_tick = get_tick_count();
	}

	CImPdu* pPdu = NULL;
	while ( ( pPdu = CImPdu::ReadPdu(m_in_buf.GetBuffer(), m_in_buf.GetWriteOffset()) ) )
	{
		uint8_t pdu_type = pPdu->GetPduType();
		uint32_t pdu_len = pPdu->GetLength();

		switch (pdu_type) {
		case IM_PDU_TYPE_ONLINE_REQUEST:
			HandleOnlineRequest( (CImPduOnlineRequest*)pPdu );
			break;
		case IM_PDU_TYPE_OFFLINE_REQUEST:
			HandleOfflineRequest( (CImPduOfflineRequest*)pPdu );
			break;
		case IM_PDU_TYPE_MSG:
			HandleMsg( (CImPduMsg*)pPdu );
			break;
		default:
			log("Wrong pdu type\n");
			break;
		}

		m_in_buf.Read(NULL, pdu_len);
		delete pPdu;
	}
}

void CRouteConn::OnWrite()
{
	//CFuncLock func_lock(&m_lock);

	if (!m_busy)
		return;

	int ret = netlib_send(m_handle, m_out_buf.GetBuffer(), m_out_buf.GetWriteOffset());
	if (ret < 0)
		ret = 0;

	int out_buf_size = (int)m_out_buf.GetWriteOffset();

	m_out_buf.Read(NULL, ret);

	if (ret < out_buf_size)
	{
		m_busy = true;
		log("not send all, remain=%d\n", m_out_buf.GetWriteOffset());
	}
	else
	{
		m_busy = false;
	}
}

void CRouteConn::OnClose()
{
	Close();
}

void CRouteConn::OnTimer(uint32_t curr_tick)
{

}

void CRouteConn::HandleOnlineRequest(CImPduOnlineRequest* pPdu)
{
	uint32_t userId = pPdu->GetUserId();
	log("HandleOnlineRequest, userId=%d\n", userId);

	g_route_user_map.insert(make_pair(userId, this));
}

void CRouteConn::HandleOfflineRequest(CImPduOfflineRequest* pPdu)
{
	uint32_t userId = pPdu->GetUserId();
	log("HandleOfflineRequest, userId=%d\n", userId);

	g_route_user_map.erase(userId);
}

void CRouteConn::HandleMsg(CImPduMsg* pPdu)
{
	g_recv_route_pkt_cnt++;
	if (g_recv_route_pkt_cnt % 10000 == 0) {
		log("g_recv_route_pkt_cnt=%d\n", g_recv_route_pkt_cnt);
	}

	uint32_t toUserId = pPdu->GetToUserId();

	CRouteConn* pConn = FindRouteConnByUserId(toUserId);

	if (pConn) {
		CImPduMsg pdu(pPdu->GetFromUserId(), pPdu->GetToUserId(), pPdu->GetMsgType(), pPdu->GetMsgData(), pPdu->GetMsgLen());
		pConn->Send(pdu.GetBuffer(), pdu.GetLength());
		pConn->ReleaseRef();

		g_send_route_pkt_cnt++;
		if (g_send_route_pkt_cnt % 10000 == 0) {
			log("g_send_route_pkt_cnt=%d\n", g_send_route_pkt_cnt);
		}
	}
}
