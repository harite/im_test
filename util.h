#ifndef __UTIL_H__
#define __UTIL_H__

#define _CRT_SECURE_NO_DEPRECATE	// remove warning C4996, 

#include "ostype.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define	snprintf	sprintf_s
#else
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <hash_map>
using namespace stdext;
#else
#include <ext/hash_map>
using namespace __gnu_cxx;
#endif

#include <map>
#include <list>
#include <string>
using namespace std;

#define MAX_LOG_FILE_SIZE	0x800000	// 8MB
#define NOTUSED_ARG(v) ((void)v)		// used this to remove warning C4100, unreferenced parameter

class CThread
{
public:
	CThread();
	virtual ~CThread();

#ifdef _WIN32
	static DWORD WINAPI StartRoutine(LPVOID lpParameter);
#else
	static void* StartRoutine(void* arg);
#endif

	virtual void StartThread(void);
	virtual void OnThreadRun(void) = 0;
private:
#ifdef _WIN32
	DWORD		m_thread_id;
#else
	pthread_t	m_thread_id;
#endif
};

class CEventThread : public CThread
{
public:
	CEventThread();
	virtual ~CEventThread();

 	virtual void OnThreadTick(void) = 0;
	virtual void OnThreadRun(void);
	virtual void StartThread();
	virtual void StopThread();
	bool IsRunning() { return m_bRunning; }
private:
#ifdef _WIN32
	DWORD		m_thread_id;
#else
	pthread_t	m_thread_id;
#endif
	bool 		m_bRunning;
};

class CThreadLock
{
public:
	CThreadLock();
	~CThreadLock();
	void Lock(void);
	void Unlock(void);
private:
#ifdef _WIN32
	CRITICAL_SECTION	m_critical_section;
#else
	pthread_mutex_t 	m_mutex;
	pthread_mutexattr_t	m_mutexattr;
#endif
};

class CFuncLock
{
public:
	CFuncLock(CThreadLock* lock) 
	{ 
		m_lock = lock; 
		if (m_lock)
			m_lock->Lock(); 
	}

	~CFuncLock() 
	{ 
		if (m_lock)
			m_lock->Unlock(); 
	}
private:
	CThreadLock*	m_lock;
};

class CRefObject
{
public:
	CRefObject();
	virtual ~CRefObject();

	void SetLock(CThreadLock* lock) { m_lock = lock; }
	void AddRef();
	void ReleaseRef();
private:
	int				m_refCount;
	CThreadLock*	m_lock;
};

class CSimpleBuffer
{
public:
	CSimpleBuffer();
	~CSimpleBuffer();
	uchar_t*  GetBuffer() { return m_buffer; }
	uint32_t GetAllocSize() { return m_alloc_size; }
	uint32_t GetWriteOffset() { return m_write_offset; }
	void IncWriteOffset(uint32_t len) { m_write_offset += len; }

	void Extend(uint32_t len);
	uint32_t Write(void* buf, uint32_t len);
	uint32_t Read(void* buf, uint32_t len);
private:
	uchar_t*	m_buffer;
	uint32_t	m_alloc_size;
	uint32_t	m_write_offset;
};

class CByteStream
{
public:
	CByteStream(uchar_t* buf, uint32_t len);
	CByteStream(CSimpleBuffer* pSimpBuf, uint32_t pos);
	~CByteStream() {}

	unsigned char* GetBuf() { return m_pBuf; }
	uint32_t GetPos() { return m_pos; }
	uint32_t GetLen() { return m_len; }

	static int16_t ReadInt16(uchar_t* buf);
	static uint16_t ReadUint16(uchar_t* buf);
	static int32_t ReadInt32(uchar_t* buf);
	static uint32_t ReadUint32(uchar_t* buf);
	static void WriteInt16(uchar_t* buf, int16_t data);
	static void WriteUint16(uchar_t* buf, uint16_t data);
	static void WriteInt32(uchar_t* buf, int32_t data);
	static void WriteUint32(uchar_t* buf, uint32_t data);

	void operator << (int8_t data);
	void operator << (uint8_t data);
	void operator << (int16_t data);
	void operator << (uint16_t data);
	void operator << (int32_t data);
	void operator << (uint32_t data);

	void operator >> (int8_t& data);
	void operator >> (uint8_t& data);
	void operator >> (int16_t& data);
	void operator >> (uint16_t& data);
	void operator >> (int32_t& data);
	void operator >> (uint32_t& data);

	void WriteString(const char* str);
	char* ReadString();
	void WriteStdString(string& s);
	char* ReadStdString();

	void WriteData(uchar_t* data, uint16_t len);
	void WriteData(uchar_t* data, uint32_t len);

	uchar_t* ReadData(uint16_t& len);
	uchar_t* ReadData(uint32_t& len);
private:
	void _WriteByte(void* buf, uint32_t len);
	void _ReadByte(void* buf, uint32_t len);
private:
	CSimpleBuffer*	m_pSimpBuf;
	uchar_t*		m_pBuf;
	uint32_t		m_len;
	uint32_t		m_pos;
};

void log(const char* fmt, ...);
uint32_t get_tick_count();
void util_sleep(uint32_t millisecond);

#endif
