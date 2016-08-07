package a777.root.GApp;

import java.util.Iterator;
import java.util.Vector;

// MyLineItem has an "ObjectId", that is a value created from members and/or attributes of this
// object that uniquely identify it among all instances of it's own type.  "ObjectId"'s are
// optional but allow you to perform complex "object updates" easily through XML.  In main() below,
// this functionality is used when getXML2() is applied.  ObjectId's are like a database primary key
// in that they are generally not modified.
class MyLineItem extends XMLObject
{
    private String item;
    private String quantity;
    private int ItemID;
    MyLineItem()
    {
        // call the base class constructor with the XML-tag for 'this'
        super("LineItem");
    }
    MyLineItem(int nID, String itm, String qty)
    {
        super("LineItem");
        item = itm;
        quantity = qty;
        ItemID = nID;
    }

    void MapXMLTagsToMembers()
    {
        //	  	  member	member		xml-tag			(optional)wrapper
        //-----------------------------------------------------------------
        MapMember(quantity,	"quantity", "Quantity");
        MapMember(item,		"item",		"Description");
        MapMember(ItemID,	"ItemID",	"SKU");

        //////////////////////////////////////////////////////////////////
        // MapObjectId() is optional.
        //////////////////////////////////////////////////////////////////
        // MapObjectId() takes 1 to 5 arguments, for example:
        // MapObjectId("ItemID","item") creates an object identifier composed
        // of the combination of 2 member variables of any simple type
        // (long, double, short, byte, string, int, boolean). If multiple items
        // had the same "ItemID", and multiple could have the same description
        // but the combination of the two was unique this would be used.
        //////////////////////////////////////////////////////////////////
        MapObjectId(this, "ItemID"); // takes 1 to 5 mapped 'Key parts'
    }
    String getMyLineItem()
    {
        String sRet;
        sRet = "item= "+ item + "ItemID= "+ ItemID + "quantity= "+ quantity + "\n";
        return sRet;
    }
}

class MyOrder extends XMLObject
{
    public String salesPerson;
    public String orderDate;
    public int OrderID;
    public Vector vecLI;	// a non-preAllocated vector, aka: a null reference to a vector
    //	public HashSet hashLI;	// You can use (Vector|Stack|ArrayList|LinkedList|TreeSet|HashSet)
    MyOrder()
    {
        // call the base class constructor with an xml tag.
        super("Order");
    }
    void MapXMLTagsToMembers()
    {
        //	  	  member	   member			xml-tag			  ObjectType	    (optional)wrapper
        //-----------------------------------------------------------------
        MapAttrib(orderDate, "orderDate",       "TransactionTime");
        MapMember(salesPerson,"salesPerson",	"SalesPerson");
        MapMember(OrderID,	  "OrderID",		"OrderNumber");
        MapMember(vecLI,      "vecLI",          "LineItem",     "a777/root/GApp/MyLineItem", "LineItemContainer");
        // or we could put the objects into a Hash table as easily as a Vector like this...
		// MapMember(hashLI,	"hashLI",	    "LineItem", 	 "a777/root/GApp/MyLineItem",     "LineItemContainer");

    }
    void addLineItem(int nID, String item, String quantity)
    {
        if (vecLI == null)
            vecLI =  new Vector();
        vecLI.add(new MyLineItem(nID, item, quantity));
    }
}


class Customer1 extends XMLObject
{
    private String name;
    private int CustID;
    public MyOrder objOrderXXX = null; // in this example case, a customer may have 0..1 Orders

    Vector vecStrings;		// vector is allocated in the constructor

    long l;
    short s;
    double d;
    byte b;
    boolean z;

    MyOrder getOrder()
    {
        if (objOrderXXX == null)
            objOrderXXX =  new MyOrder();
        return objOrderXXX;
    }
    Customer1()
    {
        // call the base class constructor with the xmlTag for 'this'
        super("Customer");

        // this is optional, FromXML() will allocate containers if you don't
        // and it will use your containers if already allocated.
        vecStrings = new Vector();
    }

    // Direct inheritance acquires FromXML() and ToXML()
    // by virtue of the foundation. The following method gets called
    // automatically to build a compiled object-xml map.  This map is
    // used to automatically assign members variables when FromXML()
    // is used and get member variables when ToXML() is called.  This
    // method is by default only called 1 time to build a compiled
    // map.  In the rare case that you have an object with a dynamic
    // definition you will need to call DontCacheMemberMaps() from
    // within MapXMLTagsToMembers().
    void MapXMLTagsToMembers()
    {
        //System.err.println("Customer1::MapXMLTagsToMembers()");
        MainActivity.XMLSectionFragment.showResults("Customer1::MapXMLTagsToMembers");

        // =============== simple native datatypes ===============
        //		  member	MemberName    xml tag		(optional)wrapper
        // =======================================================
        MapMember(name,	   "name",	     "FirstName");
        MapMember(CustID,  "CustID",     "CustomerID");
        MapMember(l,	   "l",			 "long");
        MapMember(s,	   "s",			 "short");
        MapMember(d,	   "d",			 "double");
        MapMember(b,	   "b",			 "byte");
        MapMember(z,	   "z",			 "bool");

        // ================== object datatypes ========================
        //		  member	     MemberName     xml tag	   ObjectType	(optional)wrapper
        // ============================================================
        MapMember(objOrderXXX, "objOrderXXX",  "Order",	   "a777/root/GApp/MyOrder");

        // ================== object container datatypes ================
        //		  member	 MemberName       xml tag	   ObjectType,  (optional)wrapper
        // ============================================================
        MapMember(vecStrings,"vecStrings",  "StringItem",  "String",  "StringList Level2Wrapper");


        // The most recent call to MapMember() uses a 2-level wrapper.  The wrapper is
        // used for grouping like information.  If the data is a list of string values
        // the information is often grouped into a container element. for example:
        //<Customer>
        //	   <StringList>
        //			<StringItem>Hello</StringItem>
        //			<StringItem>World</StringItem>
        //	   </StringList>
        //</Customer>
        // would map to:
        // MapMember(vecStrings,"vecStrings","StringItem","String",  "StringList");
        //
        // if the <StringItem>'s repeated directly in the Customer like this:
        //  <Customer>
        //	    <StringItem>Hello</StringItem>
        //	    <StringItem>World</StringItem>
        //  </Customer>
        //
        // then you would map to:
        // MapMember(vecStrings,"vecStrings","StringItem","String");
        //
        //
        // wrappers may be infinately nested with a space sepertator so in the case of
        // this xml defined in TestMain::getXML1():
        //<Customer>
        //	   <StringList>
        //			<Level2Wrapper>
        //				<StringItem>Hello</StringItem>
        //				<StringItem>World</StringItem>
        //			</Level2Wrapper>
        //	   </StringList>
        //</Customer>
        // we map the space seperated string of nested wrapper tags.
        // MapMember(vecStrings,"vecStrings","StringItem","String",  "StringList Level2Wrapper");

    }

    public void ObjDump()
    {
        String sLSD; // The Long, Short, and Double
        sLSD = "--------- Customer1 Dump --------------";
        sLSD += l;
        sLSD += "~";
        sLSD += s;
        sLSD += "~";
        sLSD += d;
        sLSD += "\n";
        MainActivity.XMLSectionFragment.showResults(sLSD);

        // We saw some numeric members - see some other members
        String strMembers = "Name=";
        strMembers += name;
        strMembers += "   CustID=";
        strMembers += CustID;
        MainActivity.XMLSectionFragment.showResults(strMembers);


        // see if the Vector of Strings has the Data that the XML said it should.
        String strArr = "contains[";
        strArr += vecStrings.size();
        strArr += "]\n";

        int i = 0;
        for(i=0;i < vecStrings.size(); i++ )
        {
            String s = (String)vecStrings.elementAt(i);
            strArr += s;
            strArr += "\n";
        }
        MainActivity.XMLSectionFragment.showResults(strArr);


        // see if we have an Order Object - this object was allocated and instanciated inside the XMLFoundation
        String sTheOrder;
        if (objOrderXXX != null) { // it WAS null before calling FromXML() which made the allocation

            sTheOrder = "Order=[" + objOrderXXX.OrderID + " " + objOrderXXX.salesPerson + " " + objOrderXXX.orderDate + "]";
            MainActivity.XMLSectionFragment.showResults(sTheOrder);
            System.err.println(sTheOrder);

            // the Order object is complex, because it has LineItem Objects in a Vector
            if (objOrderXXX.vecLI != null) {
                Iterator it = objOrderXXX.vecLI.iterator();
                while (it.hasNext()) {
                    MyLineItem o = (MyLineItem) it.next();
                    MainActivity.XMLSectionFragment.showResults(o.getMyLineItem());
                    System.err.println(o.getMyLineItem());
                }
            }
        }
    }
}


// Not "derived from" but "contains" XML support.
class Customer2
{
    public String		name;
    private int			CustID;
    private XMLObject	ContainedXMLObj;
    public MyOrder		objOrder;
    private Vector		vecStrings;

    long l;
    short s;
    double d;
    byte b;
    boolean z;


    MyOrder getOrder()
    {
        if (objOrder == null)
            objOrder =  new MyOrder();
        return objOrder;
    }
    public void ObjDump()
    {
        System.err.println("---------------- Customer2 Dump ------------------");

        System.err.println(l);
        System.err.println(s);
        System.err.println(d);
        System.err.println(b);
        System.err.println(z);


        System.err.println("name= "+ name);
        System.err.println("CustID= "+ CustID);
        System.err.println("Order=[" + objOrder.OrderID + " " +
                objOrder.salesPerson + " " + objOrder.orderDate + "]");


        System.err.println(vecStrings.size());
        int i = 0;
        for(i=0;i < vecStrings.size(); i++ )
        {
            String s = (String)vecStrings.elementAt(i);
            System.err.println(s);
        }

        if (objOrder.vecLI != null) {
            Iterator it = objOrder.vecLI.iterator();
            while (it.hasNext()) {
                MyLineItem o = (MyLineItem) it.next();
                System.err.println(o.getMyLineItem());
            }
        }

        System.err.println("--------------------------------------------------");
    }

    public String XMLDump()
    {
        MyExchange("out");
        return ContainedXMLObj.toXML();
    }

    // indirect inheritance manages calls to FromXML() and ToXML()
    // manually, exposing them is optional.  Member<-->XML exchange is
    // also done manually, generally following calls to FromXML() or ToXML()
    // this affords the developer full control for atypical implementations.
    void MyExchange(String inOut)
    {
        ContainedXMLObj.Member(this, inOut, b,	"b","byte","", 1);
        ContainedXMLObj.Member(this, inOut, z,	"z","bool","", 1);
        ContainedXMLObj.Member(this, inOut, l,	"l","long","", 1);
        ContainedXMLObj.Member(this, inOut, d,	"d","double","", 1);
        ContainedXMLObj.Member(this, inOut, s,	"s","short","", 1);
        ContainedXMLObj.Member(this, inOut, name,	"name","FirstName","", 1);
        ContainedXMLObj.Member(this, inOut, CustID, "CustID", "CustomerID", "", 1);
        ContainedXMLObj.Member(this, inOut, objOrder,	"objOrder",	"Order","a777/root/GApp/MyOrder","");
        ContainedXMLObj.Member(this, inOut, vecStrings,"vecStrings","StringItem","String",  "StringList Level2Wrapper");
    }

    public Customer2( String strXML )
    {
        // create a new 'empty' object that maps to "Customer".
        ContainedXMLObj = new XMLObject("Customer", false );
        ContainedXMLObj.fromXML(strXML);
        MyExchange("in");
    }

    // FromXML is not inheriterd, so we can expose the functionality through a
    // controlled accessor.
    public void ApplyXML( String strXML )
    {
        ContainedXMLObj.fromXML(strXML);
        MyExchange("in");
    }
}
