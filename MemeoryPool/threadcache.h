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
// �ֲ߳̾��洢������ÿ���̶߳��Ƕ����ģ�ÿ���̶߳�ӵ��һ�� pThreadCache
static _declspec(thread) ThreadCache* pThreadCache = nullptr;