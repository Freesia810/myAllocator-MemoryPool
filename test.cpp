#include "MPAllocator.h"
#include <vector>
#include <utility>
#include <memory>
#include <random>
#include <iostream>
#include <list>
#include <Windows.h>

//#define MPAllocator std::allocator


#define TIMES 1

#define INT 1
#define FLOAT 2
#define DOUBLE 3
#define CLASS 4
constexpr auto TestSize = 10000;
constexpr auto PickSize = 1000;

using namespace std;

using Point2D = pair<int, int>;


class vecWrapper
{
public:
	vecWrapper() {
		m_pVec = NULL;
		m_type = INT;
	}
	virtual ~vecWrapper() {
	}
public:
	void setPointer(int type, void* pVec) { m_type = type; m_pVec = pVec; }
	virtual void visit(int index) = 0;
	virtual int size() = 0;
	virtual void resize(int size) = 0;
	virtual bool checkElement(int index, void* value) = 0;
	virtual void setElement(int idex, void* value) = 0;
protected:
	int m_type;
	void* m_pVec;
};

template<class T>
class vecWrapperT : public vecWrapper
{
public:
	vecWrapperT(int type, std::vector<T, MPAllocator<T> >* pVec)
	{
		m_type = type;
		m_pVec = pVec;
	}
	virtual ~vecWrapperT() {
		if (m_pVec)
			delete ((std::vector<T, MPAllocator<T> > *)m_pVec);
	}
public:
	virtual void visit(int index)
	{
		T temp = (*(std::vector<T, MPAllocator<T> > *)m_pVec)[index];
	}
	virtual int size()
	{
		return ((std::vector<T, MPAllocator<T> > *)m_pVec)->size();
	}
	virtual void resize(int size)
	{
		((std::vector<T, MPAllocator<T> > *)m_pVec)->resize(size);
	}
	virtual bool checkElement(int index, void* pValue)
	{
		T temp = (*(std::vector<T, MPAllocator<T> > *)m_pVec)[index];
		if (temp == (*((T*)pValue)))
			return true;
		else
			return false;
	}

	virtual void setElement(int index, void* value)
	{
		(*(std::vector<T, MPAllocator<T> > *)m_pVec)[index] = *((T*)value);
	}
};

class myObject
{
public:
	myObject() : m_X(0), m_Y(0) {}
	myObject(int t1, int t2) :m_X(t1), m_Y(t2) {}
	myObject(const myObject& rhs) { m_X = rhs.m_X; m_Y = rhs.m_Y; }
	~myObject() { /*std::cout << "my object destructor called" << std::endl;*/ }
	bool operator == (const myObject& rhs)
	{
		if ((rhs.m_X == m_X) && (rhs.m_Y == m_Y))
			return true;
		else
			return false;
	}
protected:
	int m_X;
	int m_Y;
};


int main()
{
	long t1 = GetTickCount64();

	for (int i = 0; i < TIMES; i++)
	{
		vecWrapper** testVec;
		testVec = new vecWrapper * [TestSize];

		int tIndex, tSize;
		//test allocator
		for (int i = 0; i < TestSize - 4; i++)
		{
			tSize = (int)((float)rand() / (float)RAND_MAX * 100);
			vecWrapperT<int>* pNewVec = new vecWrapperT<int>(INT, new vector<int, MPAllocator<int>>(tSize));
			testVec[i] = (vecWrapper*)pNewVec;
		}

		for (int i = 0; i < 4; i++)
		{
			tSize = (int)((float)rand() / (float)RAND_MAX * 10000);
			vecWrapperT<myObject>* pNewVec = new vecWrapperT<myObject>(CLASS, new vector<myObject, MPAllocator<myObject>>(tSize));
			testVec[TestSize - 4 + i] = (vecWrapper*)pNewVec;
		}

		//test resize
		for (int i = 0; i < 100; i++)
		{
			tIndex = (int)((float)rand() / (float)RAND_MAX * (float)TestSize);
			tSize = (int)((float)rand() / (float)RAND_MAX * (float)TestSize);
			testVec[tIndex]->resize(tSize);
		}

		//test assignment
		tIndex = (int)((float)rand() / (float)RAND_MAX * (TestSize - 4 - 1));
		int tIntValue = 10;
		testVec[tIndex]->setElement(testVec[tIndex]->size() / 2, &tIntValue);
		if (!testVec[tIndex]->checkElement(testVec[tIndex]->size() / 2, &tIntValue))
			cout << "incorrect assignment in vector " << tIndex << endl;
		else
			cout << "correct assignment in vector " << tIndex << endl;

		tIndex = TestSize - 4 + 3;
		myObject tObj(11, 15);
		testVec[tIndex]->setElement(testVec[tIndex]->size() / 2, &tObj);
		if (!testVec[tIndex]->checkElement(testVec[tIndex]->size() / 2, &tObj))
			cout << "incorrect assignment in vector " << tIndex << endl;
		else
			cout << "correct assignment in vector " << tIndex << endl;


		myObject tObj1(13, 20);
		testVec[tIndex]->setElement(testVec[tIndex]->size() / 2, &tObj1);
		if (!testVec[tIndex]->checkElement(testVec[tIndex]->size() / 2, &tObj1))
			cout << "incorrect assignment in vector " << tIndex << " for object (13,20)" << endl;
		else
			cout << "correct assignment in vector " << tIndex << " for object (13,20)" << endl;

		for (int i = 0; i < TestSize; i++)
			delete testVec[i];

		delete[]testVec;



		//PTA test
		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<> dis(1, TestSize);

		// vector creation
		using IntVec = vector<int, MPAllocator<int>>;
		vector<IntVec, MPAllocator<IntVec>> vecints(TestSize);

		for (int i = 0; i < TestSize; i++)
			vecints[i].resize(dis(gen));

		using PointVec = vector<Point2D, MPAllocator<Point2D>>;
		vector<PointVec, MPAllocator<PointVec>> vecpts(TestSize);

		for (int i = 0; i < TestSize; i++)
			vecpts[i].resize(dis(gen));

		// vector resize
		for (int i = 0; i < PickSize; i++)
		{
			int idx = dis(gen) - 1;
			int size = dis(gen);
			vecints[idx].resize(size);
			vecpts[idx].resize(size);
		}

		// vector element assignment
		{
			int val = 10;
			int idx1 = dis(gen) - 1;
			int idx2 = vecints[idx1].size() / 2;
			vecints[idx1][idx2] = val;
			if (vecints[idx1][idx2] == val)
				cout << "correct assignment in vecints: " << idx1 << endl;
			else
				cout << "incorrect assignment in vecints: " << idx1 << endl;
		}
		{
			Point2D val(11, 15);
			int idx1 = dis(gen) - 1;
			int idx2 = vecpts[idx1].size() / 2;
			vecpts[idx1][idx2] = val;
			if (vecpts[idx1][idx2] == val)
				cout << "correct assignment in vecpts: " << idx1 << endl;
			else
				cout << "incorrect assignment in vecpts: " << idx1 << endl;
		}
	}


	long t2 = GetTickCount64();
	cout << "程序运行时间：" << t2 - t1 << "ms\n";

	system("pause");
	return 0;
}