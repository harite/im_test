#include "util.h"

CThread::CThread()
{
	m_thread_id = 0;
}

CThread::~CThread()
{
}

#ifdef _WIN32
DWORD WINAPI CThread::StartRoutine(LPVOID arg)	
#else
void* CThread::StartRoutine(void* arg)
#endif
{
	CThread* pThread = (CThread*)arg;
	
	pThread->OnThreadRun();

#ifdef _WIN32
	return 0;
#else
	return NULL;
#endif
}

void CThread::StartThread()
{
#ifdef _WIN32
	(void)CreateThread(NULL, 0, StartRoutine, this, 0, &m_thread_id);
#else
	(void)pthread_create(&m_thread_id, NULL, StartRoutine, this);
#endif
}

CEventThread::CEventThread()
{
	m_bRunning = false;
}

CEventThread::~CEventThread()
{
	StopThread();
}

void CEventThread::StartThread()
{
	m_bRunning = true;
	CThread::StartThread();
}

void CEventThread::StopThread()
{
	m_bRunning = false;
}

void CEventThread::OnThreadRun()
{
	while (m_bRunning)
	{
		OnThreadTick();
	}
}

/////////// CThreadLock ///////////
CThreadLock::CThreadLock()
{
#ifdef _WIN32
	InitializeCriticalSection(&m_critical_section);
#else
	pthread_mutexattr_init(&m_mutexattr);
	pthread_mutexattr_settype(&m_mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &m_mutexattr);
#endif
}

CThreadLock::~CThreadLock()
{
#ifdef _WIN32
	DeleteCriticalSection(&m_critical_section);
#else
	pthread_mutexattr_destroy(&m_mutexattr);
	pthread_mutex_destroy(&m_mutex);
#endif
}

void CThreadLock::Lock(void)
{
#ifdef _WIN32
	EnterCriticalSection(&m_critical_section);
#else
	pthread_mutex_lock(&m_mutex);
#endif
}

void CThreadLock::Unlock(void)
{
#ifdef _WIN32
	LeaveCriticalSection(&m_critical_section);
#else
	pthread_mutex_unlock(&m_mutex);
#endif
}

CRefObject::CRefObject()
{
	m_lock = NULL;
	m_refCount = 1;
}

CRefObject::~CRefObject()
{

}

void CRefObject::AddRef()
{
	if (m_lock)
	{
		m_lock->Lock();
		m_refCount++;
		m_lock->Unlock();
	}
	else
	{
		m_refCount++;
	}
}

void CRefObject::ReleaseRef()
{
	if (m_lock)
	{
		m_lock->Lock();
		m_refCount--;
		if (m_refCount == 0)
		{
			delete this;
			return;
		}
		m_lock->Unlock();
	}
	else
	{
		m_refCount--;
		if (m_refCount == 0)
			delete this;
	}
}


///////////// CSimpleBuffer ////////////////
CSimpleBuffer::CSimpleBuffer()
{
	m_buffer = NULL;

	m_alloc_size = 0;
	m_write_offset = 0;
}

CSimpleBuffer::~CSimpleBuffer()
{
	m_alloc_size = 0;
	m_write_offset = 0;
	if (m_buffer)
	{
		free(m_buffer);
		m_buffer = NULL;
	}
}

void CSimpleBuffer::Extend(uint32_t len)
{
	m_alloc_size = m_write_offset + len;
	m_alloc_size += m_alloc_size >> 2;	// increase by 1/4 allocate size
	uchar_t* new_buf = (uchar_t*)realloc(m_buffer, m_alloc_size);
	m_buffer = new_buf;
}

uint32_t CSimpleBuffer::Write(void* buf, uint32_t len)
{
	if (m_write_offset + len > m_alloc_size)
	{
		Extend(len);
	}

	if (buf)
	{
		memcpy(m_buffer + m_write_offset, buf, len);
	}
	
	m_write_offset += len;

	return len;
}

uint32_t CSimpleBuffer::Read(void* buf, uint32_t len)
{
	if (len > m_write_offset)
		len = m_write_offset;

	if (buf)
		memcpy(buf, m_buffer, len);

	m_write_offset -= len;
	memmove(m_buffer, m_buffer + len, m_write_offset);
	return len;
}

////// CByteStream //////
CByteStream::CByteStream(uchar_t* buf, uint32_t len)
{
	m_pBuf = buf;
	m_len = len;
	m_pSimpBuf = NULL;
	m_pos = 0;
}

CByteStream::CByteStream(CSimpleBuffer* pSimpBuf, uint32_t pos)
{
	m_pSimpBuf = pSimpBuf;
	m_pos = pos;
	m_pBuf = NULL;
	m_len = 0;
}

int16_t CByteStream::ReadInt16(uchar_t *buf)
{
	int16_t data = (buf[0] << 8) | buf[1];
	return data;
}

uint16_t CByteStream::ReadUint16(uchar_t* buf)
{
	uint16_t data = (buf[0] << 8) | buf[1];
	return data;
}

int32_t CByteStream::ReadInt32(uchar_t *buf)
{
	int32_t data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	return data;
}

uint32_t CByteStream::ReadUint32(uchar_t *buf)
{
	uint32_t data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	return data;
}

void CByteStream::WriteInt16(uchar_t *buf, int16_t data)
{
	buf[0] = static_cast<uchar_t>(data >> 8);
	buf[1] = static_cast<uchar_t>(data & 0xFF);
}

void CByteStream::WriteUint16(uchar_t *buf, uint16_t data)
{
	buf[0] = static_cast<uchar_t>(data >> 8);
	buf[1] = static_cast<uchar_t>(data & 0xFF);
}

void CByteStream::WriteInt32(uchar_t *buf, int32_t data)
{
	buf[0] = static_cast<uchar_t>(data >> 24);
	buf[1] = static_cast<uchar_t>((data >> 16) & 0xFF);
	buf[2] = static_cast<uchar_t>((data >> 8) & 0xFF);
	buf[3] = static_cast<uchar_t>(data & 0xFF);
}

void CByteStream::WriteUint32(uchar_t *buf, uint32_t data)
{
	buf[0] = static_cast<uchar_t>(data >> 24);
	buf[1] = static_cast<uchar_t>((data >> 16) & 0xFF);
	buf[2] = static_cast<uchar_t>((data >> 8) & 0xFF);
	buf[3] = static_cast<uchar_t>(data & 0xFF);
}

void CByteStream::operator << (int8_t data)
{
	_WriteByte(&data, 1);
}

void CByteStream::operator << (uint8_t data)
{
	_WriteByte(&data, 1);
}

void CByteStream::operator << (int16_t data)
{
	unsigned char buf[2];
	buf[0] = static_cast<uchar_t>(data >> 8);
	buf[1] = static_cast<uchar_t>(data & 0xFF);
	_WriteByte(buf, 2);
}

void CByteStream::operator << (uint16_t data)
{
	unsigned char buf[2];
	buf[0] = static_cast<uchar_t>(data >> 8);
	buf[1] = static_cast<uchar_t>(data & 0xFF);
	_WriteByte(buf, 2);
}

void CByteStream::operator << (int32_t data)
{
	unsigned char buf[4];
	buf[0] = static_cast<uchar_t>(data >> 24);
	buf[1] = static_cast<uchar_t>((data >> 16) & 0xFF);
	buf[2] = static_cast<uchar_t>((data >> 8) & 0xFF);
	buf[3] = static_cast<uchar_t>(data & 0xFF);
	_WriteByte(buf, 4);
}

void CByteStream::operator << (uint32_t data)
{
	unsigned char buf[4];
	buf[0] = static_cast<uchar_t>(data >> 24);
	buf[1] = static_cast<uchar_t>((data >> 16) & 0xFF);
	buf[2] = static_cast<uchar_t>((data >> 8) & 0xFF);
	buf[3] = static_cast<uchar_t>(data & 0xFF);
	_WriteByte(buf, 4);
}

void CByteStream::operator >> (int8_t& data)
{
	_ReadByte(&data, 1);
}

void CByteStream::operator >> (uint8_t& data)
{
	_ReadByte(&data, 1);
}

void CByteStream::operator >> (int16_t& data)
{
	unsigned char buf[2];

	_ReadByte(buf, 2);

	data = (buf[0] << 8) | buf[1];
}

void CByteStream::operator >> (uint16_t& data)
{
	unsigned char buf[2];

	_ReadByte(buf, 2);

	data = (buf[0] << 8) | buf[1];
}

void CByteStream::operator >> (int32_t& data)
{
	unsigned char buf[4];

	_ReadByte(buf, 4);

	data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

void CByteStream::operator >> (uint32_t& data)
{
	unsigned char buf[4];

	_ReadByte(buf, 4);

	data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

void CByteStream::WriteString(const char *str)
{
	uint8_t size = str ? (uint8_t)strlen(str) + 1 : 0;

	*this << size;
	_WriteByte((void*)str, size);
}

char* CByteStream::ReadString()
{
	uint8_t size = 0;
	*this >> size;
	char* pStr = (char*)malloc(size);
	_ReadByte(pStr, size);
	return pStr;
}

void CByteStream::WriteStdString(string& s)
{
	WriteString(s.c_str());
}

char* CByteStream::ReadStdString()
{
	uint8_t size = 0;
	*this >> size;	// WriteStdString() will always have at least 1 byte

	char* ptr = (char*)(m_pBuf + m_pos);
	m_pos += size;

	return ptr;
}

void CByteStream::WriteData(uchar_t *data, uint16_t len)
{
	*this << len;
	_WriteByte(data, len);
}

void CByteStream::WriteData(uchar_t *data, uint32_t len)
{
	*this << len;
	_WriteByte(data, len);
}

uchar_t* CByteStream::ReadData(uint16_t &len)
{
	*this >> len;
	uchar_t* pData = (uchar_t*)malloc(len);
	_ReadByte(pData, len);
	return pData;
}

uchar_t* CByteStream::ReadData(uint32_t &len)
{
	*this >> len;
	uchar_t* pData = (uchar_t*)malloc(len);
	_ReadByte(pData, len);
	return pData;
}

void CByteStream::_ReadByte(void* buf, uint32_t len)
{
	if (m_pos + len > m_len)
		return;

	if (m_pSimpBuf)
		m_pSimpBuf->Read((char*)buf, len);
	else
		memcpy(buf, m_pBuf + m_pos, len);

	m_pos += len;
}

void CByteStream::_WriteByte(void* buf, uint32_t len)
{
	if (m_pBuf && (m_pos + len > m_len))
		return;

	if (m_pSimpBuf)
		m_pSimpBuf->Write((char*)buf, len);
	else
		memcpy(m_pBuf + m_pos, buf, len);

	m_pos += len;
}

static void print_tread_id(FILE* fp)
{
	uint16_t thread_id = 0;
#ifdef _WIN32
	thread_id = (uint16_t)GetCurrentThreadId();
#elif __APPLE__
	thread_id = syscall(SYS_gettid);
#else
	thread_id = (uint16_t)pthread_self();
#endif
	fprintf(fp, "(tid=%d)", thread_id);
}

static void print_format_time(FILE* fp)
{
#ifdef _WIN32
	SYSTEMTIME systemTime;

	GetLocalTime(&systemTime);
	fprintf(fp, "%04d-%02d-%02d, %02d:%02d:%02d.%03d, ", systemTime.wYear, systemTime.wMonth, systemTime.wDay,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
#else
	struct timeval tval;
	struct tm* tm;
	time_t currTime;

	time(&currTime);
	tm = localtime(&currTime);
	gettimeofday(&tval, NULL);
	fprintf(fp, "%04d-%02d-%02d, %02d:%02d:%02d.%03d, ", 1900 + tm->tm_year, 1 + tm->tm_mon,
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tval.tv_usec / 1000);
#endif
}

static CThreadLock s_log_lock;

void log(const char* fmt, ...)
{
	CFuncLock func_lock(&s_log_lock);

	static int file_no = 1;
	static FILE* log_fp = NULL;
	if (log_fp == NULL)
	{
		char log_name[64];
		uint16_t pid = 0;
#ifdef _WIN32
		pid = (uint16_t)GetCurrentProcessId();
#else
		pid = (uint16_t)getpid();
#endif
		snprintf(log_name, 64, "log_%d_%d.txt", pid, file_no);
		log_fp = fopen(log_name, "w");
		if (log_fp == NULL)
			return;
	}

	print_tread_id(log_fp);
	print_format_time(log_fp);

	va_list ap;
	va_start(ap, fmt);
	vfprintf(log_fp, fmt, ap);
	va_end(ap);
	fflush(log_fp);

	if (ftell(log_fp) > MAX_LOG_FILE_SIZE)
	{
		fclose(log_fp);
		log_fp = NULL;
		file_no++;
	}
}

uint32_t get_tick_count()
{
#ifdef _WIN32
	LARGE_INTEGER liCounter; 
	LARGE_INTEGER liCurrent;

	if (!QueryPerformanceFrequency(&liCounter))
		return GetTickCount();

	QueryPerformanceCounter(&liCurrent);
	return (uint32_t)(liCurrent.QuadPart * 1000 / liCounter.QuadPart);
#else
	struct timeval tval;
	uint32_t ret_tick;

	gettimeofday(&tval, NULL);

	ret_tick = tval.tv_sec * 1000 + tval.tv_usec / 1000;
	return ret_tick;
#endif
}

void util_sleep(uint32_t millisecond)
{
#ifdef _WIN32
	Sleep(millisecond);
#else
	usleep(millisecond * 1000);
#endif
}

