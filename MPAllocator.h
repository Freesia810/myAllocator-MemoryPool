#pragma once
#include <climits>
#include "MemoryPool.h"

template<typename T>
class MPAllocator
{
public:
	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef size_t size_type;
	template <typename U> struct rebind {
		typedef MPAllocator<U> other;
	};

	MPAllocator();
	~MPAllocator();
	template <typename U> MPAllocator(const MPAllocator<U>& mpAllocator) noexcept;

	pointer address(reference x) const noexcept;
	const_pointer address(const_reference x) const noexcept;
	pointer allocate(size_type n = 1, const_pointer hint = 0);
	void deallocate(pointer p, size_type n = 1);
	size_type max_size();

	template <typename U, class... Args> void construct(U* p, Args&&... args);
	template <typename U> void destroy(U* p);
};

MemoryPool memoryPool;

template<typename T>
MPAllocator<T>::MPAllocator()
{

}

template<typename T>
MPAllocator<T>::~MPAllocator()
{

}

template<typename T>
inline typename MPAllocator<T>::pointer MPAllocator<T>::address(reference x) const noexcept
{
	return &x;
}
template<typename T>
inline typename MPAllocator<T>::const_pointer MPAllocator<T>::address(const_reference x) const noexcept
{
	return &x;
}

template<typename T>
inline typename MPAllocator<T>::pointer MPAllocator<T>::allocate(size_type n, const_pointer hint)
{
	size_type totalBytes = n * sizeof(value_type);
	return reinterpret_cast<pointer>(memoryPool.AllocateMem(totalBytes));
}

template<typename T>
inline void MPAllocator<T>::deallocate(pointer p, size_type n)
{
	size_type totalBytes = n * sizeof(value_type);
	memoryPool.DeallocateMem(reinterpret_cast<char*>(p), totalBytes);
}

template<typename T>
inline typename MPAllocator<T>::size_type MPAllocator<T>::max_size()
{
	return SIZE_MAX;
}


template<typename T>
template<typename U>
inline MPAllocator<T>::MPAllocator(const MPAllocator<U>& mpAllocator) noexcept:MPAllocator()
{

}

template<typename T>
template<typename U, class ...Args>
inline void MPAllocator<T>::construct(U* p, Args && ...args)
{
	new (p) U(std::forward<Args>(args)...);
}


template<typename T>
template<typename U>
inline void MPAllocator<T>::destroy(U* p)
{
	//不管好像更快
	p->~U();
}
