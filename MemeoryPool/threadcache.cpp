#include "threadcache.h"

void* ThreadCache::fetchFromCentralCache(size_t index, size_t n) {
		
}
void* ThreadCache::allocate(size_t n) {
	assert(n <= MAX_BYTES);
	size_t roundUpSize = Size::roundUp(n);
	size_t index = Size::freelistIndex(n);

	if (m_freelists[index].empty()) { return fetchFromCentralCache(index, roundUpSize); }
	else { return m_freelists[index].pop(); }
}

void ThreadCache::deallocate(void* p, size_t n) {
	assert(n < MAX_BYTES);
	assert(p);

	size_t index = Size::freelistIndex(n);
	m_freelists[index].push(p);
}