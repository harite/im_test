/*
 * impdu.h
 *
 *  Created on: 2013-6-8
 *      Author: jianqingdu
 */

#ifndef IMPDU_H_
#define IMPDU_H_

#include "util.h"

#define IM_PDU_HEADER_LEN		6
#define IM_PDU_VERSION			0

#define IM_PDU_TYPE_ONLINE_REQUEST		1
#define IM_PDU_TYPE_OFFLINE_REQUEST		2
#define IM_PDU_TYPE_MSG					3

class CImPdu
{
public:
	CImPdu();
	virtual ~CImPdu() {}

	uchar_t* GetBuffer();
	uint32_t GetLength();
	void WriteHeader();
	virtual uint8_t GetPduType() { return 0; }

	static CImPdu* ReadPdu(uchar_t* buf, uint32_t len);
private:
	static bool _IsPduAvailable(uchar_t* buf, uint32_t len, uint32_t& pdu_len);
	void _SetIncomingLen(uint32_t len) { m_incoming_len = len; }
	void _SetIncomingBuf(uchar_t* buf) { m_incoming_buf = buf; }

protected:
	CSimpleBuffer		m_buf;
	uchar_t*			m_incoming_buf;
	uint32_t			m_incoming_len;
};

class CImPduOnlineRequest : public CImPdu
{
public:
	CImPduOnlineRequest(uchar_t* buf, uint32_t len);
	CImPduOnlineRequest(uint32_t userId);
	virtual ~CImPduOnlineRequest() {}

	virtual uint8_t GetPduType() { return IM_PDU_TYPE_ONLINE_REQUEST; }

	uint32_t GetUserId() { return m_userId; }
private:
	uint32_t	m_userId;
};

class CImPduOfflineRequest : public CImPdu
{
public:
	CImPduOfflineRequest(uchar_t* buf, uint32_t len);
	CImPduOfflineRequest(uint32_t userId);
	virtual ~CImPduOfflineRequest() {}

	virtual uint8_t GetPduType() { return IM_PDU_TYPE_OFFLINE_REQUEST; }

	uint32_t GetUserId() { return m_userId; }
private:
	uint32_t	m_userId;
};

class CImPduMsg : public CImPdu
{
public:
	CImPduMsg(uchar_t* buf, uint32_t len);
	CImPduMsg(uint32_t fromUserId, uint32_t toUserId, uint8_t msgType, char* msgContent);
	virtual ~CImPduMsg();

	virtual uint8_t GetPduType() { return IM_PDU_TYPE_MSG; }

	uint32_t GetFromUserId() { return m_fromUserId; }
	uint32_t GetToUserId() { return m_toUserId; }
	uint8_t GetMsgType() { return m_msgType; }
	char* GetMsgContent() { return m_msgContent; }
private:
	uint32_t	m_fromUserId;
	uint32_t 	m_toUserId;
	uint8_t		m_msgType;
	char*		m_msgContent;
};

#endif /* IMPDU_H_ */
