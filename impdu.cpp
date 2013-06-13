/*
 * impdu.cpp
 *
 *  Created on: 2013-6-8
 *      Author: jianqingdu
 */

#include <assert.h>
#include "impdu.h"

CImPdu::CImPdu()
{
	m_incoming_buf = NULL;
	m_incoming_len = 0;
}

uchar_t* CImPdu::GetBuffer()
{
	if (m_incoming_buf)
		return m_incoming_buf;
	else
		return m_buf.GetBuffer();
}

uint32_t CImPdu::GetLength()
{
	if (m_incoming_buf)
		return m_incoming_len;
	else
		return m_buf.GetWriteOffset();
}

void CImPdu::WriteHeader()
{
	uchar_t* buf = GetBuffer();

	buf[0] = GetPduType();
	buf[1] = IM_PDU_VERSION;
	CByteStream::WriteUint32(buf + 2, GetLength());
}


CImPdu* CImPdu::ReadPdu(uchar_t *buf, uint32_t len)
{
	uint32_t pdu_len = 0;
	if (!_IsPduAvailable(buf, len, pdu_len))
		return NULL;

	uint8_t pdu_type = buf[0];
	uchar_t* payload_buf = buf + IM_PDU_HEADER_LEN;
	uint32_t payload_len = pdu_len - IM_PDU_HEADER_LEN;
	CImPdu* pPdu = NULL;

	switch (pdu_type)
	{
	case IM_PDU_TYPE_ONLINE_REQUEST:
		pPdu = new CImPduOnlineRequest(payload_buf, payload_len);
		break;
	case IM_PDU_TYPE_MSG:
		pPdu = new CImPduMsg(payload_buf, payload_len);
		break;

	default:
		log("!!!CImPdu::ReadPdu, wrong pdu type: %d\n", pdu_type);
		return NULL;
	}

	pPdu->_SetIncomingLen(pdu_len);
	pPdu->_SetIncomingBuf(buf);
	return pPdu;
}

bool CImPdu::_IsPduAvailable(uchar_t* buf, uint32_t len, uint32_t& pdu_len)
{
	if (len < IM_PDU_HEADER_LEN)
		return false;

	pdu_len = CByteStream::ReadUint32(buf + 2);
	if (pdu_len > len)
	{
		//log("pdu_len=%d, len=%d\n", pdu_len, len);
		return false;
	}

	return true;
}

//CImPduOnlineRequest
CImPduOnlineRequest::CImPduOnlineRequest(uchar_t* buf, uint32_t len)
{
	CByteStream is(buf, len);

	is >> m_userId;

	assert(is.GetPos() == len);
}

CImPduOnlineRequest::CImPduOnlineRequest(uint32_t userId)
{
	CByteStream os(&m_buf, IM_PDU_HEADER_LEN);
	m_buf.Write(NULL, IM_PDU_HEADER_LEN);

	os << userId;

	WriteHeader();
}

//CImPduOfflineRequest
CImPduOfflineRequest::CImPduOfflineRequest(uchar_t* buf, uint32_t len)
{
	CByteStream is(buf, len);

	is >> m_userId;

	assert(is.GetPos() == len);
}

CImPduOfflineRequest::CImPduOfflineRequest(uint32_t userId)
{
	CByteStream os(&m_buf, IM_PDU_HEADER_LEN);
	m_buf.Write(NULL, IM_PDU_HEADER_LEN);

	os << userId;

	WriteHeader();
}

//CImPduMsg
CImPduMsg::CImPduMsg(uchar_t* buf, uint32_t len)
{
	m_msgContent = NULL;
	CByteStream is(buf, len);

	is >> m_fromUserId;
	is >> m_toUserId;
	is >> m_msgType;
	m_msgContent = is.ReadString();

	assert(is.GetPos() == len);
}


CImPduMsg::CImPduMsg(uint32_t fromUserId, uint32_t toUserId, uint8_t msg_type, char* msg_content)
{
	m_msgContent = NULL;
	CByteStream os(&m_buf, IM_PDU_HEADER_LEN);
	m_buf.Write(NULL, IM_PDU_HEADER_LEN);

	os << fromUserId;
	os << toUserId;
	os << msg_type;
	os.WriteString(msg_content);

	WriteHeader();
}

CImPduMsg::~CImPduMsg()
{
	if (m_msgContent) {
		free(m_msgContent);
		m_msgContent = NULL;
	}
}
