// --------------------------------------------------------------------------
//			John E. Bossom and United Business Technologies
//			  Copyright (c) 2013  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------

#ifndef _WIN32		// This is how XMLFoundation handles all threading on all non windows platforms
					// On Linux/AIX/Solaris/Android GThread is a direct map to this subset of POSIX thread calls.
	#define gthread_mutex_t			pthread_mutex_t
	#define gthread_mutex_init		pthread_mutex_init
	#define gthread_mutex_destroy	pthread_mutex_destroy
	#define gthread_mutex_lock		pthread_mutex_lock
	#define gthread_mutex_unlock	pthread_mutex_unlock
	#define gthread_mutex_trylock	pthread_mutex_trylock
	#define gthread_cond_t			pthread_cond_t
	#define gthread_cond_destroy	pthread_cond_destroy
	#define gthread_cond_init		pthread_cond_init
	#define gthread_cond_signal		pthread_cond_signal
	#define gthread_cond_wait		pthread_cond_wait
	#define gthread_cond_timedwait	pthread_cond_timedwait
	#define gthread_cancel			pthread_cancel
	#define gthread_create			pthread_create
	#define gthread_exit			pthread_exit
	#define gthread_t				pthread_t
	#define gsched_yield			sched_yield
	#define gtimespec				timespec

	#include <pthread.h>		// POSIX Threads Library
	#include <sched.h>			// POSIX Threads Library

#else // the entire remainder of this source file is for several Windows builds

#ifdef __WINPHONE
	#include <Windows.h>
	#include <WinBase.h>
	#include "WinPhoneThreadEmulation.h"
	using namespace ThreadEmulation;
#endif

typedef struct gthread_attr_t_ *gthread_attr_t;
typedef struct gthread_mutex_t_ * gthread_mutex_t;
typedef struct gthread_cond_t_ * gthread_cond_t;
typedef struct gthread_condattr_t_ * gthread_condattr_t;
typedef struct gthread_mutexattr_t_ * gthread_mutexattr_t;
typedef struct gthread_t_ *gthread_t;
typedef struct ThreadParms ThreadParms;
typedef struct ThreadKeyAssoc ThreadKeyAssoc;

#include "Windows.h" // for HANDLE
typedef HANDLE sem_t;

#ifndef HAVE_STRUCTS
#define HAVE_STRUCTS 1
struct gtimespec {
        long tv_sec;
        long tv_nsec;
};
struct gthread_mutex_t_ {
  HANDLE mutex;
  CRITICAL_SECTION cs;
};
struct gthread_mutexattr_t_ {
  int pshared;
  int forcecs;
};
struct gthread_cond_t_ {
  long waiters;                       /* # waiting threads             */
  gthread_mutex_t waitersLock;        /* Mutex that guards access to 
					 waiter count                  */
  sem_t sema;        /* Queue up threads waiting for the condition to become signaled  */
  HANDLE waitersDone;    /* An auto reset event used by the 
					 broadcast/signal thread to wait 
					 for the waiting thread(s) to wake
					 up and get a chance at the  
					 semaphore                     */
  int wasBroadcast;                   /* keeps track if we are signaling 
					 or broadcasting               */
};

struct gthread_condattr_t_ {
  int pshared;
};
struct gthread_attr_t_ {
  unsigned long valid;
  void *stackaddr;
  size_t stacksize;
  int detachstate;
  int priority;
#if HAVE_SIGSET_T
  sigset_t sigmask;
#endif /* HAVE_SIGSET_T */
};

#define GTHREAD_CREATE_JOINABLE		0
#define GTHREAD_CREATE_DETACHED		1
#define GTHREAD_CANCEL_ASYNCHRONOUS	0
#define GTHREAD_CANCEL_DEFERRED		1
#define _GTHREAD_EPS_CANCEL       0
#define _GTHREAD_EPS_EXIT         1


typedef enum {
  /*
   * This enumeration represents the state of the thread;
   * The thread is still "alive" if the numeric value of the
   * state is greater or equal "PThreadStateRunning".
   */
  PThreadStateInitial = 0,	/* Thread not running                   */
  PThreadStateRunning,	    /* Thread alive & kicking               */
  PThreadStateSuspended,	/* Thread alive but suspended           */
  PThreadStateCanceling,	/* Thread alive but and is              */
                            /* in the process of terminating        */
                            /* due to a cancellation request        */
  PThreadStateException,	/* Thread alive but exiting             */
                            /* due to an exception                  */
  PThreadStateLast
}
PThreadState;

typedef enum {
  /*
   * This enumeration represents the reason why a thread has
   * terminated/is terminating.
   */
  PThreadDemisePeaceful = 0,/* Death due natural causes     */
  PThreadDemiseCancelled,	/* Death due to user cancel     */
  PThreadDemiseException,	/* Death due to unhandled       */
                            /* exception                    */
  PThreadDemiseNotDead		/* I'm not dead!                */
}
PThreadDemise;

struct gthread_t_ {
  DWORD thread;
  HANDLE threadH;
  PThreadState state;
  PThreadDemise demise;
  void *exitStatus;
  void *parms;
  int ptErrno;
  int detachState;
  int cancelState;
  int cancelType;
  HANDLE cancelEvent;
#if HAVE_SIGSET_T
  sigset_t sigmask;
#endif /* HAVE_SIGSET_T */
  int implicit:1;
  void *keys;
};

struct ThreadParms {
  gthread_t tid;
  void *(*start) (void *);
  void *arg;
};

#endif /* HAVE_STRUCTS */

int _gthread_processInitialize (void);
void _gthread_processTerminate (void);

int gthread_cancel (gthread_t thread);
int gthread_create (gthread_t * tid, const gthread_attr_t * attr,   void *(*start) (void *),  void *arg);
void gthread_exit (void *value_ptr);

int gthread_cond_init (gthread_cond_t * cond, const gthread_condattr_t * attr);
int gthread_cond_destroy (gthread_cond_t * cond);
int gthread_cond_wait (gthread_cond_t * cond, gthread_mutex_t * mutex);
int gthread_cond_signal (gthread_cond_t * cond);
int gthread_cond_timedwait (gthread_cond_t * cond, gthread_mutex_t * mutex,  const struct gtimespec *abstime);

int gthread_mutex_init (gthread_mutex_t * mutex,  const gthread_mutexattr_t * attr);
int gthread_mutex_destroy (gthread_mutex_t * mutex);
int gthread_mutex_lock (gthread_mutex_t * mutex);
int gthread_mutex_trylock (gthread_mutex_t * mutex);
int gthread_mutex_unlock (gthread_mutex_t * mutex);

int gsched_yield (void);

#endif
