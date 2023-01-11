#pragma once

#include <assert.h>

static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208;

static void*& nextObj(void* obj) {
	return *(void**)obj;
}

class Freelist {
public:
	void push(void* obj) {
		assert(obj);
		nextObj(obj) = m_freelist;
		m_freelist = obj;
	}
	void* pop() {
		assert(m_freelist);
		void* obj = m_freelist;
		m_freelist = nextObj(obj);
		return obj;
	}
	bool empty() {
		return m_freelist == nullptr;
	}
private:
	void* m_freelist = nullptr;
};

class Size {
private:
	static size_t _roundUp(size_t bytes, int align) {
		return (((bytes)+(size_t)align - 1) & ~((size_t)align - 1));
	}
	static  size_t _freelistIndex(size_t bytes, int align) {
		return (((bytes)+(size_t)align - 1) / (size_t)align - 1);
	}
public:
	static size_t roundUp(size_t bytes) {
		if (bytes <= 128) return _roundUp(bytes, 8);
		else if (bytes <= 1024) return _roundUp(bytes, 16);
		else if (bytes <= 1024 * 8) return _roundUp(bytes, 128);
		else if (bytes <= 1024 * 64) return _roundUp(bytes, 1024);
		else if (bytes <= 1024 * 256) return _roundUp(bytes, 8192);
		else return -1;
	}

	static size_t freelistIndex(size_t bytes) {
		assert(bytes <= MAX_BYTES);
		static int group[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) return _freelistIndex(bytes, 8);
		else if (bytes <= 1024) return _freelistIndex(bytes - 128, 16) + group[0];
		else if (bytes <= 1024 * 8) return _freelistIndex(bytes - 1024, 128) + group[1] + group[0];
		else if (bytes <= 1024 * 64) return _freelistIndex(bytes - 1024 * 8, 1024) + group[2] + group[1] + group[0];
		else if (bytes <= 1024 * 256) return _freelistIndex(bytes - 1024 * 64, 8192) + group[3] + group[2] + group[1] + group[0];
		else return -1;
	}
};