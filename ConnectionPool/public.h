#pragma once
#include <iostream>

#define LOG(str) \
	std::cout << __FILE__ << ":"<< __LINE__ << " " <<\
	 __TIMESTAMP__ " : " << str << std::endl

// 单例模式
// 线程安全：主线程创建对象，子线程调用，规避线程安全问题
template<typename T>
class Singleton {
public:
	Singleton() = delete;
	Singleton(const Singleton<T>&) = delete;
	Singleton(const Singleton<T>&&) = delete;
	Singleton& operator=(const Singleton<T>&) = delete;

	static T* getInstance() {
		if (!m_instance) { m_instance = new T(); }
		return m_instance;
	}

private:
	static T* m_instance;
};

template<typename T>
T* Singleton<T>::m_instance = nullptr;


