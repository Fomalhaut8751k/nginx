#include<iostream>

using namespace std;

// ###### union #####################################################################
#if 0
/*
	union ��һ����������ݽṹ������������ͬ���ڴ�λ�ô洢��ͬ���������͡�
	union ���԰�����ʡ�ڴ棬��Ϊ����ʹ����ͬ���ڴ�ռ����洢��ͬ���������ͣ�
	����ֻ��ͬʱ�洢����һ��
*/
union _Obj  // ȡռ���ֽ���������
{
	_Obj* next;  // 4�ֽ�
	char a;  // 1�ֽ�
	int b;  // 4�ֽ�
};

int main()
{
	_Obj o1;

	cout << "_Obj.size: " << sizeof(_Obj) << endl;
	o1.b = 2;
	cout << "_Next = " <<  o1.next << "\t_intVal = " << o1.b << endl;
	/*
	* һ���ֽڰ˸����أ�һ��16�����൱��4λ���ʰ˸�����32λ��4�ֽ�
		��o1.b��ֵ��Ϊ2����00000002��������������ֽڿռ�ͱ�Ϊ00000002
		����o1.next����Ҳ��4���ֽڣ�������ʾ00000002
	*/
	cout << "charVal = " << o1.a << endl;  // ��ӡ������

	return 0;
}
#endif
// ###### �ڴ�ƫ�� #####################################################################
#if 0
#include<memory>
union _Obj 
{
	_Obj* next;  // 4�ֽ�
	char a;  // 1�ֽ�
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
// #### �ڴ濪�� #####################################################################
#if 1
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

	cout << "malloc���ٿռ�" << endl;

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
		�������̲�û�е����κι��캯����˵�����漰����Ĺ���
		ֻ���ڴ�Ŀ��١�������_Obj��һ��union���͵�....
	*/

	return 0;
}
#endif