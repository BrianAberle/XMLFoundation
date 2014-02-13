Sometimes firewalls go beyond keeping unauthorized people out of the network
sometimes they keep the authorized people out too.  This tool is used to safely 
extend authorization as allowed for within the HTTP specification published by W3C.

There are some software vendors who have extended their HTTP implementation beyond 
the W3C specification, and they have created a proprietary HTTP like protocol that 
only works with certain browsers.  This utility will not function in those environments.

Nobody can gain access to what they do not already have access to with this tool. 
It simply allows them to do what they already could do in a different way that allows 
them more flexibility.  Any user who had the ability to download a file to their machine, 
now has the ability to grant others permission to put files on their machine through the 
same HTTP mechanism that they themselves put them there.


This is how it works:

The machine behind the firewall (aka "The Server") polls an HTTP GET to a 
"switchboard server".  The Switchboard is normally a machine on the internet.  
The switchboard server could also be a domain server on your local network, if you 
want to allow VPN clients on PPP to act as named servers within a private network.  
In either case, the switchboard is a machine that both parties can always reach.  
The "Polling Machine" is conceptually "The Server", in a client/server environment.
In this environment all machines are Peers regardless of their role.

The polling machine issues an HTTP GET to ask for a named connection at the switchboard.  
The named connection is placed at the switchboard by "The Client". In this case, it 
is this FilePoster application.  If the client is waiting to connect when the server
checks in at the switchboard, the client is sent an HTTP 200 with instructions to
begin sending the data.  The client, then POSTS that data to the switchboard who
proxies it to the server.

Here's the tricky part:  The server polls in with a GET, and the client pushes up
a POST.  If you know HTTP - GETS's cannot receive a POST so the server rips off
the POST headers and converts them to an HTTP 200 reply to the GET - it does this without
touching the actual data stream.  Since the switchboard does not have to process
the POST prior to proxying it back to "The Server", it's very efficient, and produces
extremely little latency time once the connection is established.  Furthermore the 
switchboard does not use TCP kernel buffering so the data is actually sent directly
from the same memory that it was received in.

Now once this initial message proxy is complete, the client connection that POSTED it 
waits in the switchboard, for the server to POST back a response.  Then the Switchboard goes 
through the same process of ripping off the POST HTTP header and replacing it with a 200 OK 
and sending the response back to the client.  Lastly, the server is sent an HTTP 200 to
it's post to complete the normal HTTP request/response design.


So:  Each machine issued (2) HTTP commands.  The client sent a GET that was answered by 
the Switchboard server then the client issued a POST, that was answered (indirectly) by the server.
The server first issued a GET that was (indirectly) answered by the client, then in a 
response to that GET - the server issued a POST that was answered by the switchboard.

The overhead is minimal to accomplish this task.  The client, the server, and the switchboard
support HTTP keep alives.  There are no delays in the protocol, and it makes all network
writes as efficient as possible.  Despite using keep alives, it does not rely on them.
So if a connection breaks between HTTP commands, the transaction will still complete normally.


So that people everywhere can apply this technology for countless applications it supports
Custom message handlers that can be written in Java, C++, COM, CORBA, Perl and Python plus
any language that can create a DLL in Windows like VB and C#.



Here's how to see it work:


"The Server" runs 5Loaves with this configuration:  Normally the [MsgFrom-*] would repeat
for every client account on the server and they would all use the Default* settings under
[Messaging], but this example shows you how to overide the default, as there is nothing that
limits one server to a single switchboard.  remove the "No" before NoLocalNetworkAuthenticate
to enable HTTP Basic Authentication if necessary.  The Polling interval of 20 seconds should
be increased in most cases.  create the folder "c:\5LMessages\UBTsAccountForYou" or what ever 
you want to call it to put any file this client sends into that folder. LetSenderPlaceFile=No
keeps the client always within the single folder you have assigned to him. "AcceptFrom=" 
should be followed by a comma separated list of all the client names allowed to access this
server - each one of this names should have its own [MsgFrom-*] section



[Messaging]
Enable=yes
AcceptFrom=UBTsAccountForYou
DefaultSwitchBoardServer=192.168.1.3
DefaultSwitchBoardPort=81
UseBrowserProxy=no
BrowserProxy=MyCompanysProxyServer
BrowserProxyPort=8080
NoLocalNetworkAuthenticate=user:password


[MsgFrom-UBTsAccountForYou]
Enable=yes
CheckAtSwitchBoard=yes
Name=/PublicPath/UBTsAccountForYou
DiskLocation=c:\5LMessages\UBTsAccountForYou
LetSenderPlaceFile=No
SwitchBoardServer=192.168.1.3
SwitchBoardPort=81
PollIntervalSeconds=20


======================================
To use a custom message handler, add the following to each section

[TXML]
Drivers=D:\Drivers

[CStdCall]
Path=D:\MySharedLibs

[Messaging]
UsePluginHandler=yes
PluginHandler=CStdCall::PluginExample.dll::PluginExample::CustomMessagingHandler


You must have the C Language driver in D:\Drivers
You must have the custom plugin handler(PluginExample.dll) in D:\MySharedLibs
Obviously the paths are configurable.

The handler overrides DiskLocation= under [MsgFrom-*], 
it will not be written to disk unless your custom handler write it.

PluginExample.cpp has a complete implementation example called, CustomMessagingHandler

======================================





"The SwitchBoardServer" runs 5Loaves with this configuration.  In this case it runs on port 81.
Normally it would be on port 80, but there is nothing that restricts it to port 80 and sometimes
port 80 is already in use.  It's not just a switchboard, it's a fully functional HTTP server
with the switchboard feature enabled.


[SwitchBoardServer]
Enable=yes
Name=/PublicPath/

[HTTP]
Enable=yes
Port=81
ContentCache=0
UseKeepAlives=1
HTTPHeaderServerName=5Loaves
KeepAliveTimeOut=20
ShowIPAddressPageName=ShowIP
Home=d:\home


[Trace]
HTTPHeaderTrace=0
ThreadTrace=0
ConnectTrace=1



Now this application, FilePoster,("The Client") can run from anywhere, 
behind any sort of proxy or firewall also to send a file to "The Server"

