#ifndef _OBJECT_MODEL_H__
#define _OBJECT_MODEL_H__

#include "xmlObject.h"
#include "GString.h"
#include "ObjectPointer.h"
#include "ObjectDataHandler.h"
#include "GArray.h"
extern int g_verify0ref;


class MyCustomer : public XMLObject
{
	GString m_strContactName;
	GString m_strCity;
	GString m_strCountry;
	
//	GString m_strTest;

	GList m_lstOrders; 

	virtual void MapXMLTagsToMembers();
public:
	static int nInstances;
	virtual long IncRef(StackFrameCheck *pStack =0, int nDeep=1)
	{
		return XMLObject::IncRef(pStack, nDeep);
	}
	// Object --> XML Element map
	DECLARE_FACTORY(MyCustomer, Customer);
	
	GList *GetOrderList() {return &m_lstOrders;}
	const char *GetName() {return m_strContactName;}
	void SetName(const char *p) {SetMember(&m_strContactName, p);}
	MyCustomer(){ ModifyObjectBehavior(MEMBER_UPDATE_NOTIFY); nInstances++; };
	~MyCustomer(){nInstances--;  if (!nInstances && g_verify0ref) printf("MyCustomer        0 Refcount\n");};
};

// This class is a container for MyCustomer Objects.
// If an ObjQuery<MyCustomer> is used, then the query
// becomes the container, and a "starting point" object 
// like this is never used.  MFC'ers may want the CWinApp
// derived object to contain the object model entry points.
// Example 3, and 4 use this approach rather than ObjQuery<>
class AllMyCustomers : public XMLObject
{
	GList m_lstCustomers; 
	virtual void MapXMLTagsToMembers();
public:
	static int nInstances;
	AllMyCustomers(){nInstances++;}
	~AllMyCustomers() {nInstances--;  if (!nInstances && g_verify0ref) 	printf("AllMyCustomers   0 Refcount\n"); DecRefContained();}
	
	// this style of destruction allows 'this' to be on the stack
	// when 'this' is used rather than an ObjQuery.
//	DecRefContained();

	// this object will destroy naturally if it's on the stack
	// or must be 'deleted' if it's on the heap like any object

	DECLARE_FACTORY(AllMyCustomers, ObjectsFromXMLSample);
	GList *GetCustomers(){return &m_lstCustomers;}
};


class AnyObject : public XMLObject
{
	GString str1;
	GString str2;
	virtual void MapXMLTagsToMembers();
public:
	static int nInstances;
	AnyObject()  {nInstances++; ModifyObjectBehavior(PREVENT_AUTO_CACHE);}
	~AnyObject() {nInstances--;  if (!nInstances && g_verify0ref) 	printf("AnyObject        0 Refcount\n"); }

	DECLARE_FACTORY(AnyObject, AnyObject);
};
class PointsInTime : public XMLObject
{
	GList m_lstPoints; 
	virtual void MapXMLTagsToMembers();
public:
	static int nInstances;
	PointsInTime()  {nInstances++;}
	~PointsInTime() {nInstances--;  if (!nInstances && g_verify0ref) 	printf("PointsInTime     0 Refcount\n"); }

	DECLARE_FACTORY(PointsInTime, PointsInTime);
};

class MyOrderLineItem : public XMLObject
{
	virtual void MapXMLTagsToMembers();
public:
	int m_nProductID;
	GString m_strUnitPrice;
	GString m_strDescription;
	static int nInstances;
	DECLARE_FACTORY(MyOrderLineItem, LineItem);

	// factory construction
	MyOrderLineItem(){nInstances++;}
	~MyOrderLineItem(){nInstances--;  if (!nInstances && g_verify0ref) printf("MyOrderLineItem  0 Refcount\n");}
	// user construction
	MyOrderLineItem(int nProductID, const char *pzUnitPrice, const char *pzDesc = 0)
	{
		nInstances++;
		m_nProductID = nProductID; 
		m_strUnitPrice = pzUnitPrice;
		m_strDescription = pzDesc;
	}
};


class MyOptionalDetail;
class MyOrder : public XMLObject
{
	virtual void MapXMLTagsToMembers();
public:
	GString m_strShippedDate; 
	static int nInstances;
	GString m_strOrderDate;
	// simple data types
	GList m_lstLineItems;	// contains MyOrderLineItem pointers
	
	// A MyOrder may have 0 or 1 MyOptionalDetail's associated to it.
	// Denotes a (0..1) relationship.  If the MyOptionalDetail exists
	// in the source XML stream, the object will be allocated
	// and the relationship is tied through this object pointer.
	ObjectPtr <MyOptionalDetail> m_ptrExample;
	// alternatively your object may use 'containment' like this:
	// MyOptionalDetail m_Example;
	// You should understand that the 'containment' object will always 
	// exist since MyOptionalDetail gets constructed at the same time as 
	// 'this' regardless of if the data for the object exists in the XML.
	 

public:
	MyOptionalDetail *getExtraInfo()
		{if (m_ptrExample.isValid()) return m_ptrExample; return 0;}


	DECLARE_FACTORY(MyOrder, Order);
	void SetOrderDate(const char *p) {SetMember(&m_strOrderDate, p);}
	void AddLineItem(int n, const char *s)
	{
		MyOrderLineItem *pLI = new MyOrderLineItem(n,s);
		m_lstLineItems.AddLast(pLI);
		pLI->setObjectDirty(1);
		setObjectDirty();
	}
	MyOrder(){nInstances++;}
	~MyOrder(){nInstances--;  if (!nInstances && g_verify0ref) printf("MyOrder          0 Refcount\n");}

	
	// Accessors
};


class MyOptionalDetail : public XMLObject
{
	GString m_strSalesPerson;
	GStringList m_lstAssistedSalesPersons;
	GArray m_ArrayDepartmentIDs;

	virtual void MapXMLTagsToMembers();
public:
	static int nInstances;
	DECLARE_FACTORY(MyOptionalDetail, SalesDetail);
	MyOptionalDetail(){ 
		// Jan 2014 note: Because this object is 'double mapping' to 1 member.  Perhaps it would be simple to accomplish 
		// this in a way that still uses block memory allocations.
		ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT); 
		nInstances++;}
	~MyOptionalDetail(){nInstances--;  if (!nInstances && g_verify0ref) printf("MyOptionalDetail  0 Refcount\n");}
};


class SimpleContainer : public XMLObject
{
	GList m_List1;
	GList m_List2;
	GList m_List3;
	virtual void MapXMLTagsToMembers();
public:
	static int nInstances;
	DECLARE_FACTORY(SimpleContainer, SimpleContainer);
	SimpleContainer(){nInstances++;}
	~SimpleContainer(){nInstances--;  if (!nInstances && g_verify0ref) printf("SimpleContainer  0 Refcount\n");}
};
class SimpleItem : public XMLObject
{
	GString m_strSimple;
	virtual void MapXMLTagsToMembers();
public:
	static int nInstances;
	DECLARE_FACTORY(SimpleItem, SimpleItem);
	SimpleItem(){nInstances++;}
	~SimpleItem(){nInstances--;  if (!nInstances && g_verify0ref) printf("SimpleContainer  0 Refcount\n");}
};

// maps member data, attribute data, and object data, handles unmapped data
class MyDataExample : public XMLObject, public XMLObjectDataHandler
{
	virtual void MapXMLTagsToMembers();
	GString m_strSting1;				// Member Data
	GString m_strSting2;				// Attribute Data
	GString m_strObjectValue;			// Object Data (design thought - often this means that you could have mapped 'this' as a member to another object)
	GList m_listDynamicStringMembers;	// Unmapped Data

	virtual void SetObjectValue(const char *strSource, __int64 nLength, int nType)
	{
		// You may need to strip CR's LF's Tabs's depending on your desired behavior
		m_strObjectValue = strSource;
	}
	virtual void AppendObjectValue(GString& xml)
	{
		xml << "\n\t";
		xml.AppendEscapeXMLReserved(m_strObjectValue);
	}
	virtual MemberDescriptor *HandleUnmappedMember( const char *pzTag );
//	{
//		GString *pValue = new GString();
//		MemberDescriptor *pMD = new MemberDescriptor(this, pzTag, (voidpValue, gG);
//		RegisterMember(pMD);
//		m_listDynamicStringMembers.AddLast(pValue);
//		return pMD;
//	}

public:
	static int nInstances;
	DECLARE_FACTORY(MyDataExample, AttributeExample);

	// factory construction
	MyDataExample()	{ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT); setObjectDataHandler(this);nInstances++;}
	~MyDataExample()
	{
		nInstances--;  
		if (!nInstances && g_verify0ref) 
			printf("MyDataExample  0 Refcount\n");
		GListIterator it(&m_listDynamicStringMembers);
		while(it())
		{
			delete (GString *)it++;
		}
	}
};


#endif //_OBJECT_MODEL_H__

