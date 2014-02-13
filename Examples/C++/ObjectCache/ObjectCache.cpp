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
#include "XMLFoundation.h"
#include "string.h"

#ifndef _WIN32
	#include <unistd.h> // for unlink() 
#endif 

// Any Object model will do, so borrow one from another project
#include "../ExObjsFromXML/ObjectModel.cpp"


// Init the Object Model with 2 customers each containing orders with line items and all objects contain ObjectID's
char p2CustomersOID[] = 
"<!DOCTYPE ObjectsFromXMLSample>"
"<ObjectsFromXMLSample>"
	"<Customer oid='1'>"
		"<ContactName>Maria Anders</ContactName>"
		"<City>Berlin</City>"
		"<Country>Germany</Country>"
		"<Order oid='11'>"
			"<OrderDate>1997-08-25</OrderDate>"
			"<ShippedDate>1997-09-02</ShippedDate>"
			"<LineItem>"
				"<ProductID>28</ProductID>"
				"<UnitPrice>45.6000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>39</ProductID>"
				"<UnitPrice>18.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>46</ProductID>"
				"<UnitPrice>12.0000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid='22'>"
			"<OrderDate>1997-10-03</OrderDate>"
			"<ShippedDate>1997-10-13</ShippedDate>"
			"<LineItem>"
				"<ProductID>63</ProductID>"
				"<UnitPrice>43.9000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid ='33'>"
			"<OrderDate>1997-10-13</OrderDate>"
			"<ShippedDate>1997-10-21</ShippedDate>"
			"<LineItem>"
				"<ProductID>3</ProductID>"
				"<UnitPrice>10.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>76</ProductID>"
				"<UnitPrice>18.0000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid='44'>"
			"<OrderDate>1998-01-15</OrderDate>"
			"<ShippedDate>1998-01-21</ShippedDate>"
			"<LineItem>"
				"<ProductID>59</ProductID>"
				"<UnitPrice>55.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>77</ProductID>"
				"<UnitPrice>13.0000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid='55'>"
			"<OrderDate>1998-03-16</OrderDate>"
			"<ShippedDate>1998-03-24</ShippedDate>"
			"<LineItem>"
				"<ProductID>6</ProductID>"
				"<UnitPrice>25.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>28</ProductID>"
				"<UnitPrice>45.6000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid='66'>"
			"<OrderDate>1998-04-09</OrderDate>"
			"<ShippedDate>1998-04-13</ShippedDate>"
			"<LineItem>"
				"<ProductID>58</ProductID>"
				"<UnitPrice>13.2500</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>71</ProductID>"
				"<UnitPrice>21.5000</UnitPrice>"
			"</LineItem>"
		"</Order>"
	"</Customer>"
	"<Customer oid='2'>"
		"<ContactName>Ana Trujillo</ContactName>"
		"<City>M&#233;xico D.F.</City>"
		"<Country>Mexico</Country>"
		"<Order oid='77'>"
			"<OrderDate>1996-09-18</OrderDate>"
			"<ShippedDate>1996-09-24</ShippedDate>"
			"<LineItem>"
				"<ProductID>69</ProductID>"
				"<UnitPrice>28.8000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>70</ProductID>"
				"<UnitPrice>12.0000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid ='88'>"
			"<OrderDate>1997-08-08</OrderDate>"
			"<ShippedDate>1997-08-14</ShippedDate>"
			"<LineItem>"
				"<ProductID>14</ProductID>"
				"<UnitPrice>23.2500</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>42</ProductID>"
				"<UnitPrice>14.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>60</ProductID>"
				"<UnitPrice>34.0000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid='99'>"
			"<OrderDate>1997-11-28</OrderDate>"
			"<ShippedDate>1997-12-12</ShippedDate>"
			"<LineItem>"
				"<ProductID>32</ProductID>"
				"<UnitPrice>32.0000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid='100'>"
			"<OrderDate>1998-03-04</OrderDate>"
			"<ShippedDate>1998-03-11</ShippedDate>"
			"<LineItem>"
				"<ProductID>11</ProductID>"
				"<UnitPrice>21.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>13</ProductID>"
				"<UnitPrice>6.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>19</ProductID>"
				"<UnitPrice>9.2000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>72</ProductID>"
				"<UnitPrice>34.8000</UnitPrice>"
			"</LineItem>"
		"</Order>"
	"</Customer>"
"</ObjectsFromXMLSample>";



char pCustomerUpdate[] = 
"<!DOCTYPE ObjectsFromXMLSample>"
"<ObjectsFromXMLSample>"
	"<Customer oid='1'>"
		"<City>Antioch</City>"
		"<Country>California</Country>"
	"</Customer>"
"</ObjectsFromXMLSample>";


char pzCacheQuery[] = 
"<!DOCTYPE ObjectsFromXMLSample>"
"<ObjectsFromXMLSample>"
	"<Customer oid='1'/>"
"</ObjectsFromXMLSample>";


char pNewCustomer[] = 
"<!DOCTYPE ObjectsFromXMLSample>"
"<ObjectsFromXMLSample>"
	"<Customer oid='3'>"
		"<ContactName>New Dude</ContactName>"
		"<City>Chocolate City</City>"
		"<Country>Hipville</Country>"
		"<Order oid='11'>"
			"<OrderDate>1997-08-25</OrderDate>"
			"<ShippedDate>1997-09-02</ShippedDate>"
			"<LineItem>"
				"<ProductID>28</ProductID>"
				"<UnitPrice>45.6000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>39</ProductID>"
				"<UnitPrice>18.0000</UnitPrice>"
			"</LineItem>"
			"<LineItem>"
				"<ProductID>46</ProductID>"
				"<UnitPrice>12.0000</UnitPrice>"
			"</LineItem>"
		"</Order>"
		"<Order oid='22'/>"
		"<Order oid='789'>"
			"<OrderDate>Tomorrow</OrderDate>"
			"<ShippedDate>Futuristic</ShippedDate>"
			"<LineItem>"
				"<ProductID>1234567</ProductID>"
				"<UnitPrice>1234567.77</UnitPrice>"
			"</LineItem>"
		"</Order>"
	"</Customer>"
"</ObjectsFromXMLSample>";


char pTest1[] = 
"<!DOCTYPE ObjectsFromXMLSample>"
"<ObjectsFromXMLSample>"
	"<Customer oid='1'>"
		"<ContactName>The Boss</ContactName>"
		"<Order oid='11'>"
			"<OrderDate>The near future</OrderDate>"
			"<LineItem>"
				"<ProductID>46</ProductID>"
				"<UnitPrice>12.0000</UnitPrice>"
				"<Description>Update This.</Description>"
			"</LineItem>"
		"</Order>"
	"</Customer>"
"</ObjectsFromXMLSample>";


char pTest2[] = 
"<!DOCTYPE ObjectsFromXMLSample>"
"<ObjectsFromXMLSample>"
	"<Customer oid='1'>"
		"<Country>Earth</Country>"
		"<Order oid='11'>"
			"<ShippedDate>Yesterday</ShippedDate>"
			"<LineItem>"
				"<ProductID>46</ProductID>"
				"<UnitPrice>12.0000</UnitPrice>"
				"<Description/>"
			"</LineItem>"
		"</Order>"
	"</Customer>"
"</ObjectsFromXMLSample>";


char pAAA[] = 
"<PointsInTime>"
	"<AnyObject oid='1'>"
		"<One>1</One>"
		"<Two>1</Two>"
	"</AnyObject>"
	"<AnyObject oid='2'>"
		"<One>2</One>"
		"<Two>2</Two>"
	"</AnyObject>"
	"<AnyObject oid='3'>"
		"<One>3</One>"
		"<Two>3</Two>"
	"</AnyObject>"
"</PointsInTime>";

char pBBB[] = 
"<PointsInTime>"
	"<AnyObject oid='1'>"
		"<One>1</One>"
		"<Two>111</Two>"
	"</AnyObject>"
	"<AnyObject oid='2'>"
		"<One>2</One>"
		"<Two>222</Two>"
	"</AnyObject>"
	"<AnyObject oid='3'>"
		"<One>3</One>"
		"<Two>333</Two>"
	"</AnyObject>"
"</PointsInTime>";


char pSimpleXML[] = 
"<SimpleContainer>"
	"<List1>"
		"<LineItem>"
			"<ProductID>918</ProductID>"
			"<UnitPrice>Simple</UnitPrice>"
		"</LineItem>"
	"</List1>"
	"<List2>"
		"<SimpleItem oid='7'>"
			"<SimpleMember>With OID</SimpleMember>"
		"</SimpleItem>"
	"</List2>"
	"<List3>"
		"<SimpleItem>"
			"<SimpleMember>VOID of OID</SimpleMember>"
		"</SimpleItem>"
	"</List3>"
"</SimpleContainer>";

char pSimpleXML2[] = 
"<SimpleContainer>"
	"<List1>"
		"<LineItem oid='918Simple'>"
			"<ProductID>918</ProductID>"
			"<UnitPrice>Simple</UnitPrice>"
			"<Description>Update this one</Description>"
		"</LineItem>"
		"<LineItem oid='777Root'>"
			"<ProductID>777</ProductID>"
			"<UnitPrice>Root</UnitPrice>"
			"<Description>Here is a new one</Description>"
		"</LineItem>"
	"</List1>"
"</SimpleContainer>";


char pSimpleXML2Improper[] = 
"<SimpleContainer>"
	"<List1>"
		"<LineItem oid='918Simple'>"
			"<ProductID>777</ProductID>"
			"<UnitPrice>Simple</UnitPrice>"
			"<Description>Improper</Description>"
		"</LineItem>"
	"</List1>"
"</SimpleContainer>";

char pSimpleXML2Proper[] = 
"<SimpleContainer>"
	"<List1>"
		"<LineItem>"
			"<ProductID>777</ProductID>"
			"<UnitPrice>Simple</UnitPrice>"
			"<Description>Improper</Description>"
		"</LineItem>"
	"</List1>"
"</SimpleContainer>";



char pSimpleXML3[] = 
"<SimpleContainer>"
	"<List1>"
		"<LineItem oid='918Simple'>"
			"<ProductID>918</ProductID>"
			"<UnitPrice>Simple</UnitPrice>"
		"</LineItem>"
		"<LineItem>"
			"<ProductID>777</ProductID>"
			"<UnitPrice>Root</UnitPrice>"
		"</LineItem>"
		"<LineItem oid='123Tester'>"
			"<ProductID>123</ProductID>"
			"<UnitPrice>Tester</UnitPrice>"
			"<Description>Here is the newest one</Description>"
		"</LineItem>"
	"</List1>"
"</SimpleContainer>";


char pDataExample[] = 
"<!DOCTYPE DataExample>"
"<DataExample String1='Value of First String'  UnmappedAttribute='UnmappedAttribute value'>"
	"This is the object Value"
	"<String2>Second String</String2>"
	"<UnmappedMember>UnmappedMember Value</UnmappedMember>"
"</DataExample>";



int g_nTestsPassed = 0;
int g_nTestsFailed = 0;
void CompleteTest(GString strOutput, const char *pzTestName, GString strPath, int bRecord)
{
	GString strFile;
	strFile << strPath << pzTestName << ".txt";
	if (bRecord)
	{
		strOutput.ToFile(strFile);
	}
	else
	{
		if ( strOutput.CompareToFile(strFile) != 0 )
		{
			g_nTestsFailed++;
			strFile << ".fail.txt";
			printf("[%s] unexpected results[%s]\n",pzTestName,strFile.StrVal());
			strOutput.ToFile(strFile);
		}
		else
		{
			g_nTestsPassed++;
		}
	}
}
void CompleteTest(XMLObject &pO, const char *pzTestName, GString strPath, int bRecord)
{
	GString strFile;
	GString strDump;

	for(int i =0; i < 2; i++)
	{
		strDump = "";
		strFile = "";
		strFile << strPath << pzTestName;
		if (i ==0)
		{
			strFile << "A.txt";
			pO.Dump(strDump);
		}
		else
		{
			strFile << "B.txt";
			pO.ToXML(&strDump,0,0,FULL_SERIALIZE|WINDOWS_LINEFEEDS);
		}

		if (bRecord)
		{
			strDump.ToFile(strFile);
		}
		else
		{
			if ( strDump.CompareToFile(strFile) != 0 )
			{
				g_nTestsFailed++;
				strFile << ".fail.txt";
				printf("[%s%c] unexpected results[%s]\n",pzTestName,65+i,strFile.StrVal());
				strDump.ToFile(strFile);
			}
			else
			{
				g_nTestsPassed++;
			}
		}
	}
}

int finalInstanceCountTest()
{
	g_verify0ref = 1;

	// test temp objects put in lists, then is the list updated with 'real' references
	printf("\n\nDone. Active instance counts follow:\n");
	if (MyCustomer::nInstances)
		printf("MyCustomer      =%d\n",MyCustomer::nInstances);
	if (MyOrder::nInstances)
		printf("MyOrder         =%d\n",MyOrder::nInstances);
	if (MyOrderLineItem::nInstances)
		printf("MyOrderLineItem =%d\n",MyOrderLineItem::nInstances);
	if (MyOptionalDetail::nInstances)
		printf("MyOptionalDetail=%d\n",MyOptionalDetail::nInstances);
	if (AllMyCustomers::nInstances)
		printf("AllMyCustomers  =%d\n",AllMyCustomers::nInstances);
	if (SimpleContainer::nInstances)
		printf("SimpleContainer =%d\n",SimpleContainer::nInstances);
	if (SimpleItem::nInstances)
		printf("SimpleItem      =%d\n",SimpleItem::nInstances);
	printf("---------------------          verify all reach 0:\n");

	return 0;
}



// The best way to learn this code, is to put a breakpoint at the first line of main() then step through it.
// look at the test results for each step and read why they are what they are in the comments in this code.
GString g_strObjCacheLogFile;
int main(int argc, char* argv[])
{
try
{
	// This application is designed to provide automated regression testing.
	// It runs in 2 modes (1=Record results, 0=Compare to last known goods results)
	// If you re-record the expected results, You must manually verify that they are correct
	// by stepping through this application and visually inspecting that the output is correct.
	int bRecord = 0; // 1=Record results, 0=Compare to prior results 
#ifdef _WIN32
	GString strTestPath(".\\ObjectCacheTestResults\\"); 
#else
	GString strTestPath("./ObjectCacheTestResults/"); 
#endif

	g_strObjCacheLogFile << strTestPath << "LogFile.txt";
	unlink(g_strObjCacheLogFile);
	SetLogFile(g_strObjCacheLogFile);

	// ---------------------------------------------------------------------------------
	if (bRecord)
		printf("Generating new test results...\n");
	else
	{
		printf("Comparing system operation to designed behavior...\n");
		printf("using test results files at[%s]\n",(const char *)strTestPath);
	}

	// ----------------------------------------------------------------------------
	// An OID is just a unique identifier.  It can be defined 1. by the object or 2. by the data(the XML)
	// or it can be 3. undefined(no oid) or it can be defined 4. by BOTH the data and the object.
	// It is a central concept to understanding designed behavior of object caching, object indexing and reference counting.

	// The OID of the 'MyOrderLineItem' is NOT expected to be defined in the XML,
	// MyOrderLineItem::MapXMLTagsToMembers() called MapObjectID() to define the OID.  In this case,
	// it uses a combination of the "ProductID" and "UnitPrice", so a price change constitutes a different object.

	// Other objects in this example 'Customer' and 'Order' are assigned OID's in the XML.
	// neither MyOrder::MapXMLTagsToMembers() nor MyCustomer::MapXMLTagsToMembers() calls MapObjectID()
	// rather the XML looks like this: <Customer oid='1'> and <Order oid='11'>
	//
	// NOTE: XML defined OID's take precedence over object defined oid's.
	// NOTE: Performance of either OID definition is very fast, however OID's defined in the XML are faster.  The difference would rarely be measurable.
	// ----------------------------------------------------------------------------


	// SimpleContainer contains 3 lists of objects.
	// List1 is a list of 'MyOrderLineItem' (oid defined by the object through MapObjectID())
	// List2 is a list of 'SimpleItem' (oid defined in the XML or undefined if missing)
	// List3 is a list of 'SimpleItem' (oid defined in the XML or undefined if missing)


	// Look at [pSimpleXML] it will
	// Add 1 object to each list from an XML source that (in the XML):
	// does not define an OID for the object that goes in List1		(defined in object)
	// does define an OID for the object that goes in List2			(defined in XML)
	// does not define an OID for the object that goes in List3		(not defined)
	SimpleContainer s;
	s.FromXML(pSimpleXML);
	CompleteTest(s, "Test001", strTestPath, bRecord);
	// now look at the results in "Test001", the first two have OID's of "918Simple" and"7" respectively.  The third has no OID.
	// also notice that the MyOrderLineItem is aware that "Description" did not exist in the XML as it has a state of (Clean | Null | Uncached)

	// update the object [s] with the exact same XML [pSimpleXML] and look again.
	s.FromXML(pSimpleXML);
	CompleteTest(s, "Test002", strTestPath, bRecord);
	// Now you can see that the objects in List1, and List2 are unchanged because the XML was unchanged.
	// Those objects were 'refreshed' with 'new' data that was equal to the value of the last object refresh.
	// Had the XML contained new values those new values would have been applied to the existing objects.
	// However List3 got a new object inserted into it.  It now has 2 identical items in the list.
	// List 3 was not updated because no OID was defined in the Object or the XML.
	// For List 3, if the desired behavior is an update, then you must manually clear List3 prior to the 'refresh' to prevent duplicate objects.
	// If the desired behavior is accumulation then it will happen naturally when no OID is defined.

	
	// Now test the situation where the OID is defined BOTH in the XML and by the object via MapObjectID(). So the OID is defined in 2 places.
	// Test this case with 2 objects where one exists in the cache and the other does not
	// [s] already contains "LineItem" with OID='918Simple', however the object was created by XML that did not define the OID
	// This shows how the two methods of OID definition may be used together.
	s.FromXML(pSimpleXML2);
	CompleteTest(s, "Test003", strTestPath, bRecord);
	// Notice that the first object was updated, it now contains the "Description" and still has a reference count of 1
	// and a new object (OID='777Root')was added to the list.
	// The update of the existing object was proper because the OID defined in the XML was equal to that of the MapObjectID() definition


	// Remember that the OID definition in the XML takes precedence over that of the object definition via MapObjectID()
	// In test003, we could not see the precedence order because both definitions were (correctly) the same so you must just believe that it used the XML OID definition.
	// Now we will do an "improper" update just to show the precedence.
	s.FromXML(pSimpleXML2Improper);
	CompleteTest(s, "Test004", strTestPath, bRecord);
	// Normally OID's should not change, they are like a primary index in the database and they normally have a direct correlation to DBMS indexes.
	// In this case the Object OID definition has been overridden by the XML
	// Had the MapObjectID() definition of the OID been used, this would have added a new object to List1 with an OID of "777Simple"
	// but since it used the attribute "oid" in the XML, the key information(product id) was changed to 777.

	// This uses the exact same XML as [pSimpleXML2Improper] but the OID has been removed from the XML, therefore the Object definition will create a new OID of "777Simple"
	// and add the new object to List1
	s.FromXML(pSimpleXML2Proper);
	CompleteTest(s, "Test005", strTestPath, bRecord);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	// Notice that we are adding objects to a new container now [s2] rather than [s]
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	// Use a new container so we can watch the reference counts change.
	SimpleContainer s2;
	s2.FromXML(pSimpleXML3); // 3 objects
	CompleteTest(s2, "Test006", strTestPath, bRecord);
	// notice that the first and second objects have a ref count of 2, 
	// This is because they exist in 2 places.  [s] - the first container, also contains those objects.
	// Also notice that the first object defined the oid in the XML, the second did not.  
	// The third object is new to the system (it resides only in [s2] not in [s]) so it has a refcount of 1.
	// List2 and List3 will both be empty in [s2]



	// now we'll use a more complex object relationship.
	// Create an Object Model with 2 customers, each Customer contains multiple orders.  Each Order contains LineItems.	 All objects contain ObjectID's

	AllMyCustomers query;
	query.FromXML(p2CustomersOID);
	// verify output that every refcount = 1 - except MyOrderLineItem with the OID 2845.6000 with a refcount of 2
	// That object uses MapObjectID() to define a unique key.
	// Orders 11 and 55 in the first customer both reference the object, so only 1 of them is instantiated - with a refcount 2.
	CompleteTest(query, "Test007", strTestPath, bRecord);

	// obtain object pointers from the query result.  The resultset [AllMyCustomers] contains the object reference
	// so the following 4 object pointers are guaranteed to be valid for as long as the resultset they are in exists.
	MyCustomer *pC = (MyCustomer *)query.GetCustomers()->First();
	MyCustomer *pC2 = (MyCustomer *)query.GetCustomers()->Last();
	MyOrder *pO = (MyOrder *)pC->GetOrderList()->First();
	MyOrder *pO2 = (MyOrder *)pC2->GetOrderList()->Last();


	// verify 1 of the 2 customers in the result
	CompleteTest(*pC, "Test008", strTestPath, bRecord);

	// Suppose another process updated the city and state values for one of the two customers in the database.
	// This means that objects in our resultset are now out of date and reflect old information.
	// There is no need to re-fetch the entire resultset, just the new information.  Likely invoked by a database trigger.
	// This updates the objects with the new data.
	query.FromXML(pCustomerUpdate);


	// see the new values applied to the already existing object, There is no need to go back to the resultset for a new object pointer.
	// RefCount is still only 1
	CompleteTest(*pC, "Test009", strTestPath, bRecord);



	// Suppose you wanted a pointer to an object that currently existed.  There are 3 ways to obtain a preexisting object reference:
	// The object we are looking for is a Customer object with an OID of "1". (that is [pC] from Test007)
	// The object already exists in the system, it was first created in [AllMyCustomers] in Test005

	// way 1 of 3 (by oid query)
	// First, we'll obtain the object reference through a new resultset.
	AllMyCustomers *pCacheQuery = new AllMyCustomers;
	pCacheQuery->FromXML(pzCacheQuery);
	MyCustomer *pCNewRef1 = (MyCustomer *)pCacheQuery->GetCustomers()->First();
	// since the new resultset 'owns' the object it raised the ref count for you
	// ref count now is 2, the object addresses are the same because there is only 1 object in the system.
	CompleteTest(*pCNewRef1, "Test010", strTestPath, bRecord);
	if (pCNewRef1 == pC && pCNewRef1->GetRefCount() == 2)
	{
		// as expected, memory address match and refcount is 2 .
	}
	else // so this code is never reached
	{
		printf("Test010 - failed.\n");
		return 0;
	}

	// way 2 of 3 (by Object Type)
	// It is not necessary to create a new resultset to obtain a new reference, You may obtain it directly from the cacheManager.
	MyCustomer *pCNewRef2 = (MyCustomer *)cacheManager.findObjectByType("MyCustomer", "1");
	// way 3 of 3 (by XML tag)
	MyCustomer *pCNewRef3 = (MyCustomer *)cacheManager.findObjectByTag("Customer", "1");
	// [pCNewRef2] and [pCNewRef3] are not owned by any resultset. The cacheManager raised the reference count for you
	// to ensure that the pointer will be valid for as long as you need it.  
	// When going directly to the cacheManager, it is your responsibility to decrement the reference count when you are finished with the object.

	// Note that all of the following pointers are the same:
	if (pC == pCNewRef1 && pC == pCNewRef2 && pC == pCNewRef3)
	{
		// as expected, memory address match
	}
	else // so this code is never reached
	{
		printf("Test010@2 - failed.\n");
		return 0;
	}
	// we'll dump out the new reference of this object so you can see that the reference count is now 4.
	// 1 reference is in [AllMyCustomers query] from Test005
	// 1 reference is in [AllMyCustomers *pCacheQuery] from Test008
	// 1 reference is in pCNewRef2
	// 1 reference is in pCNewRef3
	CompleteTest(*pCNewRef3, "Test011", strTestPath, bRecord);


	// now lets re-examine our [query] from Test005 that has 2 customers in it.
	// The 4 references are on the first Customer, the second should still have a reference count of 1
	CompleteTest(query, "Test012", strTestPath, bRecord);
	// notice ALL refcounts under Customer OID='1' raise to 4, even that of the Orders and LineItems, except the doubly-referenced 
	// MyOrderLineItem with OID 2845.6000 that goes to a refcount of 8



	// as this query is destroyed, refcount drops back to 3
	// but ALL object references(pointers) are still valid because they are equal and the refcount is > 0
	delete pCacheQuery;
	// pCNewRef1 is still valid even though it came from pCacheQuery that was just destroyed because it has references elsewhere
	CompleteTest(*pC, "Test013", strTestPath, bRecord);

	// now we'll let go of [pCNewRef2] and [pCNewRef3]
	pCNewRef2->DecRef();
	pCNewRef3->DecRef();



	// Now the refcount is back to 1 except for our funky doubly-referenced LineItem which is now back at refcount 2.
	CompleteTest(*pC, "Test014", strTestPath, bRecord);
	// and we can look at the query that contains both Customers(OID 1 and OID 2) as well.
	CompleteTest(query, "Test015", strTestPath, bRecord);



	// Object Updates can come from XML (often the datasource or database) or they can from from the user (often through a GUI)
	// [pO] is a MyOrder from Test005
	// We will update the 'Order' two different ways.
	//
	// This style of member assignment used XMLObject::SetMember(), so the system is aware that the object no longer reflects the XML that created it.
	// When the object is dumped you will see it has a state of (Dirty | Valid | Cached)  "Dirty" means it is out of sync with the datasource.
	// This indicates that our value is newer than that in the datasource, For example the user has made changes in the GUI but has not save to the database yet.
	// If we have an unsolicited update via XML, this value must not be replaced with that from the datastream.
	pO->SetOrderDate("Far out");				
	//
	// This style of member assignment does not inform the base class of any update.  It has a state of (Clean | Valid | Cached)
	pO->m_strShippedDate = "Member Management";
	CompleteTest(*pO, "Test016", strTestPath, bRecord);
	
	// The two members just changed are in Order OID='11', you'll see how it affects the next XML Update in a moment.




	// Suppose that the result set is of a type that may grow over time.
	// Refresh the query with data that contains a new Customer object.
	// The result set will now contain a new (3rd) object.
	// This new Customer has 3 Orders.
	// 1. The First Order, (oid=11) is an exact copy of an order existing in Customer #1
	// 2. the second, (oid=22) is an 'object marker' to an order existing in Customer #1
	// 3. the third is an Order (oid=789) new to the system.
	// //////////////////////////////////////////////////////////////
	// the first (oid 11)inefficiently added a reference to a cached object by resupplying all the data that we already have
	// the second (oid 22)efficiently had the same effect without resupplying the object state that we already had
	// the third (oid 789)is the first OID of it's type (a new instance in the system)
	// //////////////////////////////////////////////////////////////
	query.FromXML(pNewCustomer);

	// verify reference counts for (oid 11 & 22) are both 2 because they exist in 
	// customer #1 and in #3, the refcount for the third Order(oid=789) is only 1.
	// also our doubly-referenced LineItem (OID 2845.6000) is up by 1 to a refcount of 3.
	CompleteTest(query, "Test017", strTestPath, bRecord);


	// verify the dirty member(order date = 'far out') is NOT replaced by the update, 
	// the clean member 'Shipped Date = Member Management" is now 'Shipped Date = 1997-09-02'
	// "Order Date" value was not overwritten by the XML, but the "Shipped Date" value was.
	// this is because the uncommitted updates from the GUI have not been applied to the DBMS.
	// "Order Date" has a state of (Dirty) think of it as "uncommitted" so it is NOT updated with 
	// 'old' information while Shipped Date is (clean) so it was updated.
	CompleteTest(*pO, "Test018", strTestPath, bRecord);
	// see XMLObject::UserDefinedBehavior("XMLAssign") for custom behavior.





	// Objects are generally indifferent to their container, but they don't have to be.
	// [pO] is an Order with a RefCount of 2 because it exists within Customer OID 1 and Customer OID 3
	GList l;
	// Obtains a list of all objects that own a reference to this Order Object (there are 2 of them)
	GListIterator it(pO->GetParentObjects(&l)); 
	int n = 0;
	while (it())
	{
		GString strTest("Test019-"); // ensure that test files Test015-1 and Test015-2 are created, customer 1 and 3
		strTest << ++n;
		XMLObject *pNextObject = (XMLObject *)it++;

		// pointers to Customer 1 and 3 because (pO Refcount is = 2)
		CompleteTest(*pNextObject, strTest, strTestPath, bRecord);
		
	}
	// Often they only have 1 parent.
	// This returns a pointer to Customer oid 2 (pO2 Refcount is = 1)
	CompleteTest(*pO2->GetParentObject(), "Test020", strTestPath, bRecord);



	// Observe the customer before we begin...
	CompleteTest(*pC, "Test021", strTestPath, bRecord);


	AllMyCustomers query3;			// a fresh query
	// Get Customer #1, update his name to "The Boss".
	// The XML will attempt to update his Order #1's date to "The near future" but it cannot because the state is still "dirty" from Test012
	// Update the description of ProductID 46 to "Update This"
	// reference counts go up because the query3 now has a reference too. 
	query3.FromXML(pTest1);
	CompleteTest(*pC, "Test022", strTestPath, bRecord);


	// reset the dirty flag (simulating that the current state has been written to the database) then reassign the same XML to set the New Order Date
	pO->setMemberDirty(&pO->m_strOrderDate,0);
	query3.FromXML(pTest1);
	CompleteTest(*pC, "Test023", strTestPath, bRecord);
	// now verify that the order date from xml[pTest1] was applied to the object as "The near future"



	// Updates the same objects, sets Country to 'Earth', ShippedDate to 'Yesterday', 
	// It also removes the line item description for #46 with shorthand
	// termination in the XML.  Refcounts are unchanged since the query3 already 
	// had the object references.
	// Note that "<Description/>" is the same as "<Description></Description>" (both indicating an empty value) 
	//  - but quite different than nothing being supplied for "Description" (indicating no change to the value)
	query3.FromXML(pTest2);
	// verify both updates, one new reference.
	CompleteTest(*pC, "Test024", strTestPath, bRecord);



	// suppose you wanted to store multiple instances of the same object in different points in time
	// This example has OID's doubly defined, in the object with MapObjectID() and in the XML, but the
	// instances are NEVER cached and every reference is a new instance this is typically
	// how historical data is stored for presentation.
	// This is done through ModifyObjectBehavior(PREVENT_AUTO_CACHE)
	PointsInTime points;
	points.FromXML(pAAA); // three new instances
	points.FromXML(pBBB); // three new instances of existing objects
	CompleteTest(points, "Test025", strTestPath, bRecord);

	// NOTE: the objects can still be doubly referenced even though they are not cached.
	// For example, if another thread needed to go run some task on one of the 'historic'
	// instances, you should IncRef() the single object of interest and this 'PointsInTime'
	// container may be destroyed and only the single object will outlive the rest.



	// serialize with the default settings
	CompleteTest(*pC, "Test026", strTestPath, bRecord);

	// Write out everything - order the XML tags alphabetically, by default they are in the order of the calls to MapMember()
	GString strTest027;
	pC->ToXML(&strTest027,0,0,FULL_SERIALIZE | ORDER_MEMBERS_ALPHABETICALLY);
	CompleteTest(strTest027, "Test027", strTestPath, bRecord);

	// with no Serialization flags it only writes out dirty members.
	// This allows you to serialize out only what has been changed for efficient database updates.
	// This object has no dirty members, and therefore should write out no data
	GString strTest028;
	pC->ToXML(&strTest028,0,0,0); 
	CompleteTest(strTest028, "Test028", strTestPath, bRecord);

	pO->SetOrderDate("Powerful Object Serialization Control"); // this sets 1 member in 1 sub-object of this Customer dirty

	// with no Serialization flags it only writes out dirty members.
	// This time we should see our dirty member.
	GString strTest029;
	pC->ToXML(&strTest029,0,0,0); 
	CompleteTest(strTest029, "Test029", strTestPath, bRecord);

	GString strTest030;
	pC->ToXML(&strTest030,0,0,INCLUDE_DOCTYPE_DECLARATION | FULL_SERIALIZE);
	CompleteTest(strTest030, "Test030", strTestPath, bRecord);

	GString strTest031;
	pC->ToXML(&strTest031,0,0,NO_WHITESPACE | FULL_SERIALIZE);
	CompleteTest(strTest031, "Test031", strTestPath, bRecord);

	GString strTest032;
	pC->ToXML(&strTest032,0,0,NO_EMPTY_STRINGS | FULL_SERIALIZE);
	CompleteTest(strTest032, "Test032", strTestPath, bRecord);

	// This object holds several different kinds of data
	MyDataExample MyData;
	MyData.FromXML(pDataExample);
	CompleteTest(MyData, "Test033", strTestPath, bRecord);

	GString strTest034;
	MyData.ToXML(&strTest034,0,0,EXCLUDE_MAPPED_ATTRIBUTES| FULL_SERIALIZE);
	CompleteTest(strTest034, "Test034", strTestPath, bRecord);

	GString strTest035;
	MyData.ToXML(&strTest035,0,0,EXCLUDE_UNMAPPED_ATTRIBUTES| EXCLUDE_OBJECT_VALUE| FULL_SERIALIZE);
	CompleteTest(strTest035, "Test035", strTestPath, bRecord);

	// do not include this member
	MyData.SetMemberSerialize("String2",0);
	GString strTest036;
	MyData.ToXML(&strTest036,0,0,FULL_SERIALIZE);
	CompleteTest(strTest036, "Test036", strTestPath, bRecord);



	////////////////////////////////////////////////////////////////////////////////////////////////
	// finalInstanceCountTest() will display the numbers of object instances that exist at this point.
	// You will see something like:
	//
	/*
		MyCustomer      =3			
		MyOrder         =11			
		MyOrderLineItem =26			
		AllMyCustomers  =2			
		SimpleContainer =2			
		SimpleItem      =3			
		---------------------
	*/
	//
	//  Then all object containers (like SimpleContainer and AllMyCustomers) will go out of scope
	//  causing them to decrement ref counts on the objects that they contain (like MyCustomer and MyOrder)
	//  whenever a refcount hits 0, the system deletes the object causing it's destructor to be called.
	//  For this example, every object destructor prints out a notice when the instance count reaches 0.
	//  So you should see the following:
	//
	/*
		MyDataExample    0 Refcount
		PointsInTime     0 Refcount
		AnyObject        0 Refcount
		AllMyCustomers   0 Refcount
		MyOrder          0 Refcount
		MyCustomer       0 Refcount
		SimpleContainer  0 Refcount
		MyOrderLineItem  0 Refcount
		SimpleContainer  0 Refcount	
	*/
	// 
	//  Notice that the final instance count is ONLY DISPLAYED IF IT REACHES 0,
	//  If any object did not reach a refcount of 0 as it should, IT WILL NOT APPEAR IN THE LIST.
	//  You must see all 9 listed, if 1 is missing that object still has 1 or more references.
	//
	////////////////////////////////////////////////////////////////////////////////////////////////
	if (bRecord == 0)
	{
		printf("\n\n[%d] Tests produced the expected results. [%d] failed.\n",g_nTestsPassed,g_nTestsFailed);
	}

	return finalInstanceCountTest();
	////////////////////////////////////////////////////////////////////////////////////////////////


	// also - the Framework logging was enabled to write to LogFile.txt where you should see this:
	/*

	**MESSAGE: ****** Begin Object Instance Cache Dump ******
	**MESSAGE: ****** End Object Instance Cache Dump ******
	**MESSAGE: ****** Begin Object State Cache Dump ******
	**MESSAGE: ****** End Object State Cache Dump ******
	**MESSAGE: ****** Begin Foreign Object Instance Cache Dump ******
	**MESSAGE: -----------------------------------------------

	**MESSAGE: ****** End Foreign Object Instance Cache Dump ******
	*/

	//	This indicates that there were no objects in the cache at shutdown, otherwise
	//	they will be listed by Type and Tag
}
catch( GException &rErr )
{
	printf( rErr.GetDescription() );
}
	
	return 0;
}

