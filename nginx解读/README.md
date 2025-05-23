# nginx内存池解读

## 1. 一些重要的变量和方法

```cpp
struct ngx_pool_s {  // 内存池头信息
    ngx_pool_data_t       d;  // 数据信息
    size_t                max;  // 可以分配的最大内存，不会超过一个页面
    ngx_pool_t           *current;  // struct ngx_pool_s的typedef
    ngx_chain_t          *chain;  // 链接内存池
    ngx_pool_large_t     *large;  // 大块内存的入口指针
    ngx_pool_cleanup_t   *cleanup;  // 有关的清理行为
    ngx_log_t            *log;  // 日志
};
```
其中```ngx_pool_data_t```表示内存池头部信息：
```cpp
typedef struct {
    u_char               *last;
    u_char               *end;
    ngx_pool_t           *next;
    ngx_uint_t            failed;
} ngx_pool_data_t;
```


## 2. 创建nginx内存池
```cpp
ngx_pool_t * ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;

    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_t);  // 实际上内存池可以使用的大小，去掉头信息占用
    p->max = // 可以分配的最大内存大小，取较小值
    (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;  

    p->current = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    p->log = log;

    return p;
}
```
<img src='img/1.png'>
注意这里未使用和已使用的空间远大于前面的区域。

<br>

- 函数```ngx_memalign```，在不同的宏定义情况下，函数的实现有所不同：
    ```cpp
    #if (NGX_HAVE_POSIX_MEMALIGN || NGX_HAVE_MEMALIGN)

    void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log);

    #else

    #define ngx_memalign(alignment, size, log)  ngx_alloc(size, log)

    #endif
    ```
    如两个宏: ```NGX_HAVE_POSIX_MEMALIGN``` 和 ```NGX_HAVE_MEMALIGN``` 都没有定义，则函数```ngx_memalign``` 直接使用 ```ngx_alloc```, ```ngx_alloc``` 实现的就是简单的malloc开辟内存，成功与否都会更新日志。
    ```cpp
    void * ngx_alloc(size_t size, ngx_log_t *log)
    {
        void  *p;

        p = malloc(size);
        if (p == NULL) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                        "malloc(%uz) failed", size);
        }

        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

        return p;
    }
    ```
    由于定义是一样的，只有实现方面不同，故两种宏的情况在定义出合并为```(NGX_HAVE_POSIX_MEMALIGN || NGX_HAVE_MEMALIGN)```. 当宏定义 ```NGX_HAVE_POSIX_MEMALIGN``` 存在时，函数的实现为：
    ```cpp
    void * ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
    {
        void  *p;
        int    err;

        err = posix_memalign(&p, alignment, size);

        if (err) {  // 注意成功返回0，不成功非0
            ngx_log_error(NGX_LOG_EMERG, log, err,
                        "posix_memalign(%uz, %uz) failed", alignment, size);
            p = NULL;
        }

        ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                    "posix_memalign: %p:%uz @%uz", p, size, alignment);

        return p;
    }
    ```
    当宏定义 ```NGX_HAVE_MEMALIGN``` 存在时，函数的实现为：
    ```cpp
    void * ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
    {
        void  *p;

        p = memalign(alignment, size);
        if (p == NULL) {
            ngx_log_error(NGX_LOG_EMERG, log, ngx_errno,
                        "memalign(%uz, %uz) failed", alignment, size);
        }

        ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0,
                    "memalign: %p:%uz @%uz", p, size, alignment);

        return p;
    }
    ```
    ```posix_memalign``` 和 ```memalign``` 都是系统的api，是标准C库中的内存分配函数，用于满足特定对齐要求的内存分配需求。前者会尝试分配一个大小为size字节，按照alignment字节对齐的内存块，并将其起始地址存储在p中。后者则返回已分配内存的地址。这两个函数在window都没有。

    后续的操作大致如下：
    <img src='img/2.png'>

### 3. 从内存池中申请内存
主要有两种函数：```ngx_palloc```, ```ngx_pnalloc```, 区分点在于考不考虑对齐：

```cpp
void* ngx_palloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 1); // 考虑对齐
    }
#endif

    return ngx_palloc_large(pool, size);
}
```

```cpp
void* ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);  // 不考虑对齐
    }
#endif

    return ngx_palloc_large(pool, size);
}
```
二者的逻辑非常清晰，如果要分配的内存大小小于等于```pool->max```, 那么就调用小块内存的分配函数```ngx_palloc_small```，如果大于```pool->max```, 那么就调用大块内存的分配函数```ngx_palloc_large```.

- 小块内存的分配函数```ngx_palloc_small```
    ```cpp
    static ngx_inline void* ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
    {
        u_char      *m;
        ngx_pool_t  *p;

        p = pool->current;  // 先看current指向哪个内存块

        do {
            m = p->d.last;  // 找到可用空间的起始位置

            if (align) {
                m = ngx_align_ptr(m, NGX_ALIGNMENT);
            }

            if ((size_t) (p->d.end - m) >= size) {  // 判断空间够不够
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return ngx_palloc_block(pool, size);
    }
    ```
    <img src='img/1.gif'>

    如果```(size_t) (p->d.end - m) < size``` ，即小块内存的分配过程，当前内存块没有足够的空间了，则不会走到有return的语句，并且初始化的时候```d.next```是NULL，于是就退出了循环，执行函数```ngx_palloc_block()```，开辟新的内存块等一系列操作。新开辟的内存块只需要```ngx_pool_data_t```，不需要额外的信息，见下图。

    ```cpp
    static void* ngx_palloc_block(ngx_pool_t *pool, size_t size)
    {
        u_char      *m;
        size_t       psize;
        ngx_pool_t  *p, *new;

        psize = (size_t) (pool->d.end - (u_char *) pool);  // 原先池的大小

        m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);  // 开辟新的内存
        if (m == NULL) {  // 开辟失败的话....
            return NULL;
        }

        new = (ngx_pool_t *) m;  // 新池的起点

        new->d.end = m + psize;  // 新池的结尾
        new->d.next = NULL;
        new->d.failed = 0;  // 记录内存分配失败的次数

        m += sizeof(ngx_pool_data_t);  // 定位到max?
        m = ngx_align_ptr(m, NGX_ALIGNMENT);  // 
        new->d.last = m + size;

        for (p = pool->current; p->d.next; p = p->d.next) {
            if (p->d.failed++ > 4) {
                pool->current = p->d.next;
            }
        }

        p->d.next = new;

        return m;
    }
    ```
    <img src='img/2.gif'>

    注意```failed```, 它记录内存分配“失败”的次数。如果发现剩余空间不足以分配对应大小的内存，累计五次，这个内存块就会被认为剩余空间足够小，就不会再使用。当然前提是他的```next```不为NULL，为NULL的话应该先去创建新的空间。

    ```cpp
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }
    ```
    这段代码是指当开辟完新的内存块，分配出去内存之后。结合```ngx_palloc_small```函数中的```do while```循环，可知：访问第一个内存块，发现空间不够了，就通过```p = p->d.next;```访问下一个内存块，重复如此，如果每一个都不够，那就开辟新的内存块。此时前面的内存块都需要把它们对应的```failed```加一，便是通过这个```for```循环实现的。如果累计五次，就执行```pool->current = p->d.next;```, 即不再使用这个内存块。因为正如```ngx_palloc_small```中所示，要先看```current```指向哪个内存块。
    <img src='img/3.gif'>