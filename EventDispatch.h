/*
 * A socket event dispatcher, features include: 
 * 1. portable: worked both on Windows, MAC OS X,  LINUX platform
 * 2. a singleton pattern: only one instance of this class can exist
 */
#ifndef __EVENT_DISPATCH_H__
#define __EVENT_DISPATCH_H__

#include "ostype.h"
#include "util.h"

enum {
	SOCKET_READ		= 0x1,
	SOCKET_WRITE	= 0x2,
	SOCKET_EXCEP	= 0x4,
	SOCKET_ALL		= 0x7
};

class CEventDispatch
{
public:
	virtual ~CEventDispatch();

	void AddEvent(SOCKET fd, uint8_t socket_event);
	void RemoveEvent(SOCKET fd, uint8_t socket_event);

	void AddTimer(callback_t callback, void* user_data, uint32_t interval);
	void RemoveTimer(callback_t callback, void* user_data);

	void StartDispatch();

	static CEventDispatch* Instance();
protected:
	CEventDispatch();

private:
	void _CheckTimer();

	typedef struct {
		callback_t	callback;
		void*		user_data;
		uint32_t	interval;
		uint32_t	next_tick;
	} TimerItem;

private:
#ifdef _WIN32
	fd_set	m_read_set;
	fd_set	m_write_set;
	fd_set	m_excep_set;
#elif __APPLE__
	int 	m_kqfd;
#else
	int		m_epfd;
#endif
	CThreadLock			m_lock;
	list<TimerItem*>	m_timer_list;

	static CEventDispatch* m_pEventDispatch;
};

#endif
