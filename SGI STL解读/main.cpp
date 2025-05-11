#include<iostream>

using namespace std;

class __default_alloc_template
{
public:
	static void* allocate(size_t __n)
	{
		void* __ret = 0;

		// ###### 如果超过指定大小，就用一级的空间配置器，和stl的一样 ######
		if (__n > (size_t)_MAX_BYTES) {
			__ret = malloc_alloc::allocate(__n);
		}
		// ###### 否则，就用二级的空间配置器 ######
		else {
			_Obj* volatile* __my_free_list = _S_free_list + _S_freelist_index(__n);  // 数组起始位置+偏移量
#     ifndef _NOTHREADS
			_Lock __lock_instance;  // 互斥锁，保证线程安全
#     endif
			_Obj* __RESTRICT __result = *__my_free_list; // 出数组的值
			if (__result == 0)
				__ret = _S_refill(_S_round_up(__n));  // 在当前位置分配若干个16字节的chunk块，_ret指向链表的头
			else {
				*__my_free_list = __result->_M_free_list_link;  // _M_free_list_link相当于链表的指针域，即next
				__ret = __result;
				/*
					因为当前的chunk块（__result）将被分配出去，所以要把
					_my_free_list先指向下一个chunk块
				*/
			}
		}

		return __ret;
	}

private:

	// ###### 重要类型和变量定义 ###############################################

	// 内存池的粒度信息
	enum { _ALIGN = 8 };
	enum { _MAX_BYTES = 128 };
	enum { _NFREELISTS = 16 }; // _MAX_BYTES/_ALIGN

	// 每一个内存chunk块的头信息，类似链表节点
	union _Obj {    
		union _Obj* _M_free_list_link;
		char _M_client_data[1];    /* The client sees this */
	};

	// 组织所有自由链表的数组，数组的每一个元素的类型是_Obj*，全部初始化为0
	static _Obj* volatile _S_free_list[_NFREELISTS];

	// Chunk allocation state. 记录内存chunk块的分配情况，全部初始化为0
	static char* _S_start_free;
	static char* _S_end_free;
	static size_t _S_heap_size;


	// ###### 重要辅助函数 ####################################################

	// 将 __bytes 上调至最邻近的 8 的倍数
	static size_t _S_round_up(size_t __bytes)
	{
		return (((__bytes)+(size_t)_ALIGN - 1) & ~((size_t)_ALIGN - 1));
		/*
			size_t: 四字节，所以：
			 7 -> 00000000 00000000 00000000 00000111
			~7 -> 11111111 11111111 11111111 11111000
			6 + 7 = 13 -> 00000000 00000000 00000000 00001101
			13 & ~7 = 00000000 00000000 00000000 00001000 = 8
		*/
	}

	// 返回__bytes大小的chunk块位于 free-list 中的编号
	static  size_t _S_freelist_index(size_t __bytes)
	{
		return (((__bytes)+(size_t)_ALIGN - 1) / (size_t)_ALIGN - 1);
	}

};


int main()
{
	/*__default_alloc_template dat;
	for (int i = 0; i <= 25; i++)
	{
		cout << i << "->__S_round_up(" << i << ") = "
			<< dat._S_round_up((size_t)i) << endl;
	}*/

	return 0;
}