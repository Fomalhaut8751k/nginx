# SGI STL部分解读

## 1. SGI STL内存分配代码解读
### 一些重要的变量类型和方法：
```cpp
 union _Obj  // 类似链表节点
 {   
    union _Obj* _M_free_list_link;
    char _M_client_data[1];   
 }
```
```cpp
enum {_ALIGN = 8};  // chunk块大小按8字节增长，如8,16,24.....
enum {_MAX_BYTES = 128};  // 提供的最大的chunk块为128字节
enum {_NFREELISTS = 16};  // 自由链表的数量，每一个不同字节的chunk块都被开辟在一个自由链表中。
```
```cpp
// 组织所有自由链表的数组，数组的每一个元素的类型是_Obj*，全部初始化为0
static _Obj* volatile _S_free_list[_NFREELISTS];
```

```cpp
// __bytes 上调至最邻近的 8 的倍数
static size_t _S_round_up(size_t __bytes) 
{
    return (((__bytes) + (size_t) _ALIGN-1) & ~((size_t) _ALIGN - 1)); 
}

// 返回__bytes大小的chunk块位于 free-list 中的编号
static  size_t _S_freelist_index(size_t __bytes)
{
    return (((__bytes)+(size_t)_ALIGN - 1) / (size_t)_ALIGN - 1);
}
```
sqi stl内存管理的示意图大致如下，想要申请一个chunk供37字节的数据使用时，```_S_round_up(size_t __bytes)```得到的结果为40，通过```__my_free_list = _S_free_list + _S_freelist_index(__n);```定位到对应大小字节块所在的位置。

<img src='img/0.png'>

### (1) allocate()
```cpp
static void* allocate(size_t __n)
{
    void* __ret = 0;

    if (__n > (size_t) _MAX_BYTES) {
      __ret = malloc_alloc::allocate(__n);
    }
    else {
      _Obj* __STL_VOLATILE* __my_free_list
          = _S_free_list + _S_freelist_index(__n);
      // Acquire the lock here with a constructor call.
      // This ensures that it is released in exit or during stack
      // unwinding.
#     ifndef _NOTHREADS
      /*REFERENCED*/
      _Lock __lock_instance;
#     endif
      _Obj* __RESTRICT __result = *__my_free_list;
      if (__result == 0)  // 表示没有对应字节大小的chunk块
        __ret = _S_refill(_S_round_up(__n));
      else {
        *__my_free_list = __result -> _M_free_list_link;
        __ret = __result;
      }
    }

    return __ret;
}
```
如果想要申请的chunk块的大小超过了```_MAX_BYTES```即128，二级空间配置器最大只能分配128的大小，故直接使用一级的空间配置器。

有对应字节大小的chunk块的流程情况大致如下(取__n = 14)：
<img src='img/1.gif'>

### (2) __S_refill()
正如上面代码所示，如果没有对应字节大小的chunk块了，那么就会调用```__S_refill()```函数进行创建。
```cpp
template <bool __threads, int __inst>
void* __default_alloc_template<__threads, __inst>::_S_refill(size_t __n)
{
    int __nobjs = 20;
    char* __chunk = _S_chunk_alloc(__n, __nobjs);  // 负责内存开辟
    _Obj* __STL_VOLATILE* __my_free_list;  // 遍历_S_free_list，因为是_Obj*类型元素，所以是二级指针
    _Obj* __result;
    _Obj* __current_obj;
    _Obj* __next_obj;
    int __i;

    if (1 == __nobjs) return(__chunk);
    __my_free_list = _S_free_list + _S_freelist_index(__n);

    /* Build free list in chunk */
      __result = (_Obj*)__chunk;  // char* -> _Obj*
      *__my_free_list = __next_obj = (_Obj*)(__chunk + __n);
      for (__i = 1; ; __i++) {
        __current_obj = __next_obj;
        __next_obj = (_Obj*)((char*)__next_obj + __n);
        if (__nobjs - 1 == __i) {
            __current_obj -> _M_free_list_link = 0;
            break;
        } else {
            __current_obj -> _M_free_list_link = __next_obj;
        }
      }
    return(__result);
}
```
先简单说明一下```_S_chunk_alloc(__n, __nobjs)```，这个函数会开辟一块指定大小的内存，然后返回这个内存的起始位置。
```__S_refill()```的大致流程如下：
<img src='img/2.gif'>

### (3) _S_chunk_alloc()

```cpp
template <bool __threads, int __inst>
char*
__default_alloc_template<__threads, __inst>::_S_chunk_alloc(size_t __size, 
                                                            int& __nobjs)
{
    char* __result;
    size_t __total_bytes = __size * __nobjs;
    size_t __bytes_left = _S_end_free - _S_start_free;

    if (__bytes_left >= __total_bytes) {
        __result = _S_start_free;
        _S_start_free += __total_bytes;
        return(__result);
    } else if (__bytes_left >= __size) {
        __nobjs = (int)(__bytes_left/__size);
        __total_bytes = __size * __nobjs;
        __result = _S_start_free;
        _S_start_free += __total_bytes;
        return(__result);
    } else {
        size_t __bytes_to_get = 
	  2 * __total_bytes + _S_round_up(_S_heap_size >> 4);
        // Try to make use of the left-over piece.
        if (__bytes_left > 0) {
            _Obj* __STL_VOLATILE* __my_free_list =
                        _S_free_list + _S_freelist_index(__bytes_left);

            ((_Obj*)_S_start_free) -> _M_free_list_link = *__my_free_list;
            *__my_free_list = (_Obj*)_S_start_free;
        }
        _S_start_free = (char*)malloc(__bytes_to_get);
        if (0 == _S_start_free) {
            size_t __i;
            _Obj* __STL_VOLATILE* __my_free_list;
	    _Obj* __p;
            // Try to make do with what we have.  That can't
            // hurt.  We do not try smaller requests, since that tends
            // to result in disaster on multi-process machines.
            for (__i = __size;
                 __i <= (size_t) _MAX_BYTES;
                 __i += (size_t) _ALIGN) {
                __my_free_list = _S_free_list + _S_freelist_index(__i);
                __p = *__my_free_list;
                if (0 != __p) {
                    *__my_free_list = __p -> _M_free_list_link;
                    _S_start_free = (char*)__p;
                    _S_end_free = _S_start_free + __i;
                    return(_S_chunk_alloc(__size, __nobjs));
                    // Any leftover piece will eventually make it to the
                    // right free list.
                }
            }
	    _S_end_free = 0;	// In case of exception.
            _S_start_free = (char*)malloc_alloc::allocate(__bytes_to_get);
            // This should either throw an
            // exception or remedy the situation.  Thus we assume it
            // succeeded.
        }
        _S_heap_size += __bytes_to_get;
        _S_end_free = _S_start_free + __bytes_to_get;
        return(_S_chunk_alloc(__size, __nobjs));
    }
}
```