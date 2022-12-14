
/*
 * Copyright 2007-2021 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */







/**
 *  @file StdThread.cpp
 *
 *  This file contains the StdThread class.
*/

#include <gmsec/util/StdThread.h>
#include <gmsec/util/sysutil.h>
#include <gmsec/internal/strutil.h>
#include <gmsec/internal/rawbuf.h>

#include <gmsec/internal/Log.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <iostream>


using namespace gmsec::util;


enum ThreadState
{
	INIT,
	LAUNCHING,	// thread starting
	FAILED,	// failed to launch
	ABORTED,	// destroyed without run
	ACTIVE,
	COMPLETE,
	DUMMY
};


static const int IS_DEBUG = 1;



static Mutex &getMutex()
{
	static Mutex mutex;
	return mutex;
}


static std::string errorInfo(int code, const char * action = 0)
{
	std::string tmp;
	getErrorString(code, tmp);

	char raw[256] = {0};
	rawbuf buffer(raw, sizeof(raw));
	std::ostream out(&buffer);

	out << code << ": " << tmp << std::ends;

	tmp = raw;

	if (action)
		std::cerr << "StdThread.cpp:" << action << ' ' << tmp << '\n';

	return tmp;
}


static std::string state_s(int s)
{
	static const char *name[] = { "INIT", "LAUNCHING", "FAILED", "ABORTED", "ACTIVE", "COMPLETE" };
	if (s >= INIT && s <= COMPLETE)
		return name[s];
	static bool warned = 0;
	if (!warned)
	{
		warned = true;
		LOG_WARNING << "state_s: invalid state " << s;
	}
	return "INVALID";
}


static bool isBigEndian()
{
	long x = 1;
	char * p = (char *) &x;
	return p[0] == 0;
}


static StdThread::id_t handleToID(const StdThread::handle_t in)
{
	StdThread::id_t out = 0;

	size_t count = sizeof(out);

	const GMSEC_U8 * from = (GMSEC_U8 *) &in;
	GMSEC_U8 * to = (GMSEC_U8 *) &out;

	if (sizeof(in) > sizeof(out))
	{
		// will copy the start of the pthread_t
		count = sizeof(in);
	}
	else if (sizeof(in) < sizeof(out))
	{
		// place in low order bytes
		if (isBigEndian())
			to += sizeof(out) - sizeof(in);
	}

	copyBytes(to, from, count);

	return out;
}


#if 0

/* If we really want to support an exact representation */

#include <gmsec/util/Buffer.h>

static std::string handleToString(const StdThread::handle_t in)
{
	std::string out;

	const GMSEC_U8 * raw = (GMSEC_U8 *) &in;
	DataBuffer buffer(raw, sizeof(in), false);
	convertBlobToBase16(buffer, out);

	return out;
}

#endif


void StdThread::zero()
{
	fHandle = 0;
	fInfo.reset(new Info());
	fJoined = false;
	fDetached = false;
}


void *StdThread::process0(void *arg)
{
	Invoker *invoker = (Invoker *) arg;

	try
	{
		invoker->invoke();
	}
	catch (std::exception &e)
	{
		LOG_ERROR << "StdThread.process0: caught " << e.what();
	}
	catch (...)
	{
		LOG_ERROR << "StdThread.process0: caught ...";
	}

	delete invoker;

	return 0;
}


std::string StdThread::describe(const char *tag)
{
	char raw[512];
	rawbuf buffer(raw, sizeof(raw));
	std::ostream os(&buffer);

	if (tag)
		os << tag;
	else
		os << "StdThread";

	os << ':' << this << ':' << state_s(fInfo->state) << ":id=" << fInfo->id << ':' << fName << std::ends;

	return &raw[0];
}


StdThread::StdThread()
	:
	fInvoker(0),
	fHandle(0),
	fJoined(false),
	fDetached(false)
{
	if (IS_DEBUG)
	{
		LOG_DEBUG << describe("StdThread.constructor");
	}
}


StdThread::~StdThread()
{
	if (IS_DEBUG)
	{
		LOG_DEBUG << describe("StdThread.destructor");
	}

	if (fInfo->is(INIT))
		fInfo->set(ABORTED);

	delete fInvoker;
	fInvoker = 0;

#ifdef WIN32
	if (fHandle != 0)
	{
		// ExitThread(fInfo->id);
		CloseHandle(fHandle);
	}
#else /* WIN32 */
	if (fHandle != 0)
	{
		if (!fJoined && !fDetached)
		{
			int code = pthread_detach(fHandle);
			if (code)
			{
				LOG_ERROR << describe("~StdThread") << ": pthread_detach => " << errorInfo(code, "~StdThread/pthread_detach");
			}
#if GMSEC_THREAD_DESTRUCTOR_CANCEL
			code = pthread_cancel(fHandle);
			if (code)
			{
				LOG_ERROR << describe("~StdThread") << ": pthread_cancel => " << errorInfo(code, "~StdThread/pthread_cancel");
			}
#endif
		}
	}
#endif /* WIN32 */
}


int StdThread::start()
{
	if (!fInvoker)
	{
		LOG_WARNING << "Attempted to start empty thread";
		return 0;
	}

#ifdef WIN32
	if (fHandle != 0)
	{
		// ExitThread(fInfo->id);
		CloseHandle(fHandle);
		fHandle = 0;
	}
#else
	if (fHandle != 0)
	{
		LOG_DEBUG << describe() << "start: fHandle=" << handleToID(fHandle) << ", cancelling\n";
		pthread_detach(fHandle);
		pthread_cancel(fHandle);
		fHandle = 0;
	}
#endif

	// Ownership of the Invoker will be passed to process0 if the thread
	// is successfully started.  If it is not successfully started, 
	// ownership will be returned to this StdThread.
	Invoker *invoker = fInvoker;
	fInvoker = 0;
	int result = GMSEC_FALSE;

	fInfo->set(LAUNCHING);
	invoker->setShared(fInfo);

#ifdef WIN32

	DWORD tmpID = 0;
	LPSECURITY_ATTRIBUTES securityAttributes
		= THREAD_SUSPEND_RESUME & THREAD_TERMINATE;
	SIZE_T stackSize = 0; // 0 => default stack size
	DWORD creationFlags = 0;

	fHandle = CreateThread(
			securityAttributes,
			stackSize,
			(LPTHREAD_START_ROUTINE) &StdThread::process0,
			invoker, // argument for new thread
			creationFlags,
			&tmpID // pointer to returned thread identifier
			);

	if (tmpID)
		result = GMSEC_TRUE;

#else

	int err;

	pthread_attr_t attr;

	err = pthread_attr_init(&attr);
	if (err)
	{
		LOG_ERROR << describe() << ": pthread_attr_init => " << errorInfo(err, "start/pthread_attr_init");
		goto finish;
	}

#ifndef __hpux
	err = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	if (err)
	{
		LOG_ERROR << describe() << ": pthread_attr_setinheritsched => " << errorInfo(err, "start/pthread_setinheritsched");
	}
#endif

	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (err)
	{
		LOG_ERROR << describe() << ": pthread_attr_setdetachstate => " << errorInfo(err, "start/pthread_attr_setdetachstate");
	}

	err = pthread_create(&fHandle, &attr, &StdThread::process0, invoker);
	if (err)
	{
		LOG_ERROR << describe() << ": pthread_create => " << errorInfo(err, "start/pthread_create");
	}

	// free the attr (no longer needed)
	pthread_attr_destroy(&attr);

	result = (err == 0) ? GMSEC_TRUE : GMSEC_FALSE;

finish:

#endif

	if (result != GMSEC_TRUE)
	{
		// If unable to start the thread, then this object reassumes ownership
		// of the Invoker.
		fInfo->set(FAILED);
		fInvoker = invoker;
	}

	return result;
}


bool StdThread::isCurrentContext()
{
#ifdef WIN32

	DWORD tmp = GetCurrentThreadId();

	return fInfo->id == tmp;

#else

	pthread_t tmp = pthread_self();

	return pthread_equal(fHandle, tmp);

#endif
}


StdThread::id_t StdThread::getID()
{
	return fInfo->id;
}


StdThread::id_t StdThread::getCurrentID()
{
	id_t id;

#ifdef WIN32

	id = (id_t) GetCurrentThreadId();

#else

	handle_t tmp = pthread_self();
	id = handleToID(tmp);

#endif

	return id;
}


int StdThread::join()
{
	if (fJoined || fDetached)
	{
		LOG_VERBOSE << describe() << ": join: already joined/detached";
		//Change this to return successfully (return code 1) since if the thread is joined
		//we're at the state we want to be and if we're detached the target thread has already
		//completed and the thread that called join should continue. -MEH
		return 1;
	}

#ifdef WIN32

	WaitForSingleObject(fHandle, INFINITE);

#else

	void *nothing = 0;
	int code = pthread_join(fHandle, &nothing);
	if (code)
	{
		LOG_ERROR << describe() << ": pthread_join => " << errorInfo(code, "pthread_join");
	}

#endif /* WIN32 */

	fJoined = true;

	return 1;
}


int StdThread::detach()
{
	if (fDetached || fJoined)
	{
		LOG_WARNING << describe() << ": detach: already joined/detached";
		return 0;
	}

	fDetached = true;

#ifdef WIN32
       // nothing
#else
       int code = pthread_detach(fHandle);
       if (code)
	{
               LOG_ERROR << describe() << ": pthread_detach => " << errorInfo(code, "pthread_detach");
	}
#endif

	return 1;
}


void StdThread::yield()
{
#ifdef WIN32
	Sleep(0);
#else
	sched_yield();
#endif
}


void StdThread::setName(const char * name)
{
	fName = name;
}


void StdThread::Invoker::setShared(gshared_ptr<Info> &in)
{
	info = in;
}


void StdThread::Invoker::invoke()
{
	AutoMutex hold(getMutex());
	if (info->is(LAUNCHING))
	{
		info->set(ACTIVE);
		info->id = getCurrentID();
		hold.leave();
		_invoke();
	}
	else
		LOG_WARNING << "Invoker.invoke: not LAUNCHING";
}


void StdThread::Invoker::_invoke()
{
	LOG_WARNING << "Invoker._invoke: must be overridden";
}


StdThread::Info::Info()
	: state(INIT), id(0)
{
}


void StdThread::Info::set(int i)
{
	AutoMutex hold(getMutex());
	state = i;
}


bool StdThread::Info::is(int i) const
{
	AutoMutex hold(getMutex());
	return state == i;
}

