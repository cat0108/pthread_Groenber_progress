#include<iostream>
#include"bitmap.h"
#include<fstream>
#include<vector>
#include<sstream>
#include<xmmintrin.h>
#include<emmintrin.h>
#include<immintrin.h>
#include<Windows.h>
#include<pthread.h>
using namespace std;

typedef struct {
	int t_id;
	BitMap* bit1;
	BitMap* bit2;
	int control;//控制信号，控制消去还是加回
	int elimi1;
	int i;
}threadParam_t;
const int Num_thread = 4;
pthread_t* handles = new pthread_t[Num_thread];//创建线程句柄
threadParam_t* param = new threadParam_t[Num_thread];//需要传递的参数
__m256i r0, r1;

//线程函数编写
void* threadFunc1(void* Param)
{
	threadParam_t* p = (threadParam_t*)Param;//传递参数
	BitMap* bit1 = p->bit1;
	BitMap* bit2 = p->bit2;
	int t_id = p->t_id;
	int control = p->control;
	int i = p->i;
	int elimi1 = p->elimi1;
	int j;
		for (j = t_id; j + 8 <= bit2[i].Getsize(); j += 8*Num_thread)
		{
			r0 = _mm256_loadu_si256((__m256i*)(bit2[i].Getbits() + j));
			r1 = _mm256_loadu_si256((__m256i*)(bit1[control].Getbits() + j));
			r1 = _mm256_xor_si256(r0, r1);
			_mm256_storeu_si256((__m256i*)(bit2[i].Getbits() + j), r1);
		}
		if (t_id == 0)
			for (j; j < bit2[i].Getsize(); j++)
				bit2[i].Getbits()[j] = bit2[i].Getbits()[j] ^ bit1[control].Getbits()[j];
		pthread_exit(NULL);
		return NULL;
}

void* threadFunc2(void* Param)
{
	threadParam_t* p = (threadParam_t*)Param;//传递参数
	BitMap* bit1 = p->bit1;
	BitMap* bit2 = p->bit2;
	int t_id = p->t_id;
	int control = p->control;
	int i = p->i;
	int j;
	int elimi1 = p->elimi1;
	for (j = t_id; j + 8 <= bit2[i].Getsize(); j += 8 * Num_thread)
	{
		r0 = _mm256_loadu_si256((__m256i*)(bit2[i].Getbits() + j));
		r1 = _mm256_loadu_si256(&r0);
		_mm256_storeu_si256((__m256i*)(bit1[elimi1 + i].Getbits() + j), r1);
	}
	if (t_id == 0)
		for (j; j < bit2[j].Getsize(); j++)
			bit1[elimi1 + i].Getbits()[j] = bit2[i].Getbits()[j];
	pthread_exit(NULL);
	return NULL;
}

int Find_row(BitMap* bit1, int elimi1, int key)//找消元子是否存在对应首项，不存在返回-1，存在返回首项所在的行数
{
	for (int i = 0; i < elimi1; i++)
		if (bit1[i].find_max() == key)
			return i;
	return -1;
}

//参数分别为消元子，被消元行，矩阵列数，消元子行数，被消元行行数
void Guass_elimination(BitMap* bit1, BitMap* bit2, int col, int elimi1, int elimi2)
{
	for (int i = 0; i < elimi2; i++)//对被消元行进行遍历
	{
		while (!bit2[i].judge_zero())//如果整行不为0则进行消元操作
		{
			int temp = Find_row(bit1, elimi1 + elimi2, bit2[i].find_max());//找消元子是否存在对应首项
			if (temp != -1)
			{
				for (int t_id = 0; t_id < Num_thread; t_id++)
				{
					param[t_id].t_id = t_id;
					param[t_id].bit1 = bit1;
					param[t_id].bit2 = bit2;
					param[t_id].control = temp;
					param[t_id].elimi1 = elimi1;
					param[t_id].i = i;
					pthread_create(&handles[t_id], NULL, threadFunc1, &param[t_id]);
				}
				for (int t_id = 0; t_id < Num_thread; t_id++)
					pthread_join(handles[t_id], NULL);//获得返回值
			}
			else
			{
				for (int t_id = 0; t_id < Num_thread; t_id++)
				{
					param[t_id].t_id = t_id;
					param[t_id].bit1 = bit1;
					param[t_id].bit2 = bit2;
					param[t_id].control = temp;
					param[t_id].elimi1 = elimi1;
					param[t_id].i = i;
					pthread_create(&handles[t_id], NULL, threadFunc2, &param[t_id]);
				}
				for (int t_id = 0; t_id < Num_thread; t_id++)
					pthread_join(handles[t_id], NULL);//获得返回值
				break;
			}
		}
	}
}

int main()
{
	LARGE_INTEGER fre, begin, end;
	double gettime;
	int col, elimi1, elimi2;
	cout << "输入总列数，消元子，被消元行数" << endl;
	cin >> col >> elimi1 >> elimi2;

	QueryPerformanceFrequency(&fre);
	QueryPerformanceCounter(&begin);


	//读取被消元行,保存到BitMap中
	vector<vector<int>> matrix;
	ifstream inputfile("C:\\Users\\cat\\Desktop\\并行程序设计\\SIMD\\Groebnerdata\\测试样例4 矩阵列数1011，非零消元子539，被消元行263\\被消元行.txt");
	if (inputfile.is_open())
	{
		string line;
		while (getline(inputfile, line))//一行一行地读取数据
		{
			vector<int> row;
			stringstream ss(line);//构造字符串输入流，用于数据转换
			int num;
			while (ss >> num)
				row.push_back(num);
			matrix.push_back(row);
		}
		inputfile.close();
	}

	BitMap* bit1 = new BitMap[elimi2];
	for (int i = 0; i < elimi2; i++)
	{
		bit1[i] = *(new BitMap(col + 1));
		bit1[i].initialize();
	}
	//赋值到位图中去
	for (int i = 0; i < (int)matrix.size(); i++)
	{
		for (int j = 0; j < (int)matrix[i].size(); j++)
		{
			/*cout << matrix[i][j]<<" ";*/
			bit1[i].add(matrix[i][j]);
		}

		/*cout << endl<<endl;*/
	}

	/*for (int i = 0; i < elimi2; i++) {
		for (int j = 0; j <= col; j++)
			cout << bit1[i].Find(j);
		cout << endl<<endl;
	}*/

	QueryPerformanceCounter(&end);
	gettime = (double)((end.QuadPart - begin.QuadPart) * 1000.0) / (double)fre.QuadPart;
	cout << "read time: " << gettime << " ms" << endl << endl;

	QueryPerformanceFrequency(&fre);
	QueryPerformanceCounter(&begin);


	//读取消元子,保存到BitMap中
	vector<vector<int>> matrix1;
	ifstream inputfile1("C:\\Users\\cat\\Desktop\\并行程序设计\\SIMD\\Groebnerdata\\测试样例4 矩阵列数1011，非零消元子539，被消元行263\\消元子.txt");
	if (inputfile1.is_open())
	{
		string line;
		while (getline(inputfile1, line))//一行一行地读取数据
		{
			vector<int> row;
			stringstream ss(line);//构造字符串输入流，用于数据转换
			int num;
			while (ss >> num)
				row.push_back(num);
			matrix1.push_back(row);
		}
		inputfile1.close();
	}

	BitMap* bit2 = new BitMap[elimi1 + elimi2];
	for (int i = 0; i < elimi1 + elimi2; i++)
	{
		bit2[i] = *(new BitMap(col));
		bit2[i].initialize();
	}
	for (int i = 0; i < (int)matrix1.size(); i++)
	{
		for (int j = 0; j < (int)matrix1[i].size(); j++)
		{
			/*cout << matrix1[i][j] << " ";*/
			bit2[i].add(matrix1[i][j]);
		}

		/*cout << endl << endl;*/
	}
	/*for (int i = 0; i < elimi1 + elimi2; i++) {
		for (int j = 0; j <= col; j++)
			cout << bit2[i].Find(j);
		cout << endl << endl;
	}*/

	QueryPerformanceCounter(&end);
	gettime = (double)((end.QuadPart - begin.QuadPart) * 1000.0) / (double)fre.QuadPart;
	cout << "read time: " << gettime << " ms" << endl << endl;


	//时间测试

	QueryPerformanceFrequency(&fre);
	QueryPerformanceCounter(&begin);

	Guass_elimination(bit2, bit1, col, elimi1, elimi2);

	QueryPerformanceCounter(&end);
	gettime = (double)((end.QuadPart - begin.QuadPart) * 1000.0) / (double)fre.QuadPart;
	cout << "elimination time: " << gettime << " ms" << endl << endl;


	//for (int i = 0; i < elimi2; i++) {
	//	for (int j = 0; j <= col; j++)
	//		cout << bit1[i].Find(j);
	//	cout << endl << endl;
	//}
}
