#include "Rope.h"

int main()
{
 	WCRope::Rope<char> test = "This is a string";
	WCRope::ReversableRope<char> r = test;
	
	test = test + " " + r.reverse();
	
	printf("%s\n", test.GetString().c_str());
	return 0;
}