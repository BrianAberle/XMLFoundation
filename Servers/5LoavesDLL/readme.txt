This DLL is the entire 5Loaves engine in a DLL that you 
can embed into your own custom application in VB, Delphi
or any language capable of making a standard DLL call.

Your application becomes enabled with every capability of
5Loaves leaving you the ability to enable only the features
you want (and the source to the .DLL to add new ones).  

Developers beware, DLL's are failure points.  Somebody may 
install a newer or older version or an intentionally modified
version that compromises your application security right
over the DLL you have as the platform to your application.
Customizing the DLL in a very small way can prevent all of this.

Since the .DLL is so small, you can rename it to match your
application, and then know for sure it won't get accidentally 
overlaid by anything new (or old) that comes along.

In the early days of NT, some people got source code cuts
so they could make tailored custom software, UNIX platforms
commonly ship with source, and on some projects you need to
get in there and tailor a piece to your own need.

