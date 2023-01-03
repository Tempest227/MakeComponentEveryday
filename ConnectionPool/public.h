#pragma once
#include <iostream>

#define LOG(str) \
	std::cout << __FILE__ << ":"<< __LINE__ << " " <<\
	 __TIMESTAMP__ " : " << str << std::endl

// ����ģʽ
// �̰߳�ȫ�����̴߳����������̵߳��ã�����̰߳�ȫ����
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


