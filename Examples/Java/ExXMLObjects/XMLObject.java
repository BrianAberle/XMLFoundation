import java.util.Vector;
import java.util.Stack;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.TreeSet;
import java.util.HashSet;

public class XMLObject {
	static { 
					System.loadLibrary("JavaXMLFoundation");    
			}
	private static int InstanceId = 0;

	private native void JavaMap(int oH, int DataType,String strName,String xmlTag,String strWrapper, String ObjType, String strContainerType, int nSource);
	private native void JavaExchange(Object o, int oH, String inout,int nType,String b,String c,String d,String strObjectType, String strContainerType, int nSource);
	private native int  JavaConstruct(int n, String strXMLTag, int bAutoDataSync);
	private native void JavaDestruct(int oH);
	private native void JavaMapCacheDisable(int oH);
	private native void JavaMapOID(int oH, Object o, String Key1, String Key2, String Key3, String Key4, String Key5);
	private native XMLObject JavaGetSubObj(int oH);
	private native void JavaFromXML(int oH, String strXML);
	private native String JavaToXML(int oH);
	private native void JavaRemoveAll(int oH);

	private int		oH; // XML-Object Handle


	public void MemberRemoveAll()
	{
		JavaRemoveAll(oH);
	}

	protected void finalize()
	{
		JavaDestruct(oH);
	}
	protected void freedom()
	{
		JavaDestruct(oH);
	}

	public XMLObject(String strXMLTag) 
	{
		oH = ++InstanceId;
		oH = JavaConstruct(oH, strXMLTag, 1);
	}
	public XMLObject(String strXMLTag, boolean bAutoDataSync)
	{
		oH = ++InstanceId;
		if (bAutoDataSync)
			oH = JavaConstruct(oH, strXMLTag, 1);
		else
			oH = JavaConstruct(oH, strXMLTag, 0);
	}

	// Override this 'virtual' method to map member to XML tags
	// through calls to MapMember();
	private void MapXMLTagsToMembers()
	{
	}
	void DontCacheMemberMaps()
	{
		JavaMapCacheDisable(oH);
	}
	void fromXML(String strXML)
	{
		JavaFromXML(oH, strXML);
	}
	String toXML()
	{
		return JavaToXML(oH);
	}


	private String MakeObjectName(String strIn)
	{
		if (strIn.compareToIgnoreCase("String") == 0)
		{
			strIn = "Ljava/lang/String;";
		}
		else
		{
			String sTemp = "L" + strIn + ";";
			strIn = sTemp;
		}
		return strIn;
	}


	//
	//	MapAttrib for each native data type (1st without the optional 'nested' argument)
	//	
	public void MapAttrib(long z, String pzName, String pzXMLTag)
	{
		JavaMap(oH,0,pzName,pzXMLTag,"","", "", 2);
	}
	public void MapAttrib(double z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,1,pzName,pzXMLTag,"","", "", 2);
	}
	public void MapAttrib(short z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,2,pzName,pzXMLTag,"","", "", 2);
	}
	public void MapAttrib(byte z, String pzName, String pzXMLTag)
	{
		JavaMap(oH,3,pzName,pzXMLTag,"","", "", 2);
	}
	public void MapAttrib(String z, String pzName, String pzXMLTag)
	{
		JavaMap(oH,4,pzName,pzXMLTag,"","", "", 2);
	}
	public void MapAttrib(int z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,5,pzName,pzXMLTag,"","", "", 2);
	}
	public void MapAttrib(boolean z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,6,pzName,pzXMLTag,"","", "", 2);
	}

	//
	//	MapAttrib for each native data type (now with the optional 'nested' argument)
	//	
	public void MapAttrib(long z, String pzName, String pzXMLTag, String pzNestedInTag)
	{
		JavaMap(oH,0,pzName,pzXMLTag,pzNestedInTag,"", "", 2);
	}
	public void MapAttrib(double z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,1,pzName,pzXMLTag,pzNestedInTag,"", "", 2);
	}
	public void MapAttrib(short z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,2,pzName,pzXMLTag,pzNestedInTag,"", "", 2);
	}
	public void MapAttrib(byte z, String pzName, String pzXMLTag, String pzNestedInTag)
	{
		JavaMap(oH,3,pzName,pzXMLTag,pzNestedInTag,"", "", 2);
	}
	public void MapAttrib(String z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,4,pzName,pzXMLTag,pzNestedInTag,"", "", 2);
	}
	public void MapAttrib(int z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,5,pzName,pzXMLTag,pzNestedInTag,"", "", 2);
	}
	public void MapAttrib(boolean z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,6,pzName,pzXMLTag,pzNestedInTag,"", "", 2);
	}




	
	//
	//	MapMember for each native data type (1st without optional nested-in-tag)
	//	
	public void MapMember(long z, String pzName, String pzXMLTag)
	{
		JavaMap(oH,0,pzName,pzXMLTag,"","", "", 1);
	}
	public void MapMember(double z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,1,pzName,pzXMLTag,"","", "", 1);
	}
	public void MapMember(short z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,2,pzName,pzXMLTag,"","", "", 1);
	}
	public void MapMember(byte z, String pzName, String pzXMLTag)
	{
		JavaMap(oH,3,pzName,pzXMLTag,"","", "", 1);
	}
	public void MapMember(String z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,4,pzName,pzXMLTag,"","", "", 1);
	}
	public void MapMember(int z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,5,pzName,pzXMLTag,"","", "", 1);
	}
	public void MapMember(boolean z, String pzName, String pzXMLTag )
	{
		JavaMap(oH,6,pzName,pzXMLTag,"","", "", 1);
	}
	public void MapMember(Object ob, String pzName, String pzXMLTag, String strObjectType )
	{
		JavaMap(oH,7,pzName,pzXMLTag,"",MakeObjectName(strObjectType), "", 1);
	}
	public void MapMember(Vector a, String pzName, String pzXMLTag, String strObjectType )
	{
		JavaMap(oH,8,pzName,pzXMLTag,"",MakeObjectName(strObjectType),"Ljava/util/Vector;", 1);
	}
	public void MapMember(Stack a, String pzName, String pzXMLTag, String strObjectType )
	{
		JavaMap(oH,8,pzName,pzXMLTag,"",MakeObjectName(strObjectType),"Ljava/util/Stack;", 1);
	}
	public void MapMember(ArrayList a, String pzName, String pzXMLTag, String strObjectType )
	{
		JavaMap(oH,8,pzName,pzXMLTag,"",MakeObjectName(strObjectType),"Ljava/util/ArrayList;", 1);
	}
	public void MapMember(LinkedList a, String pzName, String pzXMLTag, String strObjectType )
	{
		JavaMap(oH,8,pzName,pzXMLTag,"",MakeObjectName(strObjectType),"Ljava/util/LinkedList;", 1);
	}
	public void MapMember(TreeSet a, String pzName, String pzXMLTag, String strObjectType )
	{
		JavaMap(oH,8,pzName,pzXMLTag,"",MakeObjectName(strObjectType),"Ljava/util/TreeSet;", 1);
	}
	public void MapMember(HashSet a, String pzName, String pzXMLTag, String strObjectType )
	{
		JavaMap(oH,8,pzName,pzXMLTag,"",MakeObjectName(strObjectType),"Ljava/util/HashSet;", 1);
	}





	//
	//	MapMember for each native data type (now with optional nested-in-tag)
	//	
	public void MapMember(long z, String pzName, String pzXMLTag, String pzNestedInTag)
	{
		JavaMap(oH,0,pzName,pzXMLTag,pzNestedInTag,"", "", 1);
	}
	public void MapMember(double z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,1,pzName,pzXMLTag,pzNestedInTag,"", "", 1);
	}
	public void MapMember(short z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,2,pzName,pzXMLTag,pzNestedInTag,"", "", 1);
	}
	public void MapMember(byte z, String pzName, String pzXMLTag, String pzNestedInTag)
	{
		JavaMap(oH,3,pzName,pzXMLTag,pzNestedInTag,"", "", 1);
	}
	public void MapMember(String z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,4,pzName,pzXMLTag,pzNestedInTag,"", "", 1);
	}
	public void MapMember(int z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,5,pzName,pzXMLTag,pzNestedInTag,"", "", 1);
	}
	public void MapMember(boolean z, String pzName, String pzXMLTag, String pzNestedInTag )
	{
		JavaMap(oH,6,pzName,pzXMLTag,pzNestedInTag,"", "", 1);
	}
	public void MapMember(Object ob, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaMap(oH,7,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType), "", 1);
	}
	public void MapMember(Vector a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaMap(oH,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/Vector;", 1);
	}
	public void MapMember(Stack a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaMap(oH,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/Stack;", 1);
	}
	public void MapMember(ArrayList a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaMap(oH,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/ArrayList;", 1);
	}
	public void MapMember(LinkedList a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaMap(oH,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/LinkedList;", 1);
	}
	public void MapMember(TreeSet a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaMap(oH,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/TreeSet;", 1);
	}
	public void MapMember(HashSet a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaMap(oH,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/HashSet;", 1);
	}





	public void MapObjectId(Object o, String Key1)
	{
		JavaMapOID(oH, o, Key1, "", "", "", "");
	}
	public void MapObjectId(Object o, String Key1, String Key2)
	{
		JavaMapOID(oH, o, Key1, Key2, "", "", "");
	}
	public void MapObjectId(Object o, String Key1, String Key2, String Key3)
	{
		JavaMapOID(oH, o, Key1, Key2, Key3, "", "");
	}
	public void MapObjectId(Object o, String Key1, String Key2, String Key3, String Key4)
	{
		JavaMapOID(oH, o, Key1, Key2, Key3, Key4, "");
	}
	public void MapObjectId(Object o, String Key1, String Key2, String Key3, String Key4, String Key5)
	{
		JavaMapOID(oH, o, Key1, Key2, Key3, Key4, Key5);
	}


	//
	//	Get/Set Member for each native data type
	//	
	public void Member(Object o, String xfer, long z, String pzName, String pzXMLTag, String pzNestedInTag, int nSource )
	{
		JavaExchange(o,oH,xfer,0,pzName,pzXMLTag,pzNestedInTag,"","", nSource);
	}
	public void Member(Object o, String xfer, double z, String pzName, String pzXMLTag, String pzNestedInTag, int nSource )
	{
		JavaExchange(o,oH,xfer,1,pzName,pzXMLTag,pzNestedInTag,"","",nSource);
	}
	public void Member(Object o, String xfer, short z, String pzName, String pzXMLTag, String pzNestedInTag, int nSource)
	{
		JavaExchange(o,oH,xfer,2,pzName,pzXMLTag,pzNestedInTag,"","",nSource);
	}
	public void Member(Object o, String xfer, byte z, String pzName, String pzXMLTag, String pzNestedInTag, int nSource )
	{
		JavaExchange(o,oH,xfer,3,pzName,pzXMLTag,pzNestedInTag,"","",nSource);
	}
	public void Member(Object o, String xfer, String z, String pzName, String pzXMLTag, String pzNestedInTag , int nSource)
	{
		JavaExchange(o,oH,xfer,4,pzName,pzXMLTag,pzNestedInTag,"","",nSource);
	}
	public void Member(Object o, String xfer, int z, String pzName, String pzXMLTag, String pzNestedInTag, int nSource )
	{
		JavaExchange(o,oH,xfer,5,pzName,pzXMLTag,pzNestedInTag,"","",nSource);
	}
	public void Member(Object o, String xfer, boolean z, String pzName, String pzXMLTag, String pzNestedInTag, int nSource )
	{
		JavaExchange(o,oH,xfer,6,pzName,pzXMLTag,pzNestedInTag,"","",nSource);
	}
	public void Member(Object o, String xfer, Object o2, String pzName, String pzXMLTag, String pzObjectType, String pzNestedInTag )
	{
		JavaExchange(o,oH,xfer,7,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(pzObjectType),"",1);
	}
	public void Member(Object o, String xfer, Vector a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaExchange(o,oH,xfer,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/Vector;",1);
	}
	public void Member(Object o, String xfer, Stack a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag)
	{
		JavaExchange(o,oH,xfer,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/Stack;",1);
	}
	public void Member(Object o, String xfer, ArrayList a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaExchange(o,oH,xfer,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/ArrayList;",1);
	}
	public void Member(Object o, String xfer, LinkedList a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaExchange(o,oH,xfer,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/LinkedList;",1);
	}
	public void Member(Object o, String xfer, TreeSet a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaExchange(o,oH,xfer,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/TreeSet;",1);
	}
	public void Member(Object o, String xfer, HashSet a, String pzName, String pzXMLTag, String strObjectType, String pzNestedInTag )
	{
		JavaExchange(o,oH,xfer,8,pzName,pzXMLTag,pzNestedInTag,MakeObjectName(strObjectType),"Ljava/util/HashSet;",1);
	}
}
