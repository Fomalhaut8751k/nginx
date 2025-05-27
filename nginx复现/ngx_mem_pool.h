#pragma once

/*
	移植nginx内存池的代码用，OOP来实现
*/

class ngi_mem_pool
{
public:

private:
	struct ngx_pool_s
	{
		ngx_pool_data_t d;
		size_t max;
		ngx_pool_t* current;
		ngx_chain_t& chain;
		ngx_pool_large_t* large;
		ngx_pool_cleanup_t* cleanup;
		ngx_log_t* log;
	};

	ngx_pool_s* pool_;  // 指向nginx内存池的入口指针
};