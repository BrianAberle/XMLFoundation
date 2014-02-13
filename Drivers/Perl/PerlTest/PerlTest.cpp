// Includes from the Perl distribution CORE directory
// You must also link with Perl.lib, or libPerl.so from the same CORE directory
#include <EXTERN.h> 
#include <perl.h>


int main(int argc, char * argv[])
{
	PerlInterpreter *my_perl;
	my_perl = perl_alloc();
	perl_construct( my_perl );

	char pzObjectAndPath[512];
	sprintf(pzObjectAndPath,"%s%s",(const char *)"C:\\Users\\Brian\\Desktop\\XMLFoundation\\Examples\\Perl\\","PerlTest.pl");
//	sprintf(pzObjectAndPath,"%s%s",(const char *)"/home/ubt/Desktop/XMLFoundation/Examples/Perl/","PerlTest.pl");
	char *pzPerlFileArg[] = { "", pzObjectAndPath };
	
	// parse the Perl Script
 	perl_parse(my_perl, 0, 2, pzPerlFileArg, (char **)NULL);

	char *args[] = { NULL };
	perl_call_argv("showtime", G_DISCARD | G_NOARGS, args);

    perl_destruct(my_perl);
    perl_free(my_perl);

	
	
	return 0;
}
