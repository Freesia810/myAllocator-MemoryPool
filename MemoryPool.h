#pragma once
#include <cstddef>
#include <type_traits>
#include <iostream>
#include <map>

using namespace std;


class MemoryPool
{
public:
	char* AllocateMem(size_t totalBytes);
	void DeallocateMem(char* MemPt, size_t totalBytes);
	MemoryPool();
	~MemoryPool();
private:
	struct Chunk {
		char* ChunkHead; //Chunk的首地址
		Chunk* nextChunk;
	};
	Chunk* memoryChunks; //内存池Chunk链表
	char* startFreeMem;
	char* endFreeMem;	//这个位置不能放数据，是大块的下一个首地址，不属于大块
	const size_t ChunkSize = 4096;

	struct RecycleBlock {
		char* startAddress; //Block的首地址
		size_t blockSize; //Block的大小
		RecycleBlock* nextRecBlock;
	};

	map<size_t, RecycleBlock*> RecMap; //回收Block的map
	void Recycle(char* startAddress, size_t offset); //把不用的块回收了
	char* FindBlock(size_t offset); //找到可用的块
	size_t GetOffset();
};



size_t MemoryPool::GetOffset()
{
	return size_t(endFreeMem - startFreeMem);
}

MemoryPool::MemoryPool()
{
	startFreeMem = endFreeMem = nullptr;
	memoryChunks = (Chunk*)malloc(sizeof(Chunk));
	if (memoryChunks == nullptr)
	{
		cout << "Memorypool Initializing Failed...";
		return;
	}
	memoryChunks->ChunkHead = reinterpret_cast<char*>(operator new(sizeof(char) * ChunkSize));
	if (memoryChunks->ChunkHead == nullptr)
	{
		cout << "Memorypool Initializing Failed...";
		return;
	}
	memoryChunks->nextChunk = nullptr;
	startFreeMem = memoryChunks->ChunkHead;
	endFreeMem = startFreeMem + ChunkSize;
}

MemoryPool::~MemoryPool()
{
	//回收的块本质上也是在大块里，只需要释放大块就行，回收块就在里面
	Chunk* c;
	for (c = memoryChunks; c != nullptr; c = c->nextChunk)
	{
		//释放大的内存块
		operator delete(c->ChunkHead);
	}
	
	//链表处理
	while (memoryChunks != nullptr)
	{
		c = memoryChunks;
		memoryChunks = c->nextChunk;
		free(c);
	}
}





/// <summary>
/// 进行回收
/// </summary>
/// <param name="startAddress"></param>
/// <param name="offset"></param>
void MemoryPool::Recycle(char* startAddress, size_t offset)
{
	if (offset < 4)
	{
		return;
	}

	RecycleBlock* newRecBlock = (RecycleBlock*)malloc(sizeof(RecycleBlock));
	if (newRecBlock == nullptr)
	{
		cout << "Recycle error";
		return;
	}
	newRecBlock->startAddress = startAddress;
	newRecBlock->blockSize = offset;
	newRecBlock->nextRecBlock = nullptr;

	auto it = RecMap.find(offset);
	if (it == RecMap.end())
	{
		//没找到
		RecMap.insert(pair<size_t, RecycleBlock*>(offset, newRecBlock));
	}
	else {
		newRecBlock->nextRecBlock = it->second;
		it->second = newRecBlock;
	}
}



/// <summary>
/// 分配器向内存池要内存，内存池的处理过程
/// </summary>
/// <param name="totalBytes"></param>
/// <returns></returns>
char* MemoryPool::AllocateMem(size_t totalBytes)
{
	size_t freeSize = GetOffset();

	//一次要的太多了，直接申请一个他要的
	if (totalBytes > ChunkSize)
	{
		char* res = reinterpret_cast<char*>(operator new(sizeof(char) * totalBytes));
		return res;
	}

	
	//去回收块里找一圈，看看有没有合适的块给他
	char* recRes = FindBlock(totalBytes);
	if (recRes != nullptr)
	{
		return recRes;
	}
	

	//在大块里画一块给他
	if (freeSize >= totalBytes)
	{
		char* res = startFreeMem;
		startFreeMem += totalBytes;
		return res;
	}

	
	//对于现在这个申请，现在这个大块剩下部分不够给他的了，把剩下的部分 回收 掉，然后给他开个新的
	if (startFreeMem < endFreeMem)
	{
		Recycle(startFreeMem, freeSize);
	}
	

	//申请新的块
	Chunk* newChunk = (Chunk*)malloc(sizeof(Chunk));
	if (newChunk == nullptr)
	{
		cout << "New Chunk Creating Failed...";
		return nullptr;
	}
	newChunk->ChunkHead = reinterpret_cast<char*>(operator new(sizeof(char) * ChunkSize));
	if (newChunk->ChunkHead == nullptr)
	{
		cout << "New Chunk Creating Failed...";
		return nullptr;
	}

	//把新的Chunk放在前面
	newChunk->nextChunk = memoryChunks;
	memoryChunks = newChunk;
	
	//更新新的地址
	startFreeMem = memoryChunks->ChunkHead;
	endFreeMem = startFreeMem + ChunkSize;

	//在新块里分一部分返回
	char* res = startFreeMem;
	startFreeMem += totalBytes;
	return res;
}



/// <summary>
/// 不要的块直接回收
/// </summary>
/// <param name="MemPt"></param>
/// <param name="totalBytes"></param>
void MemoryPool::DeallocateMem(char* MemPt, size_t totalBytes)
{
	//太大的直接返回内存
	if (totalBytes > ChunkSize)
	{
		operator delete(MemPt);
		return;
	}
	Recycle(MemPt, totalBytes);
}



/// <summary>
/// 在回收块中找块
/// </summary>
/// <param name="totalBytes"></param>
/// <returns></returns>
char* MemoryPool::FindBlock(size_t totalBytes)
{
	auto it = RecMap.find(totalBytes);
	if (it != RecMap.end())
	{
		//找到了
		auto rec = it->second;
		char* res = rec->startAddress;
		if (rec->nextRecBlock == nullptr)
		{
			free(it->second);
			it->second = nullptr;
			RecMap.erase(it);
		}
		else {
			it->second = rec->nextRecBlock;
		}

		return res;
	}
	it = RecMap.upper_bound(totalBytes);
	if (it != RecMap.end())
	{
		//找到比他大了的
		auto cur = it->second;
		if (cur->nextRecBlock == nullptr)
		{
			RecMap.erase(it);
		}
		else {
			it->second = cur->nextRecBlock;
		}

		char* res = cur->startAddress;

		char* start = cur->startAddress + totalBytes;
		size_t offset = cur->blockSize - totalBytes;
		Recycle(start, offset);

		free(cur);

		return res;

	}
	else {
		//没找到
		return nullptr;
	}

}