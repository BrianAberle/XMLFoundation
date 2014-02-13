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

#include <stdlib.h> // for: getenv()
#include "XMLFoundation.h"
#include "GPerformanceTimer.h" 

//#include "MyObjects.h"   // Declare User Objects, [MyOrder,MyCustomer,My*]
#include "ObjectModel.h"   // Declare User Objects, [MyOrder,MyCustomer,My*]
//#include "MyObjects.cpp" 


// Select a data sources derived from CXMLDataSource, or roll your own.
CSocketHTTPDataSource broker;
// CWinINetHTTPDataSource broker; // for NT users, supports SSL

// The purpose of this sample is to use an XML document as input
// into objects that are useful in the application layer.
// Several different sub-examples are shown -  they all work 
// on the same XML Document as input.  The structure of that
// input is listed in a comment following the } of main()
// The RunTime Object definition in ObjectModel.h will be created.
int main()
{
	// By default all input comes from a disk file, setting bIsLocal to false
	// causes the XML input source to be dynamically rendered from the sample
	// northwind database on MS Access or SQL Server with UBT's TransactXML Server.
	bool bIsLocal = true;
	
	try
	{
		// Setup the location of source xml input.  It can come from:
		// a remote TXML server, a local disk file, or a memory buffer.
		if (bIsLocal)
		{
			// look in "."(current path) for the input ObjectsFromXMLSample.xml
			broker.SetConnectionInfo(".", -1);
		}
		else
		{
			// Connect to a TXML Server (Read DynamicObjectsFromXML.html first)
			broker.SetConnectionInfo("123.456.789.123", 80);
			broker.SetUser("DesignerXML");
			broker.SetPwd("password");
		}

		printf("********************* Example #1 *************************\n");
		// Example of a raw xml retrieval.  pXML is released when 
		// the XMLProcedureCall goes out of scope.
		{
			GPerformanceTimer p("(from disk + printf) on 300k(~1/3Meg) in milliseconds");

			XMLProcedureCall proc(&broker,"ObjectsFromXMLSample");

			const char *pXML = proc.GetXML();
			// This line of code takes a while to execute since we have to move 300,000+ bytes 
			// to std out - it will account for CPU cycles and time reported by GPerformanceTimer
			printf(pXML); printf("\n\n");
		}

		printf("********************* Example #2 *************************\n");
		// Sample that creates system objects that contain the state of the xml
		// message.  This is typically used to query for a 'root' object or 
		// starting point into your object model, then "drill down accessors"
		// are added to each object for further navigation of the object model.
		MyCustomer *pCustRefCounting = 0;
		{
			ObjQuery<MyCustomer> qry(&broker, "ObjectsFromXMLSample");

			GPerformanceTimer p("From disk + 91 Cust, 830 Orders, 2155 Order Details");
			// source input file is read, parsed, and objects are created here
			ObjResultSet<MyCustomer> &resultSet = qry.Execute();

			MyCustomer *pCust;
			while ( ( pCust = resultSet.next() ) != 0 )
			{
				// For each MyCustomer object returned in the query....
				// If you want the Objects to live past the scope of the 
				// ObjQuery, call AddRef(), then call Release() when you 
				// are done with them.
				if (!pCustRefCounting)
				{
					// add ref only the first CTitle in the resultSet, for 
					pCustRefCounting = pCust;
					pCustRefCounting->IncRef();

					//pCustRefCounting->IncRef();
				}
			}
		} // life expectancy of all objects in the resultSet unless AddRef()'d

// To see the GPerformanceTimer times for #1 & #2 before data rolls off the screen
//return 1;   

		// From disk + Factory Create and populate 91 Cust, 830 Orders, 2155 Order Details takes:
		//---------------------------------------------------------------------------------------
		//On my PIII-600 Running Win2000 it took 0.261 Seconds
		//On my PIII-600 Running Linux it took 0.217 Seconds	(Same CPU, but Windows background tasks added overhead)
		//On my Sun Ultra Sparc-5 it took 0.960 Seconds
		//On my Dual Core 1.66Ghz running Vista it took 0.177 Seconds

		// write out a full debug dump of the object to std out or a disk file
		pCustRefCounting->Dump();
		// release object and all referenced objects
		pCustRefCounting->DecRef();



		// Example #3 shows an alternative to ObjQuery by supplying a container
		// object "AllMyCustomers" is used to hold the query results.
		//
		// It also shows how to map a pointer to data that may or may not be in the resultset
		// Embedded in the middle of the resultset is a special Order, that Order written
		// out in two forms, in XML, and in a detailed Debug Dump in "SpecialOrderXML.txt" and "SpecialOrderDump.txt"
		printf("********************* Example #3 *************************\n");
		// "AllMyCustomers" queries for more data into it's self on the stack
		{
			AllMyCustomers AllOfEm;
			// This code should be in AllOfEm.Load(), for the sample it's here

			XMLProcedureCall proc(&broker,"ObjectsFromXMLSample");
			proc.Execute(&AllOfEm,false); // The "result set" is the first param

			GListIterator it( AllOfEm.GetCustomers() );
			while (it())
			{
				MyCustomer *pCust = (MyCustomer *)it++;

				// Find the Customer-Order that has a MyOptionalDetail Object.
				// Open ObjectsFromXMLSample.xml in a text editor and search  for
				// "SalesDetail" to understand what we are looking for.
				GListIterator it( pCust->GetOrderList() );
				while (it())
				{
					MyOrder *pOrder = (MyOrder *)it++;
					if (pOrder->getExtraInfo())
					{
						// Write out the object in debug and XML formats
						pOrder->ToXMLFile("./SpecialOrderXML.txt");
						pOrder->Dump("./SpecialOrderDump.txt");
					}
				}
				
				// write out something to see that each object has it's state
				printf("%s%d\n",pCust->GetName(),pCust->GetOrderList()->Size());
			}
			// passing true releases only sub-object references, but does not
			// delete 'this' since it is on the stack it cleans up naturally.
			AllOfEm.DecRef(true); 
		}

		printf("********************* Example #4 *************************\n");
		// An Object that queries for more data into it's self on the heap
		{
			// Only diff from example 3 is this allocation (4=heap,3=stack)
			AllMyCustomers *pAllOfEm = new AllMyCustomers; 

			XMLProcedureCall proc(&broker,"ObjectsFromXMLSample");
			proc.Execute(pAllOfEm,false);

			GListIterator it( pAllOfEm->GetCustomers() );
			
			while (it())
			{
				MyCustomer *pCust = (MyCustomer *)it++;
				// write out something to see that the object has it's state
				printf(pCust->GetName()); printf(" ");
				printf("%d\n",pCust->GetOrderList()->Size());
			}
			
			// All of the objects referenced by 'pAllOfEm' will be Released()
			// but the 'root' AllMyCustomers object must be cleaned up by the 
			// application programmer since the Release(nUserOwnsObject) flag 
			// is true.  If that flag were false, the 'delete this' would be 
			// done by Release() so the next line of code (delete pLib;) should
			// then not exist.
			pAllOfEm->DecRef(true); 
			delete pAllOfEm;
		}
	}
	catch(GException &ex)
	{
		printf("%d %s",ex.GetError(),ex.GetDescription());
	}
	return 0;

}
/*
<ObjectsFromXMLSample>
	<Customer>
		<ContactName>Maria Anders</ContactName>
		<City>Berlin</City>
		<Country>Germany</Country>
		<Order>
			<OrderDate>1997-08-25</OrderDate>
			<ShippedDate>1997-09-02</ShippedDate>
			<LineItem>
				<ProductID>28</ProductID>
				<UnitPrice>45.60</UnitPrice>
			</LineItem>
			<LineItem>
				<ProductID>39</ProductID>
				<UnitPrice>18.00</UnitPrice>
			</LineItem>
			<LineItem>
				<ProductID>46</ProductID>
				<UnitPrice>12.00</UnitPrice>
			</LineItem>
		</Order>
		<Order>
			<OrderDate>1997-10-03</OrderDate>
			<ShippedDate>1997-10-13</ShippedDate>
			<LineItem>
				<ProductID>63</ProductID>
				<UnitPrice>43.90</UnitPrice>
			</LineItem>
		</Order>
	</Customer>
*/