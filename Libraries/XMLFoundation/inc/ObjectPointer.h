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
#ifndef __OBJECT_PTR_H
#define __OBJECT_PTR_H

template <class TYPE> class ObjectPtr
{
	XMLObject *m_ObjectPointer;

	// Safely widen the pointer, even when multiply inherited
	TYPE *widen() 
	{
		if (!m_ObjectPointer)
		{
			return 0;
		}
		void *p = ((XMLObject *)m_ObjectPointer)->GetUntypedThisPtr();
		return (TYPE *)p;
	}
public:
	void assign(XMLObject *pNew){ if (pNew) pNew->RegisterObject(); m_ObjectPointer = pNew;}
	bool isNull()				{ return ( m_ObjectPointer == 0 ); }
	bool isValid()				{ return !isNull(); }
	ObjectPtr()					{ m_ObjectPointer = 0;}
	
	~ObjectPtr()				{ } //  m_ObjectPointer is still valid.
	// the object pionted to is not destroyed by the pointer it is destroyed by the creator

	
	// Auto-Widen the pointer.
	TYPE *operator ->()			{ return widen(); }
	operator TYPE *()			{ return widen(); }
	operator TYPE &()			{ return *(widen()); }


	// pass the address of our pointer into MapMember so the Object factory
	// can point us to the correct location of the object we want to reference.
	operator XMLObject **()		{ return  &m_ObjectPointer;	}
	XMLObject ** operator &()	{ return  &m_ObjectPointer;	}
};

#endif //__OBJECT_PTR_H
