#pragma once

#include "public.h"
#include "threadcache.h"

static void* concurrentAlloc(size_t n) {
	// ͨ�� TLS��ÿ���߳̿����������ķ�ʽ��ȡ�Լ��� ThreadCache ����
	if (nullptr == pThreadCache) {
		pThreadCache = new ThreadCache;
	}
	return pThreadCache->allocate(n);
}

static void concurrentFree(void* p) {
	assert(pThreadCache);
	pThreadCache->deallocate(p);
}
