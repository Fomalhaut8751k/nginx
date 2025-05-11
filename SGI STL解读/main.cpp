#include<iostream>

using namespace std;

class __default_alloc_template
{
public:
	static void* allocate(size_t __n)
	{
		void* __ret = 0;

		// ###### �������ָ����С������һ���Ŀռ�����������stl��һ�� ######
		if (__n > (size_t)_MAX_BYTES) {
			__ret = malloc_alloc::allocate(__n);
		}
		// ###### ���򣬾��ö����Ŀռ������� ######
		else {
			_Obj* volatile* __my_free_list = _S_free_list + _S_freelist_index(__n);  // ������ʼλ��+ƫ����
#     ifndef _NOTHREADS
			_Lock __lock_instance;  // ����������֤�̰߳�ȫ
#     endif
			_Obj* __RESTRICT __result = *__my_free_list; // �������ֵ
			if (__result == 0)
				__ret = _S_refill(_S_round_up(__n));  // �ڵ�ǰλ�÷������ɸ�16�ֽڵ�chunk�飬_retָ�������ͷ
			else {
				*__my_free_list = __result->_M_free_list_link;  // _M_free_list_link�൱�������ָ���򣬼�next
				__ret = __result;
				/*
					��Ϊ��ǰ��chunk�飨__result�����������ȥ������Ҫ��
					_my_free_list��ָ����һ��chunk��
				*/
			}
		}

		return __ret;
	}

private:

	// ###### ��Ҫ���ͺͱ������� ###############################################

	// �ڴ�ص�������Ϣ
	enum { _ALIGN = 8 };
	enum { _MAX_BYTES = 128 };
	enum { _NFREELISTS = 16 }; // _MAX_BYTES/_ALIGN

	// ÿһ���ڴ�chunk���ͷ��Ϣ����������ڵ�
	union _Obj {    
		union _Obj* _M_free_list_link;
		char _M_client_data[1];    /* The client sees this */
	};

	// ��֯����������������飬�����ÿһ��Ԫ�ص�������_Obj*��ȫ����ʼ��Ϊ0
	static _Obj* volatile _S_free_list[_NFREELISTS];

	// Chunk allocation state. ��¼�ڴ�chunk��ķ��������ȫ����ʼ��Ϊ0
	static char* _S_start_free;
	static char* _S_end_free;
	static size_t _S_heap_size;


	// ###### ��Ҫ�������� ####################################################

	// �� __bytes �ϵ������ڽ��� 8 �ı���
	static size_t _S_round_up(size_t __bytes)
	{
		return (((__bytes)+(size_t)_ALIGN - 1) & ~((size_t)_ALIGN - 1));
		/*
			size_t: ���ֽڣ����ԣ�
			 7 -> 00000000 00000000 00000000 00000111
			~7 -> 11111111 11111111 11111111 11111000
			6 + 7 = 13 -> 00000000 00000000 00000000 00001101
			13 & ~7 = 00000000 00000000 00000000 00001000 = 8
		*/
	}

	// ����__bytes��С��chunk��λ�� free-list �еı��
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