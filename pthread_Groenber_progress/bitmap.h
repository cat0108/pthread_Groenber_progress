#pragma once

class BitMap {
	unsigned int* bits = NULL;//总的开辟数组
	int size;//总的开辟数组的个数
	int range;//总元素个数
public:
	BitMap() { };
	BitMap(int range)
	{
		this->range = range;
		size = (int)(range / 32 + 1);
		bits = new unsigned int[size];
	}
	~BitMap()
	{
		delete[] bits;
	}
	void add(int indata)
	{
		int index = (int)(indata / 32);
		int bit_index = indata % 32;
		bits[index] |= 1 << bit_index;//将位图的某一位置为1
	}
	void remove(int indata)
	{
		int index = (int)(indata / 32);
		int bit_index = indata % 32;
		bits[index] &= ~(1 << bit_index);//将该位置为0
	}
	bool Find(int indata)
	{
		int index = int(indata / 32);
		int bit_index = indata % 32;
		return (bits[index] >> bit_index) & 1;
	}
	void initialize()
	{
		for (int i = 0; i < size; i++)
			bits[i] = 0;
	}

	bool judge_zero()
	{
		bool flag = 1;
		for (int i = 0; i < size; i++)//可以并行化判断是否全为0
			if (bits[i] != 0)
				flag = 0;
		return flag;
	}
	
	/*unsigned int  find_max()
	{
		unsigned int max1 = 0;
		for (int i = 0; i < range; i++)
		{
			if (Find(i) == 1)
				max1 = i;
		}
		return max1;
	}*/
	//利用位图的功能寻找最大值
	unsigned int  find_max()
	{
		unsigned int max1 = 0;
		int i = size - 1;
		while (bits[i] == 0 && i > 0)
		{
			i--;
		}
		for (int j = 0; j < 32; j++)
			if (Find(i * 32 + j) == 1)
				max1 = i * 32 + j;
		return max1;
	}
	int Getsize() { return size; }
	unsigned int* Getbits() { return bits; }
};