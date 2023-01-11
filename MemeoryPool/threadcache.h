#pragma once
#include "public.h"


class ThreadCache {
public:
	void* allocate(size_t n);
	void deallocate(void* p, size_t n);

	void* fetchFromCentralCache(size_t index, size_t n);

private:
	Freelist m_freelists[NFREELIST];
};
// 线程局部存储，对于每个线程都是独立的，每个线程都拥有一个 pThreadCache
static _declspec(thread) ThreadCache* pThreadCache = nullptr;