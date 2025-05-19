#include<iostream>
#include"MemoryPool.h"

using namespace std;

int main()
{
	cout << "pdcHelloWorld" << endl;

	MemoryPool mp;
	int* p = (int*)mp.allocate(sizeof(int));
	new(p) int(4);
	cout << *p << endl;

	return 0;
}