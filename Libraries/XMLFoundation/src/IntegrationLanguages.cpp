// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "IntegrationLanguages.h"
#include "GException.h"
#include "GHash.h"
#include "GList.h"
#include "GThread.h"



// language driver operation modes:
// --------------------------------
// 1 Single Threaded- 1 instance. TXML Server Blocking. 
//					  No two methods will ever be simultaneous executed.  
//					  TXML Server threads wait in line for the single instance of this object.
// 2 Multi Threaded	- 1 instance. All server threads into a single instance. 
//					  Handle you own thread safety issues.
// 3 Multi Object	- Multi instance - Object pool. (DEFAULT)
//					  Interface Objects are accessed by any thread as necessary,
//					  A single instance will only be used by a single thread at any moment.
//					  this relieves the driver implementation of thread blocking routines.
// 4 As necessary	- Multi instance. Created and destroyed with each use. No caching.
class InterfaceController
{
	GList m_lstInterfaces;

	gthread_cond_t	m_cond;
	gthread_mutex_t m_condLock;

	
	gthread_mutex_t m_lock;		// list lock
	int m_nThreadMode;
public:
	~InterfaceController()
	{
		gthread_mutex_destroy(&m_condLock);
		gthread_mutex_destroy(&m_lock);
		gthread_cond_destroy(&m_cond);
	}
	InterfaceController()
	{
		m_nThreadMode = 0;
		gthread_mutex_init(&m_condLock,0);
		gthread_mutex_init(&m_lock,0);
		gthread_cond_init(&m_cond,0);
	}
	void AddInterface(InterfaceInstance *pI)
	{
		// m_lstInterfaces should be empty

		m_nThreadMode = pI->GetThreadMode();
		pI->InUse(0);
		gthread_mutex_lock(&m_lock);
		m_lstInterfaces.AddLast(pI);
		gthread_mutex_unlock(&m_lock);
	}

	void ReturnInterface(InterfaceInstance *pI)
	{
		// all interfaces of a like type behave the same
		m_nThreadMode = pI->GetThreadMode();
		if (m_nThreadMode == 1)
		{
			// signal the return of the one and only
			pI->InUse(0);	
			gthread_cond_signal( &m_cond ); 
		}
		if (m_nThreadMode == 2)
		{
			// nothing to do
		}
		if (m_nThreadMode == 3 )
		{
			pI->InUse(0);	
		}
		if (m_nThreadMode == 4 )
		{
			delete pI;
		}
	}

	InterfaceInstance *GetInterface(const char *pzModelType)
	{
		if (m_nThreadMode == 1 || m_nThreadMode == 2)
		{
			if(m_nThreadMode == 1)
			{
				// wait for the one and only
				gthread_cond_wait(&m_cond, &m_condLock); 
			}
			InterfaceInstance *pII = (InterfaceInstance *)m_lstInterfaces.First();
			pII->InUse(1);
			return pII;
		}
		if (m_nThreadMode == 3)
		{
			// find one that's not in use
			gthread_mutex_lock(&m_lock);
			InterfaceInstance *pII = 0;
			GListIterator it(&m_lstInterfaces);
			while (it())
			{
				pII = (InterfaceInstance *)it++;
				if ( !pII->InUse() )
				{
					pII->InUse(1);
					gthread_mutex_unlock(&m_lock);
					return pII;	// and return it
				}
			}

			// unlock the mutex while we create a new instance, this will take a while 
			gthread_mutex_unlock(&m_lock);
			pII = new InterfaceInstance(pzModelType);
			pII->InUse(1);
			gthread_mutex_lock(&m_lock);

			// store the instance then return it
			m_lstInterfaces.AddLast(pII);
			gthread_mutex_unlock(&m_lock);
			return pII;
		}
		else if (m_nThreadMode == 4)
		{
			return new InterfaceInstance(pzModelType);
		}

		return 0;
	}
};

class CLanguageDriverInterfaceCache
{
	// hash to the pool based on the rule script name
	GHash m_hashCache; 
	gthread_mutex_t m_lock;

public:
	~CLanguageDriverInterfaceCache()
	{
		gthread_mutex_destroy(&m_lock);
	}
	CLanguageDriverInterfaceCache()
	{
		_gthread_processInitialize();
		gthread_mutex_init(&m_lock,0);
	}
	void ReturnInterfaceInstance(InterfaceInstance *pII)
	{
		gthread_mutex_lock(&m_lock);
		InterfaceController *pIC = (InterfaceController *)m_hashCache.Lookup(pII->GetDriverName());
		gthread_mutex_unlock(&m_lock);

		if (pIC)
		{
			pIC->ReturnInterface(pII);
		}
		else
		{
			// throw
		}
	}

	InterfaceInstance *GetInterfaceInstance(const char *pzModelType)
	{
		gthread_mutex_lock(&m_lock);
		InterfaceController *pIC = (InterfaceController *)m_hashCache.Lookup(pzModelType);
		gthread_mutex_unlock(&m_lock);

		if (!pIC)
		{
			
			// add a new one
			pIC = new InterfaceController();
			pIC->AddInterface( new InterfaceInstance(pzModelType) );
			gthread_mutex_lock(&m_lock);
			m_hashCache.Insert(pzModelType, (void *)pIC);
			gthread_mutex_unlock(&m_lock);
		}

		return pIC->GetInterface(pzModelType);
	}
};

static CLanguageDriverInterfaceCache m_LangDriverCache;


// You MUST call ReleaseInterface() with this return 
// pointer when you are finished with it.  Do NOT keep 
// an InterfaceInstance * for any duration of time.  
// Get it, use it, then return it quickly since it may be 
// a shared resource for other threads as well.
InterfaceInstance *GetInterfaceInstance(const char *pzModelType)
{
	return m_LangDriverCache.GetInterfaceInstance(pzModelType);
}

void ReleaseInterface(InterfaceInstance *pII)
{
	m_LangDriverCache.ReturnInterfaceInstance(pII);
}
