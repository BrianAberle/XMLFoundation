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
#include "ObjectModel.h"
#include "MemberDescriptor.h"

//				  Object			 XML Element
IMPLEMENT_FACTORY(MyCustomer,		Customer)
IMPLEMENT_FACTORY(MyOrder,			Order)
IMPLEMENT_FACTORY(MyOrderLineItem,	LineItem)
IMPLEMENT_FACTORY(MyOptionalDetail, SalesDetail)
IMPLEMENT_FACTORY(AllMyCustomers,	ObjectsFromXMLSample)
IMPLEMENT_FACTORY(SimpleContainer,	SimpleContainer)
IMPLEMENT_FACTORY(SimpleItem,		SimpleItem)
IMPLEMENT_FACTORY(AnyObject,		AnyObject);
IMPLEMENT_FACTORY(PointsInTime,		PointsInTime);
IMPLEMENT_FACTORY(MyDataExample,	AttributeExample);

// for tracking actual instance counts
int MyCustomer::nInstances = 0;
int MyOrder::nInstances = 0;
int MyOrderLineItem::nInstances = 0;
int MyOptionalDetail::nInstances = 0;
int AllMyCustomers::nInstances = 0;
int SimpleContainer::nInstances = 0;
int SimpleItem::nInstances = 0;
int AnyObject::nInstances = 0;
int PointsInTime::nInstances = 0;
int MyDataExample::nInstances = 0;
int g_verify0ref = 0;



void PointsInTime::MapXMLTagsToMembers()
{
	MapMember(&m_lstPoints,	 AnyObject::GetStaticTag());
}
void AnyObject::MapXMLTagsToMembers()
{
	MapMember(&str1,	"One");
	MapMember(&str2,	"Two");
	MapObjectID("one",0);
}


void SimpleContainer::MapXMLTagsToMembers()
{
	MapMember(&m_List1,	 MyOrderLineItem::GetStaticTag(),  "List1");
	MapMember(&m_List2,	 SimpleItem::GetStaticTag(),	"List2");
	MapMember(&m_List3,	 SimpleItem::GetStaticTag(),	"List3");

}
void SimpleItem::MapXMLTagsToMembers()
{
	MapMember(&m_strSimple,	"SimpleMember");
}


void AllMyCustomers::MapXMLTagsToMembers()
{

	MapMember(&m_lstCustomers,		MyCustomer::GetStaticTag());
}

void MyOrderLineItem::MapXMLTagsToMembers()
{
	//		   Member			XML Element
	MapMember(&m_strUnitPrice,	"UnitPrice");
	MapMember(&m_nProductID,	"ProductID");
	MapMember(&m_strDescription,"Description");
	MapObjectID("ProductID",1,"UnitPrice",1);
}
void MyOrder::MapXMLTagsToMembers()
{
	MapMember(&m_strShippedDate,	"ShippedDate");
	MapMember(&m_strOrderDate,		"OrderDate");
	MapMember(&m_lstLineItems,		MyOrderLineItem::GetStaticTag());
	MapMember(&m_ptrExample,		MyOptionalDetail::GetStaticTag());
}
void MyCustomer::MapXMLTagsToMembers()
{
	MapMember(&m_strContactName,	"ContactName");
	MapMember(&m_strCity,			"City");
	MapMember(&m_strCountry,		"Country");
	MapMember(&m_lstOrders,			MyOrder::GetStaticTag() );
}
void MyOptionalDetail::MapXMLTagsToMembers()
{
	// Allow either "OtherUnitPriceLabel" or "UnitPrice" to map to this member
	// note: this requires the ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT); for right now.
	MapMember(&m_strSalesPerson, "OtherSalesPersonLabel");
	MapMember(&m_strSalesPerson, "SalesPerson");

	MapMember(&m_lstAssistedSalesPersons, "SalesPerson",  "AssistedList");
	MapMember(&m_ArrayDepartmentIDs,"id","DepartmentIDsArray");
	MapAttribute(&m_strSalesPerson, "SalesPerson");
}

void MyDataExample::MapXMLTagsToMembers()
{
	MapAttribute(&m_strSting1, "String1");
	MapMember(&m_strSting2, "String2");
}

#include "AbstractionsGeneric.h"
extern GenericStringAbstract gGenericStrHandler; // in XMLObject.cpp

MemberDescriptor *MyDataExample::HandleUnmappedMember( const char *pzTag )
{
	GString *pValue = new GString();
	MemberDescriptor *pMD = ::new MemberDescriptor(this, pzTag, (void *)pValue, &gGenericStrHandler);
	RegisterMember(pMD);
	m_listDynamicStringMembers.AddLast(pValue);
	return pMD;
}
