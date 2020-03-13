
#include <iostream>

extern int a;

int main(int argc, const char* argv[]) {

	std::cout << "Hello world" << std::endl;

	if(a == 2) 
		a = 1;	
	else
		a = 3;

	return 0;
}
