//
//  ViewController.m
//  ObjectiveObjects
//
//  Created by User on 8/9/13.
//  Copyright (c) 2013 XML. All rights reserved.
//

#import "ViewController.h"
#define _IOS 1
#include "XMLFoundation.h"

@interface ViewController()

@end

@implementation ViewController

@synthesize button1 = _button1;
@synthesize button2 = _button2;
@synthesize button3 = _button3;
@synthesize textView = _textView;



//////////////////////////////////////////////////////////////////
// Define a simple object
//////////////////////////////////////////////////////////////////
class MyCustomObject : public XMLObject
{
public:							// make public here for example simplicity - this is not required
	GString m_strString;		// A String Member
	GString m_strColor;			// An attribute , not an element
	int m_nInteger;				// An Integer Member
	char m_szNative[10];		// a fixed 10 byte buffer
	GStringList m_strList;      // A String List
    
	virtual void MapXMLTagsToMembers()
	{
        //			Member variable		XML Element
		MapMember( &m_strList,		"StringList", "Wrapper");
		MapMember(	&m_nInteger,	"Number");
		MapMember(	&m_strString,	"String");
		MapMember( m_szNative,		"FixedBuffer", sizeof(m_szNative) );
		MapAttribute(&m_strColor,	"Color");
	}
	
	// 'this' type, followed by the XML Element name, normally DECLARE_FACTORY() is in an .h file
	DECLARE_FACTORY(MyCustomObject, Thing)
    
	MyCustomObject(){} // keep one constructor with no arguments
	~MyCustomObject(){};
};
// IMPLEMENT_FACTORY() must exist in a .CPP file - not an .h file - one for every DECLARE_FACTORY()
IMPLEMENT_FACTORY(MyCustomObject, Thing)




//
// This is the XML we'll process.
//
char pzXML[] =
"<Thing Color='Red'>"
"<String>Owners Word</String>"
"<Number>777</Number>"
"<FixedBuffer>native</FixedBuffer>"
"<Wrapper>"
"<StringList>one</StringList>"
"<StringList>two</StringList>"
"</Wrapper>"
"</Thing>";

//
// This sets up logging - NSString, GString, or char *
//
UITextView *gLogTextView;
static void XlogInfo(NSString *logStr)
{
    
    // Scroll or not
    BOOL scroll = NO;
    if (gLogTextView.contentSize.height-gLogTextView.contentOffset.y<gLogTextView.frame.size.height*2) {
        scroll = YES;
        NSOperationQueue *queue = [NSOperationQueue mainQueue];
        [queue cancelAllOperations];
        [queue addOperationWithBlock:^{

            // Scroll as for showing the last sentence
            int lng = [gLogTextView.text length];
            NSRange range;
            range.location = lng;
            range.length = 0;
            [gLogTextView scrollRangeToVisible:range];
            
        }];
    }
    
    // Insert log string to text view
    gLogTextView.text = [gLogTextView.text stringByAppendingString:logStr];
    // and add a newline
    gLogTextView.text = [gLogTextView.text stringByAppendingString:@"\n"];
    
}

static void XlogInfo(const char *logStr)
{
    NSString *str = [[NSString alloc] initWithBytes:logStr length:strlen(logStr) encoding:NSASCIIStringEncoding];
    XlogInfo(str);
}

void iOSInfoLog(int nMsgID, GString &strSrc)
{
    NSString *str = [[NSString alloc] initWithBytes:strSrc.Buf() length:strSrc.Length() encoding:NSASCIIStringEncoding];
    XlogInfo(str);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    gLogTextView = _textView;
}

- (void)viewDidUnload {
    [super viewDidUnload];
}


// The code below is a cut and paste from the C++ StartHere0 example except that the printf's have been 
// changed to XlogInfo's - All of the C++ examples in the XMLFoundation work in Objective C++
int StartHere0()
{
	MyCustomObject O;
	O.FromXMLX(pzXML);
    
	// look at the Object "O".
	GString strDebug;
	strDebug << "Yo! Check out O:" << O.m_strString << "[" << O.m_nInteger << "]:" << O.m_szNative << "\n\n\n";
	
    XlogInfo(strDebug);
    
	// set some data
	O.m_strString = "Root was here";
	
	// add any encoding tags or doctype you need - if you need them other wise skip the next two lines
	GString strXMLStreamDestinationBuffer = "<?xml version=\"1.0\" standAlone='yes'?>\n";
	strXMLStreamDestinationBuffer << "<!DOCTYPE totallyCustom SYSTEM \"http://www.IBM.com/example.dtd\">";
    
	O.ToXML( &strXMLStreamDestinationBuffer);
    
	 XlogInfo(strXMLStreamDestinationBuffer);
    
    /*
     <?xml version="1.0" standAlone='yes'?>
     <!DOCTYPE totallyCustom SYSTEM "http://www.IBM.com/example.dtd">
     <Thing Color="Red">
     <Wrapper>
     <StringList>one</StringList>
     <StringList>two</StringList>
     </Wrapper>
     <Number>777</Number>
     <String>Root was here</String>
     <FixedBuffer>native</FixedBuffer>
     </Thing>
     */	
    
    
    
	return 0;
}

///////////////////////////////////////////
// The code above is the C++ StartHere0 Example.
// All  of the C++ examples apply to Objective C++.
// The code below is an example that uses 5Loaves (aka ServerCore.cpp) to make an HTTP  server.
//////////////////////////////////////////

// you will need to fix this path to point to wherever you unzipped XMLFoundation to
#include "/Users/user/Desktop/XMLFoundation/Servers/Core/ServerCore.cpp"


const char *pzBoundStartupConfig =
"[System]\r\n"
"Pool=5\r\n"
"ProxyPool=0\r\n"
"\r\n"
"[HTTP]\r\n"
"Enable=yes\r\n"
"Index=index.html\r\n"
"Home=%s\r\n"
"Port=%s\r\n";

int g_isRunning = 0;

void StartHTTPServer(NSString *strHome, NSString *strPort)
{
	if (!g_isRunning)
	{
		g_isRunning = 1;
        
		SetServerCoreInfoLog( iOSInfoLog );
        
        const char  *pzHome = [strHome UTF8String];
        const char  *pzPort = [strPort UTF8String];
     
		GString strCfgData;
		strCfgData.Format(pzBoundStartupConfig,pzHome,pzPort);
        
        GProfile *pGP = new GProfile((const char *)strCfgData, (int)strCfgData.Length());
		SetProfile(pGP);
        
		server_start("-- iOS Server --");
        
	}
	else
	{
		GString G("Server is already running");
        iOSInfoLog(777, G);
	}

}

- (IBAction)test1:(id)sender {
    // create a file for the HTTP server to publish
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *filePath = [documentsDirectory stringByAppendingPathComponent:@"index.html"];
    
    NSString *str = @"<html><head><title>Hello</title></head><body><p>Hello World</p></body></html>";
    
    [str writeToFile:filePath atomically:TRUE encoding:NSUTF8StringEncoding error:NULL];
 
    
 
    
    // start HTTP Server on port 8080 (80 is in use by default)
    // To view the web pagefrom your browser type :
    // http://127.0.0.1:8080   (on the phone)
    // http://192.168.1.128:8080 (on the machine host - the server will determine your actual ip and log it to the phone's display )
    StartHTTPServer(documentsDirectory,@"8080");
}


- (IBAction)test2:(id)sender {
    
    StartHere0();

}

- (IBAction)test3:(id)sender {
    
    // Add your own test here
    
}

@end

