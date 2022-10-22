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
		char* ChunkHead; //Chunk���׵�ַ
		Chunk* nextChunk;
	};
	Chunk* memoryChunks; //�ڴ��Chunk����
	char* startFreeMem;
	char* endFreeMem;	//���λ�ò��ܷ����ݣ��Ǵ�����һ���׵�ַ�������ڴ��
	const size_t ChunkSize = 4096;

	struct RecycleBlock {
		char* startAddress; //Block���׵�ַ
		size_t blockSize; //Block�Ĵ�С
		RecycleBlock* nextRecBlock;
	};

	map<size_t, RecycleBlock*> RecMap; //����Block��map
	void Recycle(char* startAddress, size_t offset); //�Ѳ��õĿ������
	char* FindBlock(size_t offset); //�ҵ����õĿ�
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
	//���յĿ鱾����Ҳ���ڴ���ֻ��Ҫ�ͷŴ����У����տ��������
	Chunk* c;
	for (c = memoryChunks; c != nullptr; c = c->nextChunk)
	{
		//�ͷŴ���ڴ��
		operator delete(c->ChunkHead);
	}
	
	//������
	while (memoryChunks != nullptr)
	{
		c = memoryChunks;
		memoryChunks = c->nextChunk;
		free(c);
	}
}





/// <summary>
/// ���л���
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
		//û�ҵ�
		RecMap.insert(pair<size_t, RecycleBlock*>(offset, newRecBlock));
	}
	else {
		newRecBlock->nextRecBlock = it->second;
		it->second = newRecBlock;
	}
}



/// <summary>
/// ���������ڴ��Ҫ�ڴ棬�ڴ�صĴ������
/// </summary>
/// <param name="totalBytes"></param>
/// <returns></returns>
char* MemoryPool::AllocateMem(size_t totalBytes)
{
	size_t freeSize = GetOffset();

	//һ��Ҫ��̫���ˣ�ֱ������һ����Ҫ��
	if (totalBytes > ChunkSize)
	{
		char* res = reinterpret_cast<char*>(operator new(sizeof(char) * totalBytes));
		return res;
	}

	
	//ȥ���տ�����һȦ��������û�к��ʵĿ����
	char* recRes = FindBlock(totalBytes);
	if (recRes != nullptr)
	{
		return recRes;
	}
	

	//�ڴ���ﻭһ�����
	if (freeSize >= totalBytes)
	{
		char* res = startFreeMem;
		startFreeMem += totalBytes;
		return res;
	}

	
	//��������������룬����������ʣ�²��ֲ����������ˣ���ʣ�µĲ��� ���� ����Ȼ����������µ�
	if (startFreeMem < endFreeMem)
	{
		Recycle(startFreeMem, freeSize);
	}
	

	//�����µĿ�
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

	//���µ�Chunk����ǰ��
	newChunk->nextChunk = memoryChunks;
	memoryChunks = newChunk;
	
	//�����µĵ�ַ
	startFreeMem = memoryChunks->ChunkHead;
	endFreeMem = startFreeMem + ChunkSize;

	//���¿����һ���ַ���
	char* res = startFreeMem;
	startFreeMem += totalBytes;
	return res;
}



/// <summary>
/// ��Ҫ�Ŀ�ֱ�ӻ���
/// </summary>
/// <param name="MemPt"></param>
/// <param name="totalBytes"></param>
void MemoryPool::DeallocateMem(char* MemPt, size_t totalBytes)
{
	//̫���ֱ�ӷ����ڴ�
	if (totalBytes > ChunkSize)
	{
		operator delete(MemPt);
		return;
	}
	Recycle(MemPt, totalBytes);
}



/// <summary>
/// �ڻ��տ����ҿ�
/// </summary>
/// <param name="totalBytes"></param>
/// <returns></returns>
char* MemoryPool::FindBlock(size_t totalBytes)
{
	auto it = RecMap.find(totalBytes);
	if (it != RecMap.end())
	{
		//�ҵ���
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
		//�ҵ��������˵�
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
		//û�ҵ�
		return nullptr;
	}

}