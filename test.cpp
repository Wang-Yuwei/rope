#include "Rope.h"

int main()
{
 	WCRope::Rope<char, Synchronization::NullMutex> test = "This is a string";
	WCRope::ReversableRope<char, Synchronization::NullMutex> r = test;
	
	test = test + " " + r.reverse();
	
	printf("%s\n", test.GetString().c_str());
	return 0;
}