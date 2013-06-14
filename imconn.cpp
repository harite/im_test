/*
 * imconn.cpp
 *
 *  Created on: 2013-6-5
 *      Author: jianqingdu
 */

#include "imconn.h"
#define MAX_MSG_SIZE	1024
#define MIN_MSG_SIZE	128

static const char* StateName[] = {
	"CONN_STATE_IDLE",
	"CONN_STATE_CONECTING",
	"CONN_STATE_CONNECTED",
	"CONN_STATE_OPEN",
	"CONN_STATE_CLOSE"
};

typedef hash_map<conn_handle_t, CImConn*> ConnMap_t;
typedef hash_map<uint32_t, CImConn*> UserMap_t;

static uint32_t g_recv_pkt_cnt = 0;
static uint32_t g_send_pkt_cnt = 0;
static uint32_t g_to_routeserver_pkt_cnt = 0;
static uint32_t g_from_routeserver_pkt_cnt = 0;

ConnMap_t	g_conn_map;
UserMap_t	g_user_map;
uint32_t g_min_userId = 1;
uint32_t g_max_userId = 100000;
CImConn* g_route_conn = NULL;
uchar_t g_msg_buf[MAX_MSG_SIZE];

CImConn* FindImConnByUserId(uint32_t userId);

void imconn_timer_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	NOTUSED_ARG(callback_data);
	NOTUSED_ARG(msg);
	NOTUSED_ARG(handle);
	NOTUSED_ARG(pParam);

	CImConn* pConn = NULL;
	for (int i = 0; i < 250; i++) {
		uint32_t userId = rand() % (g_max_userId/2 - g_min_userId) + g_min_userId;
		pConn = FindImConnByUserId(userId);
		if (pConn) {
			pConn->OnTimer(uParam);
			pConn->ReleaseRef();
		}
	}

}

void InitImConn()
{
	srand(time(NULL));
	netlib_register_timer(imconn_timer_callback, NULL, 50);
}

CImConn* FindImConn(conn_handle_t conn_handle)
{
	CImConn* pConn = NULL;
	ConnMap_t::iterator iter = g_conn_map.find(conn_handle);
	if (iter != g_conn_map.end())
	{
		pConn = iter->second;
		pConn->AddRef();
	}

	return pConn;
}

CImConn* FindImConnByUserId(uint32_t userId)
{
	CImConn* pConn = NULL;
	UserMap_t::iterator iter = g_user_map.find(userId);
	if (iter != g_user_map.end())
	{
		pConn = iter->second;
		pConn->AddRef();
	}

	return pConn;
}

void imconn_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	NOTUSED_ARG(handle);
	NOTUSED_ARG(uParam);
	NOTUSED_ARG(pParam);

	CImConn* pConn = FindImConn(handle);
	if (!pConn)
		return;

	//log("msg=%d, handle=%d\n", msg, handle);

	switch (msg)
	{
	case NETLIB_MSG_CONFIRM:
		pConn->OnConfirm();
		break;
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
		log("!!!imconn_callback error msg: %d\n", msg);
		break;
	}

	pConn->ReleaseRef();
}


//////////////////////////
CImConn::CImConn()
{
	log("CImConn::CImConn\n");
	m_server_port = 0;
	m_state = CONN_STATE_IDLE;
	m_busy = false;
	m_userId = 0;
	m_handle = NETLIB_INVALID_HANDLE;
	m_callback = NULL;
	m_callback_data = NULL;

	m_last_send_tick = m_last_recv_tick = get_tick_count();
}

CImConn::~CImConn()
{
	log("CImConn::~CImConn, handle=%d\n", m_handle);
}

conn_handle_t CImConn::Connect(
		const char*		server_ip,
		const uint16_t	port,
		const uint32_t	userId,
		callback_t		callback,
		void*			callback_data)
{
	m_server_ip = server_ip;
	m_server_port = port;
	m_userId = userId;
	m_callback = callback;
	m_callback_data = callback_data;

	_SetState(CONN_STATE_CONNECTING);

	m_handle = netlib_connect(server_ip, port, imconn_callback, NULL);

	g_conn_map.insert(make_pair(m_handle, this));
	g_user_map.insert(make_pair(userId, this));

	return m_handle;
}

int CImConn::Send(void* data, int len)
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

void CImConn::Close()
{
	log("CImConn::Close, handle=%d\n", m_handle);

	if (g_route_conn) {
		if (g_route_conn == this) {
			printf("disconnect from route_server\n");
			g_route_conn = NULL;
		} else {
			CImPduOfflineRequest pdu(m_userId);
			g_route_conn->Send(pdu.GetBuffer(), pdu.GetLength());
		}
	}

	g_conn_map.erase(m_handle);
	g_user_map.erase(m_userId);

	if (m_handle != NETLIB_INVALID_HANDLE)
	{
		netlib_close(m_handle);
		m_handle = NETLIB_INVALID_HANDLE;
	}

	ReleaseRef();
}

void CImConn::OnConnect(net_handle_t handle)
{
	m_conn_type = CONN_TYPE_IM_SERVER;
	m_handle = handle;

	g_conn_map.insert(make_pair(handle, this));

	_SetState(CONN_STATE_CONNECTED);
	netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void*)imconn_callback);
}

void CImConn::OnConfirm()
{
	if (m_userId != 0) {	// im_client
		CImPduOnlineRequest pdu(m_userId);
		Send(pdu.GetBuffer(), pdu.GetLength());

		m_conn_type = CONN_TYPE_IM_CLIENT;
	} else {
		g_route_conn = this;
		printf("im_server connected to route_server\n");

		m_conn_type = CONN_TYPE_ROUTE_SERVER;
	}

	_SetState(CONN_STATE_OPEN);
}

void CImConn::OnRead()
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

void CImConn::OnWrite()
{
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

void CImConn::OnClose()
{
	if (m_state == CONN_STATE_CLOSE)
		return;

	Close();
}

void CImConn::OnTimer(uint32_t curr_tick)
{
	if (m_state == CONN_STATE_OPEN) {
		uint32_t toUserId = rand() % (g_max_userId - g_min_userId) + g_min_userId;	// toUserId in [g_min_userId, g_max_userId)
		uint16_t msg_len = rand() % (MAX_MSG_SIZE - MIN_MSG_SIZE) + MIN_MSG_SIZE;
		CImPduMsg pdu(m_userId, toUserId, 1, g_msg_buf, msg_len);
		Send(pdu.GetBuffer(), pdu.GetLength());

		g_send_pkt_cnt++;
		if (g_send_pkt_cnt % 10000 == 0) {
			log("im_client, g_send_pkt_cnt=%d\n", g_send_pkt_cnt);
		}
	}
}

void CImConn::HandleOnlineRequest(CImPduOnlineRequest* pPdu)
{
	if (m_conn_type != CONN_TYPE_IM_SERVER) {
		log("not an im server, but receive a online request\n");
		return;
	}

	_SetState(CONN_STATE_OPEN);

	m_userId = pPdu->GetUserId();

	g_user_map.insert(make_pair(m_userId, this));

	if (g_route_conn) {
		CImPduOnlineRequest pdu(m_userId);
		g_route_conn->Send(pdu.GetBuffer(), pdu.GetLength());
	}
}

/*
 * three scenarios:
 * 1. an im_client connection, discard packet
 * 2. an im_server connection, send packet to im_client or route_server
 * 3. a connection to route server, send packet to im_client or discard packet
 */
void CImConn::HandleMsg(CImPduMsg* pPdu)
{
	g_recv_pkt_cnt++;
	if (g_recv_pkt_cnt % 10000 == 0) {
		log("g_recv_pkt_cnt=%d\n", g_recv_pkt_cnt);
	}

	if (m_conn_type == CONN_TYPE_IM_CLIENT) {
		return;
	}

	if (m_conn_type == CONN_TYPE_ROUTE_SERVER) {
		g_from_routeserver_pkt_cnt++;
		if (g_from_routeserver_pkt_cnt % 10000 == 0) {
			log("g_from_routeserver_pkt_cnt: %d\n", g_from_routeserver_pkt_cnt);
		}
	}

	uint32_t toUserId = pPdu->GetToUserId();
	CImConn* pConn = FindImConnByUserId(toUserId);

	if (pConn) {
		CImPduMsg pdu(pPdu->GetFromUserId(), pPdu->GetToUserId(), pPdu->GetMsgType(), pPdu->GetMsgData(), pPdu->GetMsgLen());
		pConn->Send(pdu.GetBuffer(), pdu.GetLength());
		pConn->ReleaseRef();
	} else {
		if (m_conn_type == CONN_TYPE_IM_SERVER && g_route_conn) {
			CImPduMsg pdu(pPdu->GetFromUserId(), pPdu->GetToUserId(), pPdu->GetMsgType(),
							pPdu->GetMsgData(), pPdu->GetMsgLen());

			g_route_conn->Send(pdu.GetBuffer(), pdu.GetLength());

			g_to_routeserver_pkt_cnt++;
			if (g_to_routeserver_pkt_cnt % 10000 == 0) {
				log("g_to_routeserver_pkt_cnt: %d\n", g_to_routeserver_pkt_cnt);
			}
		}
	}
}

void CImConn::_SetState(uint8_t state)
{
	log("conn_handle=%d, %s -> %s\n", m_handle, StateName[m_state], StateName[state]);
	m_state = state;
}


