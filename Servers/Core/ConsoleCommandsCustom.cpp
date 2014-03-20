// put your code own code in this file and replace this example...

void ExecuteCommand(const char *pzCmd)
{
	GString strCmd(pzCmd);
	if ( strCmd.CompareNoCase("777") == 0 )
	{
		
		printf("Enter any number\n");
		printf("Or press enter to exit this command.\n");
		GString strArgNumber( GetUserCommand() );
		if (strArgNumber.Length())
		{
			printf("Enter a second number - or press enter to exit\n");
			GString strArgNumber2 ( GetUserCommand() );
			if (strArgNumber2.Length())
			{
				GString msg;
				msg << strArgNumber << " + " << strArgNumber2 << " = " << atoi(strArgNumber) + atoi(strArgNumber2);
			}
		}
	}
	else if ( strCmd.CompareNoCase("GGG") == 0 )
	{
		// do your own thing for each command you add
	}
}
