#pragma once

#include "public.h"
#include "threadcache.h"

static void* concurrentAlloc(size_t n) {
	// 通过 TLS，每个线程可以以无锁的方式获取自己的 ThreadCache 对象
	if (nullptr == pThreadCache) {
		pThreadCache = new ThreadCache;
	}
	return pThreadCache->allocate(n);
}

static void concurrentFree(void* p) {
	assert(pThreadCache);
	pThreadCache->deallocate(p);
}
