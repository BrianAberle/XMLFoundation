// put your code own code in this file and replace this example...
// This will be called automatically for GProfile entries registered with RegisterChangeNotification()
// 

if (strSection.CompareNoCase("CustomHTTP") == 0)
{
	if (strEntry.CompareNoCase("ExampleInteger") == 0)
	{
		g_MyExample.m_Int = GetProfile().GetInt("CustomHTTP","ExampleInteger",false);
		g_MyExample.foo(1); //<-----LOOK This 'trigger' calls foo()
	}
	else if (strEntry.CompareNoCase("ExampleString") == 0)
	{
		g_MyExampleGlobalString = GetProfile().GetString("CustomHTTP","ExampleString",false);
	}
}