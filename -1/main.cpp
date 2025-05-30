#include<iostream>

using namespace std;

// ###### union #####################################################################
#if 0
/*
	union 是一种特殊的数据结构，它允许在相同的内存位置存储不同的数据类型。
	union 可以帮助节省内存，因为它们使用相同的内存空间来存储不同的数据类型，
	但是只能同时存储其中一个
*/
union _Obj  // 取占用字节最大的类型
{
	_Obj* next;  // 4字节
	char a;  // 1字节
	int b;  // 4字节
};

int main()
{
	_Obj o1;

	cout << "_Obj.size: " << sizeof(_Obj) << endl;
	o1.b = 2;
	cout << "_Next = " <<  o1.next << "\t_intVal = " << o1.b << endl;
	/*
	* 一个字节八个比特，一个16进制相当于4位，故八个就是32位，4字节
		把o1.b的值置为2，即00000002，这个变量的四字节空间就变为00000002
		访问o1.next，它也是4个字节，所以显示00000002
	*/
	cout << "charVal = " << o1.a << endl;  // 打印不出来

	return 0;
}
#endif
// ###### 内存偏移 #####################################################################
#if 0
#include<memory>
union _Obj 
{
	_Obj* next;  // 4字节
	char a;  // 1字节
};
int main()
{
	_Obj arr[2];
	_Obj o1;
	_Obj o2;
	
	o1.a = 'b'; 
	o2.a = 'd';

	arr[0] = o1; arr[1] = o2;

	*(char*)arr = 'c';

	*((char*)arr + 4) = 'a';

	cout << arr[0].a << endl;
	cout << arr[1].a << endl;

	return 0;
}
#endif
// #### 内存开辟 #####################################################################
#if 0
struct _Obj
{
	_Obj()
	{
		cout << "pdcHelloWorld" << endl;
	}

	_Obj* _M_free_list_link;
	int _M_client_data;
};

int main()
{
	int __nobjs = 20;
	int __n = 8;
	_Obj* __current_chunk;
	char* __chunk = (char*)malloc(__nobjs * sizeof(_Obj));

	cout << "malloc开辟空间" << endl;

	_Obj* __next_obj = __current_chunk =(_Obj*)(__chunk);
	for (int i = 0; i < 20; i++)
	{
		__next_obj->_M_client_data = i;
		__next_obj = (_Obj*)((char*)__next_obj + __n);
	}

	for (int i = 0; i < 20; i++)
	{
		cout << __current_chunk->_M_client_data << endl;
		__current_chunk++;
	}

	free(__chunk);

	/*
		整个过程并没有调用任何构造函数，说明不涉及对象的构造
		只有内存的开辟。但本是_Obj是一个union类型的....
	*/

	return 0;
}
#endif
// ###### 系统api的内存对齐 ######################################
#if 0
int main() {

	cout << "posix_memalign只支持linux" << endl;
}
#endif
// ###### 内存初始化 #############################################
#if 0
#include<bitset>

void* ngx_pcalloc(size_t size)
{
	void* p = malloc(size);
	cout << p << endl;
	unsigned char* _p = (unsigned char*)(p);
	cout << &_p << endl;
	cout << _p + size << endl;
	for (; _p < _p + size; _p++)
	{
		*_p = 0;
		std::bitset<32> binary(*((int*)p));
		cout << binary << endl;
	}
	return (void*)p;
}

int main()
{
	void* p = ngx_pcalloc(3);
	std::bitset<32> binary(*((int*)p));
	cout << binary << endl;
	/*int a = 10;
	int* p = &a;
	cout << p << endl;
	cout << p + size_t(4)/sizeof(int) << endl;*/

	return 0;
}
#endif
// ###### 内存池初始化2 ####################################
#if 1

struct ngx_pool_data_t
{
	int* last;
	int a;
	long long b;
};

int main()
{
	
	ngx_pool_data_t* p = (ngx_pool_data_t*)malloc(sizeof(21));
	p->last = (int*)((char*)p + sizeof(ngx_pool_data_t));
	cout << p << endl;
	cout << sizeof(ngx_pool_data_t) << endl;
	cout << p->last << endl;

	return 0;
}
#endif