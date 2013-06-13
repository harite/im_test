/*
 * imconn.cpp
 *
 *  Created on: 2013-6-5
 *      Author: jianqingdu
 */

#include "imconn.h"

static const char* StateName[] = {
	"CONN_STATE_IDLE",
	"CONN_STATE_CONECTING",
	"CONN_STATE_CONNECTED",
	"CONN_STATE_OPEN",
	"CONN_STATE_CLOSE"
};

typedef hash_map<conn_handle_t, CImConn*> ConnMap_t;
typedef hash_map<uint32_t, CImConn*> UserMap_t;

static uint32_t g_pkt_cnt;
ConnMap_t	g_conn_map;
UserMap_t	g_user_map;
char send_buf[128];
const char* test_msg = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
uint32_t g_min_userId = (uint32_t)-1;
uint32_t g_max_userId = 0;

CImConn* FindImConnByUserId(uint32_t userId);

void imconn_timer_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam)
{
	NOTUSED_ARG(callback_data);
	NOTUSED_ARG(msg);
	NOTUSED_ARG(handle);
	NOTUSED_ARG(pParam);

	CImConn* pConn = NULL;
	for (int i = 0; i < 200; i++) {
		uint32_t userId = rand() % (g_max_userId - g_min_userId) + g_min_userId;
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
	m_state = CONN_STATE_IDLE;
	m_busy = false;
	m_userId = 0;
	m_handle = NETLIB_INVALID_HANDLE;
	m_callback = NULL;
	m_callback_data = NULL;

	m_last_send_tick = m_last_recv_tick = get_tick_count();

	//SetLock(&m_lock);
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

	if (g_min_userId > userId) {
		g_min_userId = userId;
	}

	if (g_max_userId < userId) {
		g_max_userId = userId;
	}

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
	m_handle = handle;

	g_conn_map.insert(make_pair(handle, this));

	_SetState(CONN_STATE_OPEN);
	netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void*)imconn_callback);
}

void CImConn::OnConfirm()
{
	if (m_state == CONN_STATE_CONNECTING)
	{
		_SetState(CONN_STATE_OPEN);
	}
	else
	{
		log("state error: %d\n", m_state);
	}

	CImPduOnlineRequest pdu(m_userId);
	Send(pdu.GetBuffer(), pdu.GetLength());
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

		g_pkt_cnt++;
		if (g_pkt_cnt % 10000 == 0) {
			log("recv g_pkt_cnt=%d\n", g_pkt_cnt);
		}

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

void CImConn::OnClose()
{
	if (m_state == CONN_STATE_CLOSE)
		return;

	Close();
}

void CImConn::OnTimer(uint32_t curr_tick)
{
	/*if (curr_tick - m_last_send_tick > IMCONN_KEEPALIVE_TIMEOUT)
	{
		Send(send_buf, 128);
	}

	if (curr_tick - m_last_recv_tick > IMCONN_TIMEOUT)
	{
		log("imconn timeout, close the connection, handle=%d\n", m_handle);
		OnClose();
	}*/
	if (m_state == CONN_STATE_OPEN) {
		//printf("send\n");
		CImPduMsg pdu(m_userId, m_userId + 1, 1, (char*)test_msg);
		Send(pdu.GetBuffer(), pdu.GetLength());
	}
}

void CImConn::HandleOnlineRequest(CImPduOnlineRequest* pPdu)
{
	m_userId = pPdu->GetUserId();

	g_user_map.insert(make_pair(m_userId, this));
}

void CImConn::HandleMsg(CImPduMsg* pPdu)
{
	uint32_t toUserId = pPdu->GetToUserId();

	if (toUserId == m_userId) {
		if (m_callback) {
			m_callback(m_callback_data, NETLIB_MSG_READ, m_handle, 0,  pPdu->GetMsgContent());
		}
		return;
	}

	CImConn* pConn = FindImConnByUserId(toUserId);

	if (pConn) {
		CImPduMsg pdu(pPdu->GetFromUserId(), pPdu->GetToUserId(), pPdu->GetMsgType(), pPdu->GetMsgContent());
		pConn->Send(pdu.GetBuffer(), pdu.GetLength());
		pConn->ReleaseRef();
	}
}

void CImConn::_SetState(uint8_t state)
{
	log("conn_handle=%d, %s -> %s\n", m_handle, StateName[m_state], StateName[state]);
	m_state = state;
}


