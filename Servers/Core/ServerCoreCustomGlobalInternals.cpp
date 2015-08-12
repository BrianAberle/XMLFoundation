// put your code own code in this file and replace this example...

// this file is at the global scope after the declaration of ServerCore.cpp's
// thread state internal structures - so if you need to access them - you can.
//
// showActiveThreads(), KillTid(), Tracing and more is available from this hook
//
// You can #include<xxxxx.h> any header files you need

// put your code own code in this file and replace this example...


GString g_MyExampleGlobalString;
void showActiveThreads(GString *pG/* = 0*/);

class CMyGlobalClassExample
{
public:
	int m_Int;
	void foo(int i)
	{
		if (i == 1)
		{
			showActiveThreads(0);
		}
	}
	CMyGlobalClassExample()
	{
		m_Int=777;
	}
} g_MyExample;