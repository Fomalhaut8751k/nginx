#pragma once
#include<iostream>

using namespace std;

#define __PRIVATE private 
#define __PUBLIC public

class MemoryPool
{
__PRIVATE:
	union _Obj
	{
		union _Obj* _M_free_list_link;
		char _M_client_data[1];
	};

	enum { _ALIGN = 8 };
	enum { _MAX_BYTES = 128 };
	enum { _NFREELISTS = 16 };

	static _Obj* _S_free_list[_NFREELISTS];

	static char* _S_start_free;
	static char* _S_end_free;
	static size_t _S_heap_size;

__PUBLIC:
	// 将 __bytes 上调至最邻近的 8 的倍数
	static size_t _S_round_up(size_t __bytes);

	// 返回__bytes大小的chunk块位于 free-list 中的编号
	static size_t _S_freelist_index(size_t __bytes);


__PUBLIC:

	/* 分配 */
	static void* allocate(size_t __n);

	/* 链接？ */
	static void* __S_refill(size_t __n);

	/* 开辟 */
	static char* _S_chunk_alloc(size_t __size, int& __nobjs);
};


