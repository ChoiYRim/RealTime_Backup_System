#include <iostream>

int main(int argc,char* argv[])
{
	int arr[16] = {0,1,2,3,4,5,6,};
	int* ptr = arr;
	int* dynamic = new int[16];
	char s1[] = "hello";
	char* s2 = s1;

	std::cout << "the size of arr : " << sizeof(arr) << '\n';
	std::cout << "the size of pointer which points to arr : " << sizeof(ptr) << '\n';
	std::cout << "the size of dynamic allocated array : " << sizeof(dynamic) << '\n';
	std::cout << "s1 : " << s1 << '\n';
	std::cout << "s2 : " << s2 << '\n';

	std::cout << "&s1 : " << &s1 << '\n';
	std::cout << "&s2 : " << &s2 << '\n';

	delete[] dynamic;
	return 0;
}
