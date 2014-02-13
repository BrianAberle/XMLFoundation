#!/usr/local/bin/perl 

sub ExposedMethods
{
##############################################################
# Format: Method[ArgName&DataType]!
# where [ArgName&&DataType] repeats for each argument
#
# This example exposes the first two routines
# showtime 
# - takes no arguments
# LeftString 
# - takes a String called "Input" and an Int called "NewLen"
##############################################################

"
showtime!
LeftString&Input&String&NewLen&Integer!
"
}

sub showtime 
{
time;
}

sub LeftString
{
my($s, $n) = @_ ;
substr($s, 0, $n);
}

sub PrivateRoutines 
{
# any routine not exposed in ExposedMethods 
# cannot be accessed directly
}
