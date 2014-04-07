// put your code own code in this file and replace this example...
//
// You CANNOT write functions or create global variables in this file.

printf("\nWorld, see. it has started.\n");
printf("try:   http://%s:%d/Page1.html\n",g_strThisIPAddress._str,GetProfile().GetIntOrDefault("HTTP","Port",80));
printf("then in the Shell type:   mc    CustomHTTP    ExampleString  NewTitle\n");
printf("then in the browser re-view Page1.html(press F5) and you will see you set the document title from the Shell.\n");
printf("try:   http://%s:%d/Page2.html\n",g_strThisIPAddress._str,GetProfile().GetIntOrDefault("HTTP","Port",80));
printf("try:   http://%s:%d/Page3.html\n",g_strThisIPAddress._str,GetProfile().GetIntOrDefault("HTTP","Port",80));
printf("try:   http://%s:%d/Page4.html\n",g_strThisIPAddress._str,GetProfile().GetIntOrDefault("HTTP","Port",80));
printf("then in the Shell type:  hits       You'll see a connection count.\n");
printf("Hold F5 while viewing Page1.html and type 'hits' in the Shell while you do it.\n\n");
