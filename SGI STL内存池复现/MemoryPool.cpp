#include<iostream>
#include"MemoryPool.h"

using namespace std;

MemoryPool::_Obj* MemoryPool::_S_free_list[_NFREELISTS];

char* MemoryPool::_S_start_free;
char* MemoryPool::_S_end_free;
size_t MemoryPool::_S_heap_size;


size_t MemoryPool::_S_round_up(size_t __bytes)
{
	return (((__bytes)+(size_t)_ALIGN - 1) & ~((size_t)_ALIGN - 1));
}


size_t MemoryPool::_S_freelist_index(size_t __bytes)
{
	return (((__bytes)+(size_t)_ALIGN - 1) / (size_t)_ALIGN - 1);
}

void* MemoryPool::allocate(size_t __n)
{
	void* _ret;  // 返回的内存地址

	/* 如果想要申请的内存块太大，大于内存池中
		的内存块大小上限_MAX_BYTES */
	if (__n > size_t(_MAX_BYTES))
	{
		/* 则使用一般的内存申请方式 */
		_ret = (void*)malloc(__n);  
		return _ret;
	}

	/* 遍历内存池，到可以容下__n大小的最小内存块的位置：:6->8->索引0 */
	 _Obj** __my_free_list = _S_free_list + _S_freelist_index(__n);

	 /* 如果当前chunk块没有剩余可用的 */
	 if (*__my_free_list == 0)
	 {
		 /* 调用函数__S_refill() */
		 _ret = __S_refill(_S_round_up(__n));
	 }
	 /* 有剩余的，取出第一个 */
	 else
	 {
		 _Obj* __result = (*__my_free_list)->_M_free_list_link;
		 *__my_free_list = __result->_M_free_list_link;
		 _ret = __result;
	 }
	 return _ret;
}

void* MemoryPool::__S_refill(size_t __n)
{
	int __nobjs = 20;  // 创建的chunk块的个数
	_Obj* __result;
	_Obj* _current_obj;
	_Obj* _next_obj;
	int _i;

	/* 开辟空间 */
	char* __chunk = _S_chunk_alloc(__n, __nobjs);

	/* 定位到对应字节大小的位置 */
	_Obj** __my_free_list = _S_free_list + _S_freelist_index(__n);

	__result = (_Obj*)__chunk;  // 开辟的空间的第一个位置

	/* 如果空间只够一个的大小，就不链接直接返回 */
	if (__nobjs == 1)
	{
		return __result;
	}

	*__my_free_list = _next_obj = (_Obj*)(__chunk + __n);  // 第二个位置

	for (_i = 1; _i < __nobjs - 1; ++_i)
	{
		_current_obj = _next_obj;
		_next_obj = (_Obj*)((char*)_current_obj + __n);
		_current_obj->_M_free_list_link = _next_obj;
	}

	_current_obj = _next_obj;
	_current_obj->_M_free_list_link = 0;

	return __result;
}

char* MemoryPool::_S_chunk_alloc(size_t __size, int& __nobjs)
{
	char* __result;
	size_t _total_bytes;
	size_t _bytes_left = _S_end_free - _S_start_free;
	/* 定位 */
	_Obj** _my_free_list = _S_free_list + _S_freelist_index(__size);

	_total_bytes = __nobjs * __size;

	/* 没有可用的备用空间了 */
	if (_bytes_left == 0)
	{
		size_t __bytes_to_get = 2 * _total_bytes;
		_S_start_free = (char*)malloc(__bytes_to_get);
		_S_end_free = _S_start_free + __bytes_to_get;
	}
	else
	{
		if (_bytes_left > _total_bytes)
		{
			__result = _S_start_free;
			_S_start_free += _total_bytes;
			return __result;
		}
		else if (_bytes_left > __size)
		{
			__result = _S_start_free;
			__nobjs = int(_bytes_left / __size);
			_total_bytes = __nobjs * __size;
			_S_start_free += _total_bytes;
			return __result;
		}
		else
		{
			/* 如果当前的备用空间已经小于一个当前字节数 */
			_my_free_list = _S_free_list + _S_freelist_index(_bytes_left);
			/* 把这个接到该位置的chunk静态链表上 */
			((_Obj*)_S_start_free)->_M_free_list_link = (*_my_free_list);
			(*_my_free_list) = (_Obj*)_S_start_free;

			/* 还是要继续开辟 */
			size_t __bytes_to_get = 2 * _total_bytes;
			_S_start_free = (char*)malloc(__bytes_to_get);
			_S_end_free = _S_start_free + __bytes_to_get;
		}
	}

	return MemoryPool::_S_chunk_alloc(__size, __nobjs);
	
}