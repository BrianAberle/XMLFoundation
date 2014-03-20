// --------------------------------------------------------------------------
//			John E. Bossom and United Business Technologies
//			  Copyright (c) 2013-2014  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
// This source is based on the 1998 port of pthreads to Win32 by John E. Bossom
// This source is based on a version that was used by the XMLFoundation from 2000 until July 2007.
// It has been modified to implement only the subset of functions listed in GThread.h
// It has been modified to build on Windows Phone over ThreadEmulation
// It has been modified to build on 32, and 64 bit Windows with old and new compilers.
// It has been modified to use non-depreciated Win32 interfaces.
// It has been modified to use non-pthread conflicting structures, defines, and function names.

// GThread is based on the 1998 port of pthreads, I picked up the source in 2000 2 years after many folks 
// had used it and debugged it.  Back then, pthreads for Win32 was incomplete by POSIX standards, it only 
// implemented the most common needs.  Millions of volunteers later, pThreads for Win32 was complete.
// The 'new' complete pthreads could never be ported to WinPhone.
// In 2014, GThread is the ONLY pthread port (subset of a pthread port) that supports all Windows platforms.
// and all Windows compilers as well. Changes have been made to the 2000 fork of pthreads regarding USE_DEPRECIATED_WIN32_THREADING


#include "GlobalInclude.h"
#include "GThread.h"
#include "errno.h"
#include <malloc.h>
#include <sys/timeb.h> // for _ftime
#include <process.h> // for _endthreadex
 
// global variables
int g_nextGThreadID = 1;
static HINSTANCE _h_kernel32;
static HINSTANCE _gthread_h_kernel32;


// Configure GThread to use the depreciated interface (the only interface in XP and older) when building for 32 bit Windows
// GThread will use: 
//					 InitializeCriticalSection() not InitializeCriticalSectionEx()
//					 CreateEvent()				 not CreateEventEx()
//					 CreateSemaphore()			 not CreateSemaphoreEx()
//					 CreateMutex()				 not CreateMutexEx()
//
//     only when USE_DEPRECIATED_WIN32_THREADING has been defined.
// 
//   IF       Visual Studio 6.0 or less     or  New Visual Studio targeting XP or older  or  Borland C++ Same era as VC6
#if defined(_MSC_VER) && (_MSC_VER <= 1200) || (_WIN32_WINNT <= 0x0501)					 ||  (__BORLANDC__)
	#define USE_DEPRECIATED_WIN32_THREADING
#endif



#ifndef ETIMEDOUT // for VC6
	#define ETIMEDOUT 10060
#endif

// for VC6 and older
#if defined(_MSC_VER) && _MSC_VER <= 1200  
	#define EXCEPTIONCAST 
#else
	#define EXCEPTIONCAST (ULONG_PTR *)
#endif



gthread_t gthread_self (void);
int _gthread_processInitialized = 0;

int sem_init (sem_t * sem,  int pshared,   unsigned int value );
int sem_destroy (sem_t * sem );
int sem_post (sem_t * sem );



#ifndef TLS_OUT_OF_INDEXES
	#define TLS_OUT_OF_INDEXES  0xffffffff
#endif

#ifdef __WINPHONE
struct ThreadProxyData 
{ 
	typedef unsigned (__stdcall* func)(void*); 
	func start_address_; 
	void* arglist_; 
	ThreadProxyData(func start_address,void* arglist) : start_address_(start_address), arglist_(arglist) {} 
}; 
 
DWORD WINAPI ThreadProxy(LPVOID args) 
{ 
	ThreadProxyData* data=reinterpret_cast<ThreadProxyData*>(args); 
	DWORD ret=data->start_address_(data->arglist_); 
	delete data; 
	return ret; 
} 


void *_beginthreadex(void* security, unsigned stack_size, unsigned (__stdcall* start_address)(void*),  void* arglist, unsigned initflag, unsigned* thrdaddr) 
{ 
	DWORD threadID; 
	HANDLE hthread=CreateThread(static_cast<LPSECURITY_ATTRIBUTES>(security),stack_size,ThreadProxy, 
	new ThreadProxyData(start_address,arglist),initflag,&threadID); 
	if (hthread!=0) 
		*thrdaddr=threadID; 
	return hthread; 
}
#endif



#define GTHREAD_PROCESS_SHARED		1
#define _GTHREAD_OBJECT_AUTO_INIT ((void *) -1)
#define GTHREAD_CANCEL_ENABLE		0
#define _GTHREAD_EPS_CANCEL       0
#define PT_STDCALL __stdcall
#define GTHREAD_CANCELED       ((void *) -1)

/*
 * Global lock for testing internal state of GTHREAD_COND_INITIALIZER
 * created condition variables. 
 */
CRITICAL_SECTION _gthread_cond_test_init_lock;
CRITICAL_SECTION _gthread_mutex_test_init_lock;
CRITICAL_SECTION _gthread_rwlock_test_init_lock;


void initGThread()
{
	// In VC 6.0 and older use the depreciated interface
	#ifdef USE_DEPRECIATED_WIN32_THREADING
	  InitializeCriticalSection(&_gthread_cond_test_init_lock);
	#else
	  InitializeCriticalSectionEx(&_gthread_cond_test_init_lock,2,0);
	#endif
}
void destroyGThread()
{
    DeleteCriticalSection(&_gthread_cond_test_init_lock);
}


int gthread_cond_init (gthread_cond_t * cond, const gthread_condattr_t * attr)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function initializes a condition variable.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of gthread_cond_t
      *
      *      attr
      *              specifies optional creation attributes.
      *
      *
      * DESCRIPTION
      *      This function initializes a condition variable.
      *
      * RESULTS
      *              0               successfully created condition variable,
      *              EINVAL          'attr' is invalid,
      *              EAGAIN          insufficient resources (other than
      *                              memory,
      *              ENOMEM          insufficient memory,
      *              EBUSY           'cond' is already initialized,
      *
      * ------------------------------------------------------
      */
{
  int result = EAGAIN;
  gthread_cond_t cv = NULL;

  if (cond == NULL)
    {
      return EINVAL;
    }

  if ((attr != NULL && *attr != NULL) &&
      ((*attr)->pshared == GTHREAD_PROCESS_SHARED))
    {
      /*
       * Creating condition variable that can be shared between
       * processes.
       */
      result = ENOSYS;

      goto FAIL0;
    }

  cv = (gthread_cond_t) calloc (1, sizeof (*cv));

  if (cv == NULL)
    {
      result = ENOMEM;
      goto FAIL0;
    }

  cv->waiters = 0;
  cv->wasBroadcast = FALSE;

  if (sem_init (&(cv->sema), 0, 0) != 0)
    {
      goto FAIL0;
    }
  if (gthread_mutex_init (&(cv->waitersLock), NULL) != 0)
    {
      goto FAIL1;
    }

	// In VC 6.0 and older use the depreciated interface
	#ifdef USE_DEPRECIATED_WIN32_THREADING
	  cv->waitersDone = CreateEvent (
					 0,
					 (int) FALSE,  /* manualReset  */
					 (int) FALSE,  /* setSignaled  */
					 NULL);
	#else
	  cv->waitersDone = CreateEventEx(
		  0,
		  NULL,
		  CREATE_EVENT_INITIAL_SET | CREATE_EVENT_MANUAL_RESET,  
		  EVENT_ALL_ACCESS);
	#endif
  if (cv->waitersDone == NULL)
    {
      goto FAIL2;
    }

  result = 0;

  goto DONE;

  /*
   * -------------
   * Failure Code
   * -------------
   */
FAIL2:
  (void) gthread_mutex_destroy (&(cv->waitersLock));

FAIL1:
  (void) sem_destroy (&(cv->sema));

FAIL0:
DONE:
  *cond = cv;

  return (result);

}  /* gthread_cond_init */

int gthread_cond_destroy (gthread_cond_t * cond)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function destroys a condition variable
      *
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of gthread_cond_t
      *
      *
      * DESCRIPTION
      *      This function destroys a condition variable.
      *
      *      NOTES:
      *              1)      Safest after wakeup from 'cond', when
      *                      no other threads will wait.
      *
      * RESULTS
      *              0               successfully released condition variable,
      *              EINVAL          'cond' is invalid,
      *              EBUSY           'cond' is in use,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  gthread_cond_t cv;

  /*
   * Assuming any race condition here is harmless.
   */
  if (cond == NULL 
      || *cond == NULL)
    {
      return EINVAL;
    }

  if (*cond != (gthread_cond_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      cv = *cond;

      if (gthread_mutex_lock(&(cv->waitersLock)) != 0)
	{
	  return EINVAL;
	}

      if (cv->waiters > 0)
	{
	  (void) gthread_mutex_unlock(&(cv->waitersLock));
	  return EBUSY;
	}

      (void) sem_destroy (&(cv->sema));
      (void) CloseHandle (cv->waitersDone);
      (void) gthread_mutex_unlock(&(cv->waitersLock));
      (void) gthread_mutex_destroy (&(cv->waitersLock));

      free(cv);
      *cond = NULL;
    }
  else
    {
      /*
       * See notes in _cond_check_need_init() above also.
       */
      EnterCriticalSection(&_gthread_cond_test_init_lock);

      /*
       * Check again.
       */
      if (*cond == (gthread_cond_t) _GTHREAD_OBJECT_AUTO_INIT)
        {
          /*
           * This is all we need to do to destroy a statically
           * initialised cond that has not yet been used (initialised).
           * If we get to here, another thread
           * waiting to initialise this cond will get an EINVAL.
           */
          *cond = NULL;
        }
      else
        {
          /*
           * The cv has been initialised while we were waiting
           * so assume it's in use.
           */
          result = EBUSY;
        }

      LeaveCriticalSection(&_gthread_cond_test_init_lock);
    }

  return (result);
} // gthread_cond_destroy





/*
 * Arguments for cond_wait_cleanup, since we can only pass a
 * single void * to it.
 */
typedef struct {
  gthread_mutex_t * mutexPtr;
  gthread_cond_t cv;
  int * resultPtr;
} cond_wait_cleanup_args_t;

static void
cond_wait_cleanup(void * args)
{
  cond_wait_cleanup_args_t * cleanup_args = (cond_wait_cleanup_args_t *) args;
  gthread_mutex_t * mutexPtr = cleanup_args->mutexPtr;
  gthread_cond_t cv = cleanup_args->cv;
  int * resultPtr = cleanup_args->resultPtr;
  int lock_result;
  int lastWaiter = 0;

  if ((lock_result = gthread_mutex_lock (&(cv->waitersLock))) == 0)
    {
      /*
       * The waiter is responsible for decrementing
       * its count, protected by an internal mutex.
       */

      cv->waiters--;

      lastWaiter = cv->wasBroadcast && (cv->waiters == 0);

      if (lastWaiter)
        {
          cv->wasBroadcast = FALSE;
        }

      lock_result = gthread_mutex_unlock (&(cv->waitersLock));
    }

  if ((*resultPtr == 0 || *resultPtr == ETIMEDOUT) && lock_result == 0)
    {
      if (lastWaiter)
        {
          /*
           * If we are the last waiter on this broadcast
           * let the thread doing the broadcast proceed
           */
          if (!SetEvent (cv->waitersDone))
            {
              *resultPtr = EINVAL;
            }
        }
    }

  /*
   * We must always regain the external mutex, even when
   * errors occur, because that's the guarantee that we give
   * to our callers
   */
  (void) gthread_mutex_lock (mutexPtr);
}

int gthread_cond_signal (gthread_cond_t * cond)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function signals a condition variable, waking
      *      one waiting thread.
      *      If SCHED_FIFO or SCHED_RR policy threads are waiting
      *      the highest priority waiter is awakened; otherwise,
      *      an unspecified waiter is awakened.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of gthread_cond_t
      *
      *
      * DESCRIPTION
      *      This function signals a condition variable, waking
      *      one waiting thread.
      *      If SCHED_FIFO or SCHED_RR policy threads are waiting
      *      the highest priority waiter is awakened; otherwise,
      *      an unspecified waiter is awakened.
      *
      *      NOTES:
      *      1)      Use when any waiter can respond and only one need
      *              respond (all waiters being equal).
      *
      *      2)      This function MUST be called under the protection 
      *              of the SAME mutex that is used with the condition
      *              variable being signaled; OTHERWISE, the condition
      *              variable may be signaled between the test of the
      *              associated condition and the blocking
      *              gthread_cond_signal.
      *              This can cause an infinite wait.
      *
      * RESULTS
      *              0               successfully signaled condition,
      *              EINVAL          'cond' is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  gthread_cond_t cv;

  if (cond == NULL || *cond == NULL)
    {
      return EINVAL;
    }

  cv = *cond;

  /*
   * No-op if the CV is static and hasn't been initialised yet.
   * Assuming that race conditions are harmless.
   */
  if (cv == (gthread_cond_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      return 0;
    }

  /*
   * If there aren't any waiters, then this is a no-op.
   * Assuming that race conditions are harmless.
   */
  if (cv->waiters > 0)
    {
      result = sem_post (&(cv->sema));
    }

  return (result);

}                               /* gthread_cond_signal */


static int
_cond_check_need_init(gthread_cond_t *cond)
{
  int result = 0;

  /*
   * The following guarded test is specifically for statically
   * initialised condition variables (via GTHREAD_OBJECT_INITIALIZER).
   *
   * Note that by not providing this synchronisation we risk
   * introducing race conditions into applications which are
   * correctly written.
   *
   * Approach
   * --------
   * We know that static condition variables will not be PROCESS_SHARED
   * so we can serialise access to internal state using
   * Win32 Critical Sections rather than Win32 Mutexes.
   *
   * If using a single global lock slows applications down too much,
   * multiple global locks could be created and hashed on some random
   * value associated with each mutex, the pointer perhaps. At a guess,
   * a good value for the optimal number of global locks might be
   * the number of processors + 1.
   *
   */
  EnterCriticalSection(&_gthread_cond_test_init_lock);

  /*
   * We got here possibly under race
   * conditions. Check again inside the critical section.
   * If a static cv has been destroyed, the application can
   * re-initialise it only by calling gthread_cond_init()
   * explicitly.
   */
  if (*cond == (gthread_cond_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      result = gthread_cond_init(cond, NULL);
    }
  else if (*cond == NULL)
    {
      /*
       * The cv has been destroyed while we were waiting to
       * initialise it, so the operation that caused the
       * auto-initialisation should fail.
       */
      result = EINVAL;
    }

  LeaveCriticalSection(&_gthread_cond_test_init_lock);

  return(result);
}


struct _gthread_cleanup_t
{
  void (*routine) (void *);
  void *arg;
#if !defined(_MSC_VER) && !defined(__cplusplus)
  _gthread_cleanup_t *prev;
#endif /* !_MSC_VER && ! __cplusplus */
};

#define gthread_cleanup_push( _rout, _arg ) \
	{ \
	    _gthread_cleanup_t	_cleanup; \
	    \
            _cleanup.routine	= (_rout); \
	    _cleanup.arg	= (_arg); \
	    __try \
	      { \





struct gthread_key_t_ {
  DWORD key;
  void (*destructor) (void *);
  gthread_mutex_t threadsLock;
  void *threads;
};
typedef struct gthread_key_t_ *gthread_key_t;

gthread_key_t _gthread_selfThreadKey = NULL;
gthread_key_t _gthread_cleanupKey = NULL;

void *
gthread_getspecific (gthread_key_t key)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function returns the current value of key in the
      *      calling thread. If no value has been set for 'key' in 
      *      the thread, NULL is returned.
      *
      * PARAMETERS
      *      key
      *              an instance of gthread_key_t
      *
      *
      * DESCRIPTION
      *      This function returns the current value of key in the
      *      calling thread. If no value has been set for 'key' in 
      *      the thread, NULL is returned.
      *
      * RESULTS
      *              key value
      *
      * ------------------------------------------------------
      */
{
  return (TlsGetValue (key->key));
}

/*
 * --------------------------------------------------------------
 * MAKE_SOFTWARE_EXCEPTION
 *      This macro constructs a software exception code following
 *      the same format as the standard Win32 error codes as defined
 *      in WINERROR.H
 *  Values are 32 bit values layed out as follows:
 *
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +---+-+-+-----------------------+-------------------------------+
 *  |Sev|C|R|     Facility          |               Code            |
 *  +---+-+-+-----------------------+-------------------------------+
 *
 * Severity Values:
 */
#define SE_SUCCESS              0x00
#define SE_INFORMATION	        0x01
#define SE_WARNING              0x02
#define SE_ERROR                0x03

#define MAKE_SOFTWARE_EXCEPTION( _severity, _facility, _exception ) \
( (DWORD) ( ( (_severity) << 30 ) |	/* Severity code	*/ \
	    ( 1 << 29 )	|		/* MS=0, User=1		*/ \
	    ( 0 << 28 )	|		/* Reserved		*/ \
	    ( (_facility) << 16 ) |	/* Facility Code	*/ \
	    ( (_exception) <<  0 )	/* Exception Code	*/ \
	    ) )

/*
 * We choose one specific Facility/Error code combination to
 * identify our software exceptions vs. WIN32 exceptions.
 * We store our actual component and error code within
 * the optional information array.
 */
#define EXCEPTION_GTHREAD_SERVICES	\
     MAKE_SOFTWARE_EXCEPTION( SE_ERROR, \
			      _GTHREAD_SERVICES_FACILITY, \
			      _GTHREAD_SERVICES_ERROR )

#define _GTHREAD_SERVICES_FACILITY		0xBAD
#define _GTHREAD_SERVICES_ERROR			0xDEED



static int
CancelableWait (HANDLE waitHandle, DWORD timeout)
     /*
      * -------------------------------------------------------------------
      * This provides an extra hook into the gthread_cancel
      * mechanism that will allow you to wait on a Windows handle and make it a
      * cancellation point. This function blocks until the given WIN32 handle is
      * signaled or gthread_cancel has been called. It is implemented using
      * WaitForMultipleObjects on 'waitHandle' and a manually reset WIN32
      * event used to implement gthread_cancel.
      * 
      * Given this hook it would be possible to implement more of the cancellation
      * points.
      * -------------------------------------------------------------------
      */
{
  int result;
  gthread_t self;
  HANDLE handles[2];
  DWORD nHandles = 1;
  DWORD status;

  handles[0] = waitHandle;

  if ((self = (gthread_t) gthread_getspecific (_gthread_selfThreadKey)) 
      != NULL)
    {
      /*
       * Get cancelEvent handle
       */
      if (self->cancelState == GTHREAD_CANCEL_ENABLE)
        {

          if ((handles[1] = self->cancelEvent) != NULL)
            {
              nHandles++;
            }
        }
    }
  else
    {
      handles[1] = NULL;
    }

 status = WaitForMultipleObjectsEx(
	  nHandles,
	  handles,
	  FALSE,
	  timeout,
	  false
	  );


  if (status == WAIT_FAILED)
    {
      result = EINVAL;

    }
  else if (status == WAIT_TIMEOUT)
    {
      result = ETIMEDOUT;
    }
  else if (status == WAIT_ABANDONED_0)
    {
      result = EINVAL;
    }
  else
    {
      /*
       * Either got the mutex or the cancel event
       * was signaled
       */
      switch (status - WAIT_OBJECT_0)
        {

        case 0:
          /*
           * Got the mutex
           */
          result = 0;
          break;

        case 1:
          /*
           * Got cancel request
           */
          ResetEvent (handles[1]);

          if (self != NULL && !self->implicit)
            {
              /*
               * Thread started with gthread_create
               */

#ifdef _MSC_VER

              DWORD exceptionInformation[3];

              exceptionInformation[0] = (DWORD) (_GTHREAD_EPS_CANCEL);
              exceptionInformation[1] = (DWORD) (0);
              exceptionInformation[2] = (DWORD) (0);

              RaiseException (
                               EXCEPTION_GTHREAD_SERVICES,
                               0,
                               3,
                               EXCEPTIONCAST exceptionInformation);

#else /* _MSC_VER */

#ifdef __cplusplus

	      throw Pthread_exception_cancel();

#endif /* __cplusplus */

#endif /* _MSC_VER */
            }

         /* Should never get to here. */
         result = EINVAL;
          break;

        default:
          result = EINVAL;
          break;
        }
    }

  return (result);

}                               /* gthreadCancelableWait */

int
gthreadCancelableWait (HANDLE waitHandle)
{
  return (CancelableWait(waitHandle, INFINITE));
}

int
gthreadCancelableTimedWait (HANDLE waitHandle, DWORD timeout)
{
  return (CancelableWait(waitHandle, timeout));
}






int
_gthread_sem_timedwait (sem_t * sem, const struct gtimespec * abstime)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits on a semaphore possibly until
      *      'abstime' time.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      *      abstime
      *              pointer to an instance of struct gtimespec
      *
      * DESCRIPTION
      *      This function waits on a semaphore. If the
      *      semaphore value is greater than zero, it decreases
      *      its value by one. If the semaphore value is zero, then
      *      the calling thread (or process) is blocked until it can
      *      successfully decrease the value or until interrupted by
      *      a signal.
      *
      *      If 'abstime' is a NULL pointer then this function will
      *      block until it can successfully decrease the value or
      *      until interrupted by a signal.
      *
      * RESULTS
      *              0               successfully decreased semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              EINTR           the function was interrupted by a signal,
      *              EDEADLK         a deadlock condition was detected.
      *              ETIMEDOUT       abstime elapsed before success.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

#if defined(__MINGW32__)

  struct timeb currSysTime;

#else

  struct _timeb currSysTime;

#endif

  const DWORD NANOSEC_PER_MILLISEC = 1000000;
  const DWORD MILLISEC_PER_SEC = 1000;
  DWORD milliseconds;

  if (sem == NULL)
    {
      result = EINVAL;
    }
  else
    {
      if (abstime == NULL)
	{
	  milliseconds = INFINITE;
	}
      else
	{
	  /* 
	   * Calculate timeout as milliseconds from current system time. 
	   */

	  /* get current system time */
	  #if defined(_MSC_VER) && _MSC_VER <= 1200  
	    _ftime(&currSysTime);
	  #else
	    _ftime64_s(&currSysTime);
	  #endif


	  /* subtract current system time from abstime */
	  milliseconds = (DWORD)(abstime->tv_sec - currSysTime.time) * MILLISEC_PER_SEC;
	  milliseconds += (abstime->tv_nsec / NANOSEC_PER_MILLISEC) -  currSysTime.millitm;

	  if (((int) milliseconds) < 0)
	    milliseconds = 0;
	}

      result = (gthreadCancelableTimedWait (*sem, milliseconds));
    }

  if (result != 0)
    {

      errno = result;
      return -1;

    }

  return 0;

}				/* _gthread_sem_timedwait */

#define gthread_cleanup_push( _rout, _arg ) \
	{ \
	    _gthread_cleanup_t	_cleanup; \
	    \
            _cleanup.routine	= (_rout); \
	    _cleanup.arg	= (_arg); \
	    __try \
	      { \

#define gthread_cleanup_pop( _execute ) \
	      } \
	    __finally \
		{ \
		    if( _execute || AbnormalTermination()) \
		      { \
			  (*(_cleanup.routine))( _cleanup.arg ); \
		      } \
		} \
	}


static int
cond_timedwait (gthread_cond_t * cond, 
		gthread_mutex_t * mutex,
		const struct gtimespec *abstime)
{
  int result = 0;
//  int internal_result = 0;
//  int lastWaiter = FALSE;
  gthread_cond_t cv;
  cond_wait_cleanup_args_t cleanup_args;

  if (cond == NULL || *cond == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static condition variable. We check
   * again inside the guarded section of _cond_check_need_init()
   * to avoid race conditions.
   */
  if (*cond == (gthread_cond_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      result = _cond_check_need_init(cond);
    }

  if (result != 0 && result != EBUSY)
    {
      return result;
    }

  cv = *cond;

  /*
   * It's not OK to increment cond->waiters while the caller locked 'mutex',
   * there may be other threads just waking up (with 'mutex' unlocked)
   * and cv->... data is not protected.
   */
  if (gthread_mutex_lock(&(cv->waitersLock)) != 0)
    {
      return EINVAL;
    }

  cv->waiters++;

  if (gthread_mutex_unlock(&(cv->waitersLock)) != 0)
    {
      return EINVAL;
    }

  /*
   * We keep the lock held just long enough to increment the count of
   * waiters by one (above).
   * Note that we can't keep it held across the
   * call to sem_wait since that will deadlock other calls
   * to gthread_cond_signal
   */
  cleanup_args.mutexPtr = mutex;
  cleanup_args.cv = cv;
  cleanup_args.resultPtr = &result;

  gthread_cleanup_push (cond_wait_cleanup, (void *) &cleanup_args);

  if ((result = gthread_mutex_unlock (mutex)) == 0)
    {
      /*
       * Wait to be awakened by
       *              gthread_cond_signal, or
       *              gthread_cond_broadcast
       *              timeout
       *
       * Note: 
       *      _gthread_sem_timedwait is a cancelation point,
       *      hence providing the
       *      mechanism for making gthread_cond_wait a cancelation
       *      point. We use the cleanup mechanism to ensure we
       *      re-lock the mutex and decrement the waiters count
       *      if we are canceled.
       */
      if (_gthread_sem_timedwait (&(cv->sema), abstime) == -1)
	{
	  result = errno;
	}
    }

  gthread_cleanup_pop (1);

  /*
   * "result" can be modified by the cleanup handler.
   * Specifically, if we are the last waiting thread and failed
   * to notify the broadcast thread to proceed.
   */
  return (result);

}                               /* cond_timedwait */


int
gthread_cond_timedwait (gthread_cond_t * cond, 
		gthread_mutex_t * mutex,
		const struct gtimespec *abstime)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits on a condition variable either until
      *      awakened by a signal or broadcast; or until the time
      *      specified by abstime passes.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of gthread_cond_t
      *
      *      mutex
      *              pointer to an instance of gthread_mutex_t
      *
      *      abstime
      *              pointer to an instance of (const struct gtimespec)
      *
      *
      * DESCRIPTION
      *      This function waits on a condition variable either until
      *      awakened by a signal or broadcast; or until the time
      *      specified by abstime passes.
      *
      *      NOTES:
      *      1)      The function must be called with 'mutex' LOCKED
      *               by the calling thread, or undefined behaviour
      *              will result.
      *
      *      2)      This routine atomically releases 'mutex' and causes
      *              the calling thread to block on the condition variable.
      *              The blocked thread may be awakened by 
      *                      gthread_cond_signal or 
      *                      gthread_cond_broadcast.
      *
      *
      * RESULTS
      *              0               caught condition; mutex released,
      *              EINVAL          'cond', 'mutex', or abstime is invalid,
      *              EINVAL          different mutexes for concurrent waits,
      *              EINVAL          mutex is not held by the calling thread,
      *              ETIMEDOUT       abstime ellapsed before cond was signaled.
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (abstime == NULL)
    {
      result = EINVAL;
    }
  else
    {
      result = cond_timedwait(cond, mutex, abstime);
    }

  return(result);
}                               /* gthread_cond_timedwait */

int gthread_cond_wait (gthread_cond_t * cond,  gthread_mutex_t * mutex)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function waits on a condition variable until
      *      awakened by a signal or broadcast.
      *
      *      Caller MUST be holding the mutex lock; the
      *      lock is released and the caller is blocked waiting
      *      on 'cond'. When 'cond' is signaled, the mutex
      *      is re-acquired before returning to the caller.
      *
      * PARAMETERS
      *      cond
      *              pointer to an instance of gthread_cond_t
      *
      *      mutex
      *              pointer to an instance of gthread_mutex_t
      *
      *
      * DESCRIPTION
      *      This function waits on a condition variable until
      *      awakened by a signal or broadcast.
      *
      *      NOTES:
      *      1)      The function must be called with 'mutex' LOCKED
      *               by the calling thread, or undefined behaviour
      *              will result.
      *
      *      2)      This routine atomically releases 'mutex' and causes
      *              the calling thread to block on the condition variable.
      *              The blocked thread may be awakened by 
      *                      gthread_cond_signal or 
      *                      gthread_cond_broadcast.
      *
      * Upon successful completion, the 'mutex' has been locked and 
      * is owned by the calling thread.
      *
      *
      * RESULTS
      *              0               caught condition; mutex released,
      *              EINVAL          'cond' or 'mutex' is invalid,
      *              EINVAL          different mutexes for concurrent waits,
      *              EINVAL          mutex is not held by the calling thread,
      *
      * ------------------------------------------------------
      */
{
  /* The NULL abstime arg means INFINITE waiting. */
  return(cond_timedwait(cond, mutex, NULL));
}                               /* gthread_cond_wait */



int
sem_init (sem_t * sem, int pshared, unsigned int value)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function initializes an unnamed semaphore. the
      *      initial value of the semaphore is 'value'
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      *      pshared
      *              if zero, this semaphore may only be shared between
      *              threads in the same process.
      *              if nonzero, the semaphore can be shared between
      *              processes
      *
      *      value
      *              initial value of the semaphore counter
      *
      * DESCRIPTION
      *      This function initializes an unnamed semaphore. The
      *      initial value of the semaphore is set to 'value'.
      *
      * RESULTS
      *              0               successfully created semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSPC          a required resource has been exhausted,
      *              ENOSYS          semaphores are not supported,
      *              EPERM           the process lacks appropriate privilege
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (pshared != 0)
    {
      /*
       * Creating a semaphore that can be shared between
       * processes
       */
      result = EPERM;

    }
  else
    {
      /*
       * NOTE: Taking advantage of the fact that
       *               sem_t is a simple structure with one entry;
       *               We don't have to allocate it...
       */
#ifdef USE_DEPRECIATED_WIN32_THREADING
      *sem = CreateSemaphore (  0,    value,  0x7FFFFFF,NULL);
#else
	  *sem = CreateSemaphoreExW(NULL, value, 0x7FFFFFF, NULL, 0, SEMAPHORE_ALL_ACCESS);
#endif

      if (*sem == 0)
	{
	  result = ENOSPC;
	}
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* sem_init */

int
sem_destroy (sem_t * sem)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function destroys an unnamed semaphore.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      * DESCRIPTION
      *      This function destroys an unnamed semaphore.
      *
      * RESULTS
      *              0               successfully destroyed semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *              EBUSY           threads (or processes) are currently
      *                                      blocked on 'sem'
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (sem == NULL)
    {
      result = EINVAL;
    }
  else if (! CloseHandle (*sem))
    {
      result = EINVAL;
    }

  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* sem_destroy */


int
sem_post (sem_t * sem)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function posts a wakeup to a semaphore.
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      * DESCRIPTION
      *      This function posts a wakeup to a semaphore. If there
      *      are waiting threads (or processes), one is awakened;
      *      otherwise, the semaphore value is incremented by one.
      *
      * RESULTS
      *              0               successfully posted semaphore,
      *              -1              failed, error in errno
      * ERRNO
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSYS          semaphores are not supported,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (sem == NULL)
    {
	result = EINVAL;
    }
  else if (! ReleaseSemaphore (*sem, 1, 0))
    {
	result = EINVAL;
    }


  if (result != 0)
    {
      errno = result;
      return -1;
    }

  return 0;

}				/* sem_post */

BOOL (WINAPI *_gthread_try_enter_critical_section)(LPCRITICAL_SECTION) = NULL;

int
gthread_mutex_init(gthread_mutex_t *mutex, const gthread_mutexattr_t *attr)
{
  int result = 0;
  gthread_mutex_t mx;

  if (mutex == NULL)
    {
      return EINVAL;
    }

  mx = *mutex;

  mx = (gthread_mutex_t) calloc(1, sizeof(*mx));

  if (mx == NULL)
    {
      result = ENOMEM;
      goto FAIL0;
    }

  mx->mutex = 0;

  if (attr != NULL
      && *attr != NULL
      && (*attr)->pshared == GTHREAD_PROCESS_SHARED
      )
    {
      /*
       * Creating mutex that can be shared between
       * processes.
       */
#if _POSIX_THREAD_PROCESS_SHARED

      /*
       * Not implemented yet.
       */

#error ERROR [__FILE__, line __LINE__]: Process shared mutexes are not supported yet.

      mx->mutex = CreateMutex (
				  NULL,
				  FALSE,
				  ????);
      result = (mx->mutex == 0) ? EAGAIN : 0;

#else

      result = ENOSYS;

#endif /* _POSIX_THREAD_PROCESS_SHARED */
    }
  else
    {
      if (_gthread_try_enter_critical_section != NULL
	  || (attr != NULL
	      && *attr != NULL
	      && (*attr)->forcecs == 1)
	  )
	{
	  /* 
	   * Create a critical section. 
	   */
		// In VC 6.0 and older use the depreciated interface
		#ifdef USE_DEPRECIATED_WIN32_THREADING
		  InitializeCriticalSection(&mx->cs);
		#else
		  InitializeCriticalSectionEx(&mx->cs,2,0);
		#endif
	}
      else
	{
	  /*
	   * Create a mutex that can only be used within the
	   * current process
	   */
  	  // In VC 6.0 and older
	  #ifdef USE_DEPRECIATED_WIN32_THREADING
  	    mx->mutex = CreateMutex (NULL,   FALSE,   NULL);
	  #else
	    mx->mutex = CreateMutexEx(NULL, FALSE, NULL, MUTEX_ALL_ACCESS);
	  #endif

	  if (mx->mutex == 0)
	    {
	      result = EAGAIN;
	      mx = NULL;
	      goto FAIL0;
	    }
	}
    }

FAIL0:
  *mutex = mx;

  return(result);
}

int
gthread_mutex_destroy(gthread_mutex_t *mutex)
{
  int result = 0;
  gthread_mutex_t mx;

  if (mutex == NULL
      || *mutex == NULL)
    {
      return EINVAL;
    }

  /*
   * Check to see if we have something to delete.
   */
  if (*mutex != (gthread_mutex_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      mx = *mutex;

      if (mx->mutex == 0)
	{
	  DeleteCriticalSection(&mx->cs);
	}
      else
	{
	  result = (CloseHandle (mx->mutex) ? 0 : EINVAL);
	}

      if (result == 0)
        {
          mx->mutex = 0;
          free(mx);
          *mutex = NULL;
        }
    }
  else
    {
      /*
       * See notes in _mutex_check_need_init() above also.
       */
      EnterCriticalSection(&_gthread_mutex_test_init_lock);

      /*
       * Check again.
       */
      if (*mutex == (gthread_mutex_t) _GTHREAD_OBJECT_AUTO_INIT)
        {
          /*
           * This is all we need to do to destroy a statically
           * initialised mutex that has not yet been used (initialised).
           * If we get to here, another thread
           * waiting to initialise this mutex will get an EINVAL.
           */
          *mutex = NULL;
        }
      else
        {
          /*
           * The mutex has been initialised while we were waiting
           * so assume it's in use.
           */
          result = EBUSY;
        }

      LeaveCriticalSection(&_gthread_mutex_test_init_lock);
    }

  return(result);
}


static int
_mutex_check_need_init(gthread_mutex_t *mutex)
{
  int result = 0;

  /*
   * The following guarded test is specifically for statically
   * initialised mutexes (via GTHREAD_MUTEX_INITIALIZER).
   *
   * Note that by not providing this synchronisation we risk
   * introducing race conditions into applications which are
   * correctly written.
   *
   * Approach
   * --------
   * We know that static mutexes will not be PROCESS_SHARED
   * so we can serialise access to internal state using
   * Win32 Critical Sections rather than Win32 Mutexes.
   *
   * If using a single global lock slows applications down too much,
   * multiple global locks could be created and hashed on some random
   * value associated with each mutex, the pointer perhaps. At a guess,
   * a good value for the optimal number of global locks might be
   * the number of processors + 1.
   *
   */
  EnterCriticalSection(&_gthread_mutex_test_init_lock);

  /*
   * We got here possibly under race
   * conditions. Check again inside the critical section
   * and only initialise if the mutex is valid (not been destroyed).
   * If a static mutex has been destroyed, the application can
   * re-initialise it only by calling gthread_mutex_init()
   * explicitly.
   */
  if (*mutex == (gthread_mutex_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      result = gthread_mutex_init(mutex, NULL);
    }
  else if (*mutex == NULL)
    {
      /*
       * The mutex has been destroyed while we were waiting to
       * initialise it, so the operation that caused the
       * auto-initialisation should fail.
       */
      result = EINVAL;
    }

  LeaveCriticalSection(&_gthread_mutex_test_init_lock);

  return(result);
}

int
gthread_mutex_lock(gthread_mutex_t *mutex)
{
  int result = 0;
  gthread_mutex_t mx;

  if (mutex == NULL || *mutex == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static mutex. We check
   * again inside the guarded section of _mutex_check_need_init()
   * to avoid race conditions.
   */
  if (*mutex == (gthread_mutex_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      result = _mutex_check_need_init(mutex);
    }

  mx = *mutex;

  if (result == 0)
    {
      if (mx->mutex == 0)
	{
	  EnterCriticalSection(&mx->cs);
	}
      else
	{
	  result = (WaitForSingleObjectEx(mx->mutex, INFINITE, false) == WAIT_OBJECT_0)  ? 0   : EINVAL;
	}
    }

  return(result);
}

int
gthread_mutex_unlock(gthread_mutex_t *mutex)
{
  int result = 0;
  gthread_mutex_t mx;

  if (mutex == NULL || *mutex == NULL)
    {
      return EINVAL;
    }

  mx = *mutex;

  /* 
   * If the thread calling us holds the mutex then there is no
   * race condition. If another thread holds the
   * lock then we shouldn't be in here.
   */
  if (mx != (gthread_mutex_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      if (mx->mutex == 0)
	{
	  LeaveCriticalSection(&mx->cs);
	}
      else
	{
	  result = (ReleaseMutex (mx->mutex) ? 0 : EINVAL);
	}
    }
  else
    {
      result = EINVAL;
    }

  return(result);
}

int
gthread_mutex_trylock(gthread_mutex_t *mutex)
{
  int result = 0;
  gthread_mutex_t mx;

  if (mutex == NULL || *mutex == NULL)
    {
      return EINVAL;
    }

  /*
   * We do a quick check to see if we need to do more work
   * to initialise a static mutex. We check
   * again inside the guarded section of _mutex_check_need_init()
   * to avoid race conditions.
   */
  if (*mutex == (gthread_mutex_t) _GTHREAD_OBJECT_AUTO_INIT)
    {
      result = _mutex_check_need_init(mutex);
    }

  mx = *mutex;

  if (result == 0)
    {
      if (mx->mutex == 0)
	{
	  if ((*_gthread_try_enter_critical_section)(&mx->cs) != TRUE)
	    {
	      result = EBUSY;
	    }
	}
      else
	{
	  DWORD status;

	  status = WaitForSingleObjectEx (mx->mutex, 0, false);

	  if (status != WAIT_OBJECT_0)
	    {
	      result = ((status == WAIT_TIMEOUT)
			? EBUSY
			: EINVAL);
	    }
	}
    }

  return(result);
}

int gthread_cancel (gthread_t thread)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function requests cancellation of 'thread'.
      *
      * PARAMETERS
      *      thread
      *              reference to an instance of gthread_t
      *
      *
      * DESCRIPTION
      *      This function requests cancellation of 'thread'.
      *      NOTE: cancellation is asynchronous; use gthread_join to
      *                wait for termination of 'thread' if necessary.
      *
      * RESULTS
      *              0               successfully requested cancellation,
      *              ESRCH           no thread found corresponding to 'thread',
      *
      * ------------------------------------------------------
      */
{
  int result;

  if (thread != NULL)
    {
      /*
       * Set for deferred cancellation.
       */
      if (!SetEvent (thread->cancelEvent))
	{
	  result = ESRCH;
	}
      else
	{
	  result = 0;
	}

    }
  else
    {
      result = ESRCH;
    }

  return (result);
}

struct ThreadKeyAssoc {
  /*
   * Purpose:
   *      This structure creates an association between a
   *      thread and a key.
   *      It is used to implement the implicit invocation
   *      of a user defined destroy routine for thread
   *      specific data registered by a user upon exiting a
   *      thread.
   *
   * Attributes:
   *      lock
   *              protects access to the rest of the structure
   *
   *      thread
   *              reference to the thread that owns the association.
   *              As long as this is not NULL, the association remains
   *              referenced by the gthread_t.
   *
   *      key
   *              reference to the key that owns the association.
   *              As long as this is not NULL, the association remains
   *              referenced by the gthread_key_t.
   *
   *      nextKey
   *              The gthread_t->keys attribute is the head of a
   *              chain of associations that runs through the nextKey
   *              link. This chain provides the 1 to many relationship
   *              between a gthread_t and all gthread_key_t on which
   *              it called gthread_setspecific.
   *
   *      nextThread
   *              The gthread_key_t->threads attribute is the head of
   *              a chain of assoctiations that runs through the
   *              nextThreads link. This chain provides the 1 to many
   *              relationship between a gthread_key_t and all the 
   *              PThreads that have called gthread_setspecific for
   *              this gthread_key_t.
   *
   *
   * Notes:
   *      1)      As long as one of the attributes, thread or key, is
   *              not NULL, the association is being referenced; once
   *              both are NULL, the association must be released.
   *
   *      2)      Under WIN32, an association is only created by
   *              gthread_setspecific if the user provided a
   *              destroyRoutine when they created the key.
   *
   *
   */
  gthread_mutex_t lock;
  gthread_t thread;
  gthread_key_t key;
  ThreadKeyAssoc *nextKey;
  ThreadKeyAssoc *nextThread;
};


int
_gthread_tkAssocCreate (ThreadKeyAssoc ** assocP,
			gthread_t thread,
			gthread_key_t key)
     /*
      * -------------------------------------------------------------------
      * This routine creates an association that
      * is unique for the given (thread,key) combination.The association 
      * is referenced by both the thread and the key.
      * This association allows us to determine what keys the
      * current thread references and what threads a given key
      * references.
      * See the detailed description
      * at the beginning of this file for further details.
      *
      * Notes:
      *      1)      New associations are pushed to the beginning of the
      *              chain so that the internal _gthread_selfThreadKey association
      *              is always last, thus allowing selfThreadExit to
      *              be implicitly called by gthread_exit last.
      *
      * Parameters:
      *              assocP
      *                      address into which the association is returned.
      *              thread
      *                      current running thread. If NULL, then association
      *                      is only added to the key. A NULL thread indicates
      *                      that the user called gthread_setspecific prior
      *                      to starting a thread. That's ok.
      *              key
      *                      key on which to create an association.
      * Returns:
      *       0              - if successful,
      *      -1              - general error
      * -------------------------------------------------------------------
      */
{
  int result;
  ThreadKeyAssoc *assoc;

  /*
   * Have to create an association and add it
   * to both the key and the thread.
   */
  assoc = (ThreadKeyAssoc *)
    calloc (1, sizeof (*assoc));

  if (assoc == NULL)
    {
      result = -1;
      goto FAIL0;
    }

  if ((result = gthread_mutex_init (&(assoc->lock), NULL)) !=
      0)
    {
      goto FAIL1;
    }

  assoc->thread = thread;
  assoc->key = key;

  /*
   * Register assoc with key
   */
  if ((result = gthread_mutex_lock (&(key->threadsLock))) !=
      0)
    {
      goto FAIL2;
    }

  assoc->nextThread = (ThreadKeyAssoc *) key->threads;
  key->threads = (void *) assoc;

  gthread_mutex_unlock (&(key->threadsLock));

  if (thread != NULL)
    {
      /*
       * Register assoc with thread
       */
      assoc->nextKey = (ThreadKeyAssoc *) thread->keys;
      thread->keys = (void *) assoc;
    }

  *assocP = assoc;

  return (result);

  /*
   * -------------
   * Failure Code
   * -------------
   */
FAIL2:
  gthread_mutex_destroy (&(assoc->lock));

FAIL1:
  free (assoc);

FAIL0:

  return (result);

}				/* _gthread_tkAssocCreate */


int gthread_setspecific (gthread_key_t key, const void *value)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function initializes an unnamed semaphore. the
      *      initial value of the semaphore is 'value'
      *
      * PARAMETERS
      *      sem
      *              pointer to an instance of sem_t
      *
      *
      * DESCRIPTION
      *      This function  initializes an unnamed semaphore. The
      *      initial value of the semaphore is set to 'value'.
      *
      * RESULTS
      *              0               successfully created semaphore,
      *              EINVAL          'sem' is not a valid semaphore,
      *              ENOSPC          a required resource has been exhausted,
      *              ENOSYS          semaphores are not supported,
      *              EPERM           the process lacks appropriate privilege
      *
      * ------------------------------------------------------
      */
{
  gthread_t self;
  int result = 0;

  if (key != _gthread_selfThreadKey)
    {
      /*
       * Using gthread_self will implicitly create
       * an instance of gthread_t for the current
       * thread if one wasn't explicitly created
       */
      self = gthread_self ();
    }
  else
    {
      /*
       * Resolve catch-22 of registering thread with threadSelf
       * key
       */
      self = (gthread_t) gthread_getspecific (_gthread_selfThreadKey);
      if (self == NULL)
        {
          self = (gthread_t) value;
        }
    }

  result = 0;

  if (key != NULL)
    {
      ThreadKeyAssoc *assoc;

      if (self != NULL &&
          key->destructor != NULL &&
          value != NULL)
        {
          /*
           * Only require associations if we have to
           * call user destroy routine.
           * Don't need to locate an existing association
           * when setting data to NULL for WIN32 since the
           * data is stored with the operating system; not
           * on the association; setting assoc to NULL short
           * circuits the search.
           */
          assoc = (ThreadKeyAssoc *) self->keys;
          /*
           * Locate existing association
           */
          while (assoc != NULL)
            {
              if (assoc->key == key)
                {
                  /*
                   * Association already exists
                   */
                  break;
                }
              assoc = assoc->nextKey;
            }

          /*
           * create an association if not found
           */
          result = (assoc == NULL)
            ? _gthread_tkAssocCreate (&assoc, self, key)
            : 0;
        }
      else
        {
          result = 0;
        }

      if (result == 0)
        {
          TlsSetValue (key->key, (LPVOID) value);
        }
    }
  return (result);
}                               /* gthread_setspecific */

void
_gthread_tkAssocDestroy (ThreadKeyAssoc * assoc)
     /*
      * -------------------------------------------------------------------
      * This routine releases all resources for the given ThreadKeyAssoc
      * once it is no longer being referenced
      * ie) both the key and thread have stopped referencing it.
      *
      * Parameters:
      *              assoc
      *                      an instance of ThreadKeyAssoc.
      * Returns:
      *      N/A
      * -------------------------------------------------------------------
      */
{

  if ((assoc != NULL) &&
      (assoc->key == NULL && assoc->thread == NULL))
    {

      gthread_mutex_destroy (&(assoc->lock));

      free (assoc);
    }

}				/* _gthread_tkAssocDestroy */


void
_gthread_callUserDestroyRoutines (gthread_t thread)
     /*
      * -------------------------------------------------------------------
      * DOCPRIVATE
      *
      * This the routine runs through all thread keys and calls
      * the destroy routines on the user's data for the current thread.
      * It simulates the behaviour of POSIX Threads.
      *
      * PARAMETERS
      *              thread
      *                      an instance of gthread_t
      *
      * RETURNS
      *              N/A
      * -------------------------------------------------------------------
      */
{
  ThreadKeyAssoc **nextP;
  ThreadKeyAssoc *assoc;

  if (thread != NULL)
    {
      /*
       * Run through all Thread<-->Key associations
       * for the current thread.
       * If the gthread_key_t still exits (ie the assoc->key
       * is not NULL) then call the user's TSD destroy routine.
       * Notes:
       *      If assoc->key is NULL, then the user previously called
       *      PThreadKeyDestroy. The association is now only referenced
       *      by the current thread and must be released; otherwise
       *      the assoc will be destroyed when the key is destroyed.
       */
      nextP = (ThreadKeyAssoc **) & (thread->keys);
      assoc = *nextP;

      while (assoc != NULL)
	{

	  if (gthread_mutex_lock (&(assoc->lock)) == 0)
	    {
	      gthread_key_t k;
	      if ((k = assoc->key) != NULL)
		{
		  /*
		   * Key still active; gthread_key_delete
		   * will block on this same mutex before
		   * it can release actual key; therefore,
		   * key is valid and we can call the destroy
		   * routine;
		   */
		  void *value = NULL;

		  value = gthread_getspecific (k);
		  if (value != NULL && k->destructor != NULL)
		    {

#ifdef _MSC_VER

		      __try
		      {
			/*
			 * Run the caller's cleanup routine.
			 */
			(*(k->destructor)) (value);
		      }
		      __except (EXCEPTION_EXECUTE_HANDLER)
		      {
			/*
			 * A system unexpected exception had occurred
			 * running the user's destructor.
			 * We get control back within this block.
			 */
		      }

#else  /* _MSC_VER */
#ifdef __cplusplus

		      try
		      {
			/*
			 * Run the caller's cleanup routine.
			 */
			(*(k->destructor)) (value);
		      }
		      catch (...)
		      {
			/*
			 * A system unexpected exception had occurred
			 * running the user's destructor.
			 * We get control back within this block.
			 */
		      }

#else  /* __cplusplus */

			/*
			 * Run the caller's cleanup routine.
			 */
			(*(k->destructor)) (value);

#endif /* __cplusplus */
#endif /* _MSC_VER */
		    }
		}

	      /*
	       * mark assoc->thread as NULL to indicate the
	       * thread no longer references this association
	       */
	      assoc->thread = NULL;

	      /*
	       * Remove association from the gthread_t chain
	       */
	      *nextP = assoc->nextKey;

	      gthread_mutex_unlock (&(assoc->lock));

	      _gthread_tkAssocDestroy (assoc);

	      assoc = *nextP;
	    }
	}
    }

}				/* _gthread_callUserDestroyRoutines */



gthread_t gthread_self (void)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function returns a reference to the current running
      *      thread.
      *
      * PARAMETERS
      *      N/A
      *
      *
      * DESCRIPTION
      *      This function returns a reference to the current running
      *      thread.
      *
      * RESULTS
      *              gthread_t       reference to the current thread
      *
      * ------------------------------------------------------
      */
{
  gthread_t self = NULL;
  /*
   * need to ensure there always is a self
   */

  if ((self = (gthread_t) gthread_getspecific (_gthread_selfThreadKey)) 
      == NULL)
    {
      /*
       * Need to create an implicit 'self' for the currently
       * executing thread.
       */
      self = (gthread_t) calloc (1, sizeof (*self));

      if (self != NULL)
	{
	  self->implicit = 1;
	  self->detachState = GTHREAD_CREATE_DETACHED;

	  self->thread = GetCurrentThreadId ();

	  if( !DuplicateHandle(
			       GetCurrentProcess(),
			       GetCurrentThread(),
			       GetCurrentProcess(),
			       &self->threadH,
			       0,
			       FALSE,
			       DUPLICATE_SAME_ACCESS ) )
	    {
	      free( self );
	      return (NULL);
	    }
	}

      gthread_setspecific (_gthread_selfThreadKey, self);
    }

  return (self);

}				/* gthread_self */


#ifdef _MSC_VER

static DWORD ExceptionFilter (EXCEPTION_POINTERS * ep, DWORD * ei)
{
  DWORD param;
  DWORD numParams = ep->ExceptionRecord->NumberParameters;
  
  numParams = (numParams > 3) ? 3 : numParams;

  for (param = 0; param < numParams; param++)
    {
      ei[param] = ep->ExceptionRecord->ExceptionInformation[param];
    }

  return EXCEPTION_EXECUTE_HANDLER;
}

#endif /* _MSC_VER */

//   'unsigned int (__stdcall *)(ThreadParms *)' changes to 
//	DWORD (WINAPI *GTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
DWORD WINAPI _gthread_threadStart (void *p)
{
	ThreadParms *threadParms = (ThreadParms *)p;

  gthread_t self;
  void *(*start) (void *);
  void *arg;

#ifdef _MSC_VER

  DWORD ei[3];

#endif

  void * status;

  self = threadParms->tid;
  start = threadParms->start;
  arg = threadParms->arg;

#if defined (__MINGW32__) && ! defined (__MSVCRT__)
  /* beginthread does not return the thread id, and so we do it here. */
  self->thread = GetCurrentThreadId ();
#endif

  free (threadParms);

  gthread_setspecific (_gthread_selfThreadKey, self);

#ifdef _MSC_VER

  __try
  {
    /*
     * Run the caller's routine;
     */
    status = (*start) (arg);
  }
  __except (ExceptionFilter(GetExceptionInformation(), ei))
  {
    DWORD ec = GetExceptionCode();

    if (ec == EXCEPTION_GTHREAD_SERVICES)
      {
	switch (ei[0])
	  {
	  case _GTHREAD_EPS_CANCEL:
	    status = GTHREAD_CANCELED;
	    break;
	  case _GTHREAD_EPS_EXIT:
	    status = (void *) ei[1];
	    break;
	  default:
	    status = GTHREAD_CANCELED;
	  }
      }
    else
      {
	/*
	 * A system unexpected exception had occurred running the user's
	 * routine. We get control back within this block.
	 */
	status = GTHREAD_CANCELED;
      }
  }

#else /* _MSC_VER */

#ifdef __cplusplus

  try
  {
    /*
     * Run the caller's routine;
     */
    status = self->exitStatus = (*start) (arg);
  }
  catch (Pthread_exception_cancel)
    {
      /*
       * Thread was cancelled.
       */
      status = GTHREAD_CANCELED;
    }
  catch (Pthread_exception_exit)
    {
      /*
       * Thread was exited via gthread_exit().
       */
      status = self->exitStatus;
    }
  catch (...)
    {
      /*
       * A system unexpected exception had occurred running the user's
       * routine. We get control back within this block.
       */
      status = GTHREAD_CANCELED;
    }

#else /* __cplusplus */

  /*
   * Run the caller's routine; no cancelation or other exceptions will
   * be honoured.
   */
  status = (*start) (arg);

#endif /* __cplusplus */

#endif /* _MSC_VER */

  _gthread_callUserDestroyRoutines(self);

#ifndef __WINPHONE
	#if ! defined (__MINGW32__) || defined (__MSVCRT__)
	  _endthreadex ((unsigned) status);
	#else
	  _endthread ();
	#endif
#endif

  /*
   * Never reached.
   */

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
  return (unsigned) status;
#endif

}				/* _gthread_threadStart */

#ifndef __WINPHONE
void _gthread_threadDestroy (gthread_t thread)
{
  if (thread != NULL)
    {
      _gthread_callUserDestroyRoutines (thread);

      if (thread->cancelEvent != NULL)
	{
	  CloseHandle (thread->cancelEvent);
	}

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
      /* See documentation for endthread vs endthreadex. */
      if( thread->threadH != 0 )
	{
	  CloseHandle( thread->threadH );
	}
#endif

      free (thread);
    }

}				/* _gthread_threadDestroy */
#endif

int gthread_create (gthread_t * tid, const gthread_attr_t * attr,void *(*start) (void *),void *arg)
{
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function creates a thread running the start function,
      *      passing it the parameter value, 'arg'.
      *
      * PARAMETERS
      *      tid
      *              pointer to an instance of gthread_t
      *
      *      attr
      *              optional pointer to an instance of gthread_attr_t
      *
      *      start
      *              pointer to the starting routine for the new thread
      *
      *      arg
      *              optional parameter passed to 'start'
      *
      *
      * DESCRIPTION
      *      This function creates a thread running the start function,
      *      passing it the parameter value, 'arg'. The 'attr'
      *      argument specifies optional creation attributes.
      *      The thread is identity of the new thread is returned
      *      as 'tid'
      *
      * RESULTS
      *              0               successfully created thread,
      *              EINVAL          attr invalid,
      *              EAGAIN          insufficient resources.
      *
      * ------------------------------------------------------
      */
{
  gthread_t thread;
  int result = EAGAIN;
  int run = TRUE;
  ThreadParms *parms = NULL;
  long stackSize;

  if ((thread = (gthread_t) calloc (1, sizeof (*thread))) ==
      NULL)
    {
      goto FAIL0;
    }

  /*
   * Setup standard default state.
   */
  thread->detachState = GTHREAD_CREATE_JOINABLE;
  thread->cancelState = GTHREAD_CANCEL_ENABLE;
  thread->cancelType  = GTHREAD_CANCEL_DEFERRED;

	// In VC 6.0 and older 
	#ifdef USE_DEPRECIATED_WIN32_THREADING
	 thread->cancelEvent = CreateEvent( 0, (int) TRUE, /* manualReset  */ (int) FALSE,/* setSignaled  */		  NULL);
	#else
	 thread->cancelEvent = CreateEventEx(  0,  NULL,  CREATE_EVENT_MANUAL_RESET,    EVENT_ALL_ACCESS);
	#endif

  if (thread->cancelEvent == NULL)
    {
      goto FAIL0;
    }

  if ((parms = (ThreadParms *) malloc (sizeof (*parms))) ==
      NULL)
    {
      goto FAIL0;
    }

  parms->tid = thread;
  parms->start = start;
  parms->arg = arg;

  if (attr != NULL && *attr != NULL)
    {
      stackSize = (*attr)->stacksize;
      thread->detachState = (*attr)->detachstate;

#if HAVE_SIGSET_T

      thread->sigmask = (*attr)->sigmask;

#endif /* HAVE_SIGSET_T */

    }
  else
    {
      /*
       * Default stackSize
       */
      stackSize = 0;
    }

  thread->state = run
    ? PThreadStateInitial
    : PThreadStateSuspended;

  thread->keys = NULL;
#if ! defined (__MINGW32__) || defined (__MSVCRT__)

#ifdef __WINPHONE
  thread->threadH = (HANDLE)CreateThread(NULL, stackSize, _gthread_threadStart, parms, (unsigned) run ? 0 : CREATE_SUSPENDED, 0);
  thread->thread = g_nextGThreadID++;
#else
  thread->threadH = (HANDLE)CreateThread(NULL, stackSize, _gthread_threadStart, parms, (unsigned) run ? 0 : CREATE_SUSPENDED, (LPDWORD) &(thread->thread)); 
#endif 

#else /* __MINGW32__ && ! __MSVCRT__ */

  thread->threadH = (HANDLE)
    _beginthread (
		   (void (*) (void *)) _gthread_threadStart,
		   (unsigned) stackSize,	/* default stack size   */
		   parms);

  /* Make the return code to match _beginthreadex's.  */
  if (thread->threadH == (HANDLE)-1L)
    thread->threadH = NULL;
  else if (! run)
    {
      /* beginthread does not allow for create flags, so we do it now.
         Note that beginthread itself creates the thread in SUSPENDED
	 mode, and then calls ResumeThread to start it.  */
      SuspendThread (thread->threadH);
    }

#endif /* __MINGW32__ && ! __MSVCRT__ */

  result = ((int)thread->threadH != 0) ? 0 : EAGAIN;

  /*
   * Fall Through Intentionally
   */

  /*
   * ------------
   * Failure Code
   * ------------
   */

FAIL0:
  if (result != 0)
    {
	#ifndef __WINPHONE // WinRT threading does need this - the thread is destroyed when returning from the 1st stack call
		_gthread_threadDestroy (thread);
	#endif
      thread = NULL;

      if (parms != NULL)
	{
	  free (parms);
	}
    }
  *tid = thread;

  return (result);

}				/* gthread_create */
}


void
gthread_exit (void *value_ptr)
{
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function terminates the calling thread, returning
      *      the value 'value_ptr' to any joining thread.
      *
      * PARAMETERS
      *      value_ptr
      *              a generic data value (i.e. not the address of a value)
      *
      *
      * DESCRIPTION
      *      This function terminates the calling thread, returning
      *      the value 'value_ptr' to any joining thread.
      *      NOTE: thread should be joinable.
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
  gthread_t self;

#ifdef _MSC_VER

  DWORD exceptionInformation[3];

#endif

  /* If the current thread is implicit it was not started through
     gthread_create(), therefore we cleanup and end the thread
     here. Otherwise we raise an exception to unwind the exception
     stack. The exception will be caught by _gthread_threadStart(),
     which will cleanup and end the thread for us.
   */

  self = (gthread_t) gthread_getspecific (_gthread_selfThreadKey);

  if (self == NULL || self->implicit)
    {
      _gthread_callUserDestroyRoutines(self);

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
	#ifndef __WINPHONE
		_endthreadex ((unsigned) value_ptr);
	#endif
#else
      _endthread ();
#endif
      
      /* Never reached */
    }

  self->exitStatus = value_ptr;

#ifdef _MSC_VER

  exceptionInformation[0] = (DWORD) (_GTHREAD_EPS_EXIT);
  exceptionInformation[1] = (DWORD) (value_ptr);
  exceptionInformation[2] = (DWORD) (0);

  RaiseException (
		  EXCEPTION_GTHREAD_SERVICES,
		  0,
		  3,
		  EXCEPTIONCAST exceptionInformation);

#else /* ! _MSC_VER */

#ifdef __cplusplus

  throw Pthread_exception_exit();

#else /* ! __cplusplus */

  (void) gthread_pop_cleanup( 1 );

  _gthread_callUserDestroyRoutines(self);

#if ! defined (__MINGW32__) || defined (__MSVCRT__)
  _endthreadex ((unsigned) value_ptr);
#else
  _endthread ();
#endif

#endif /* __cplusplus */

#endif /* _MSC_VER */

  /* Never reached. */

}

int gsched_yield(void)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function indicates that the calling thread is
      *      willing to give up some time slices to other threads.
      *
      * PARAMETERS
      *      N/A
      *
      *
      * DESCRIPTION
      *      This function indicates that the calling thread is
      *      willing to give up some time slices to other threads.
      *      NOTE: Since this is part of POSIX 1003.1b
      *                (realtime extensions), it is defined as returning
      *                -1 if an error occurs and sets errno to the actual
      *                error.
      *
      * RESULTS
      *              0               successfully created semaphore,
      *              ENOSYS          sched_yield not supported,
      *
      * ------------------------------------------------------
      */
{
  Sleep(0);

  return 0;
}



int
gthread_key_create (gthread_key_t * key, void (*destructor) (void *))
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function creates a thread-specific data key visible
      *      to all threads. All existing and new threads have a value
      *      NULL for key until set using gthread_setspecific. When any
      *      thread with a non-NULL value for key terminates, 'destructor'
      *      is called with key's current value for that thread.
      *
      * PARAMETERS
      *      key
      *              pointer to an instance of gthread_key_t
      *
      *
      * DESCRIPTION
      *      This function creates a thread-specific data key visible
      *      to all threads. All existing and new threads have a value
      *      NULL for key until set using gthread_setspecific. When any
      *      thread with a non-NULL value for key terminates, 'destructor'
      *      is called with key's current value for that thread.
      *
      * RESULTS
      *              0               successfully created semaphore,
      *              EAGAIN          insufficient resources or GTHREAD_KEYS_MAX
      *                              exceeded,
      *              ENOMEM          insufficient memory to create the key,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;
  
  if ((*key = (gthread_key_t) calloc (1, sizeof (**key))) == NULL)
    {
      result = ENOMEM;
    }
  else if (((*key)->key = TlsAlloc ()) == TLS_OUT_OF_INDEXES)
    {
      /*
       * Create system key
       */
      result = EAGAIN;

      free (*key);
      *key = NULL;
    }
  else if (destructor != NULL)
    {
      /*
       * Have to manage associations between thread and key;
       * Therefore, need a lock that allows multiple threads
       * to gain exclusive access to the key->threads list
       */
      result = gthread_mutex_init (&((*key)->threadsLock), NULL);

      if (result != 0)
        {
          TlsFree ((*key)->key);

          free (*key);
          *key = NULL;
        }
      (*key)->destructor = destructor;
    }

  return (result);
}

int
gthread_key_delete (gthread_key_t key)
     /*
      * ------------------------------------------------------
      * DOCPUBLIC
      *      This function deletes a thread-specific data key. This
      *      does not change the value of the thread spcific data key
      *      for any thread and does not run the key's destructor
      *      in any thread so it should be used with caution.
      *
      * PARAMETERS
      *      key
      *              pointer to an instance of gthread_key_t
      *
      *
      * DESCRIPTION
      *      This function deletes a thread-specific data key. This
      *      does not change the value of the thread spcific data key
      *      for any thread and does not run the key's destructor
      *      in any thread so it should be used with caution.
      *
      * RESULTS
      *              0               successfully deleted the key,
      *              EINVAL          key is invalid,
      *
      * ------------------------------------------------------
      */
{
  int result = 0;

  if (key != NULL)
    {
      if (key->threads != NULL &&
          gthread_mutex_lock (&(key->threadsLock)) == 0)
        {
          /*
           * Run through all Thread<-->Key associations
           * for this key.
           * If the gthread_t still exits (ie the assoc->thread
           * is not NULL) then leave the assoc for the thread to
           * destroy.
           * Notes:
           *      If assoc->thread is NULL, then the associated thread
           *      is no longer referencing this assoc.
           *      The association is only referenced
           *      by this key and must be released; otherwise
           *      the assoc will be destroyed when the thread is destroyed.
           */
          ThreadKeyAssoc *assoc;

          assoc = (ThreadKeyAssoc *) key->threads;

          while (assoc != NULL)
            {
              if (gthread_mutex_lock (&(assoc->lock)) == 0)
                {
                  ThreadKeyAssoc *next;

                  assoc->key = NULL;
                  next = assoc->nextThread;
                  assoc->nextThread = NULL;

                  gthread_mutex_unlock (&(assoc->lock));

                  _gthread_tkAssocDestroy (assoc);

                  assoc = next;
                }
            }
          gthread_mutex_unlock (&(key->threadsLock));
        }

      TlsFree (key->key);
      if (key->destructor != NULL)
        {
          gthread_mutex_destroy (&(key->threadsLock));
        }

#if defined( _DEBUG )
      memset ((char *) key, 0, sizeof (*key));
#endif
      free (key);
    }

  return (result);
}




void _gthread_processTerminate (void)
     /*
      * ------------------------------------------------------
      * DOCPRIVATE
      *      This function performs process wide termination for
      *      the gthread library.
      *
      * PARAMETERS
      *      N/A
      *
      * DESCRIPTION
      *      This function performs process wide termination for
      *      the gthread library.
      *      This routine sets the global variable
      *      _gthread_processInitialized to FALSE
      *
      * RESULTS
      *              N/A
      *
      * ------------------------------------------------------
      */
{
  if (_gthread_processInitialized)
    {

      if (_gthread_selfThreadKey != NULL)
	{
	  /*
	   * Release _gthread_selfThreadKey
	   */
	  gthread_key_delete (_gthread_selfThreadKey);

	  _gthread_selfThreadKey = NULL;
	}

      if (_gthread_cleanupKey != NULL)
	{
	  /*
	   * Release _gthread_cleanupKey
	   */
	  gthread_key_delete (_gthread_cleanupKey);

	  _gthread_cleanupKey = NULL;
	}

      /* 
       * Destroy the global test and init check locks.
       */
      DeleteCriticalSection(&_gthread_rwlock_test_init_lock);
      DeleteCriticalSection(&_gthread_cond_test_init_lock);
      DeleteCriticalSection(&_gthread_mutex_test_init_lock);

      _gthread_processInitialized = FALSE;
    }
	FreeLibrary(_gthread_h_kernel32);

}				/* processTerminate */


int _gthread_processInitialize (void)
     /*
      * ------------------------------------------------------
      * DOCPRIVATE
      *      This function performs process wide initialization for
      *      the gthread library.
      *
      * PARAMETERS
      *      N/A
      *
      * DESCRIPTION
      *      This function performs process wide initialization for
      *      the gthread library.
      *      If successful, this routine sets the global variable
      *      _gthread_processInitialized to TRUE.
      *
      * RESULTS
      *              TRUE    if successful,
      *              FALSE   otherwise
      *
      * ------------------------------------------------------
      */
{
	if (_gthread_processInitialized)
		return 1; // if we already did it - success

	_gthread_processInitialized = TRUE;

  /*
   * Initialize Keys
   */
  if ((gthread_key_create (&_gthread_selfThreadKey, NULL) != 0) ||
      (gthread_key_create (&_gthread_cleanupKey, NULL) != 0))
    {

      _gthread_processTerminate ();
    }

  /* 
   * Set up the global test and init check locks.
   */
	// In VC 6.0 and older use the depreciated interface
	#ifdef USE_DEPRECIATED_WIN32_THREADING
	  InitializeCriticalSection(&_gthread_mutex_test_init_lock);
	  InitializeCriticalSection(&_gthread_cond_test_init_lock);
	  InitializeCriticalSection(&_gthread_rwlock_test_init_lock);
	#else
	  InitializeCriticalSectionEx(&_gthread_mutex_test_init_lock,2,0);
	  InitializeCriticalSectionEx(&_gthread_cond_test_init_lock,2,0);
	  InitializeCriticalSectionEx(&_gthread_rwlock_test_init_lock,2,0);
	#endif

	// This dynamic load of TryEnterCriticalSection is necessary on some builds, this will be true for older Borland Builds also - here it only test for VC6
	#if defined(_MSC_VER) && _MSC_VER <= 1200 
		_gthread_h_kernel32 = LoadLibrary(TEXT("KERNEL32.DLL"));
		_gthread_try_enter_critical_section = (BOOL (PT_STDCALL *)(LPCRITICAL_SECTION))	GetProcAddress(_gthread_h_kernel32,	   (LPCSTR) "TryEnterCriticalSection");
	#else
		_gthread_try_enter_critical_section = TryEnterCriticalSection;
	#endif

  return _gthread_processInitialized;

}				
