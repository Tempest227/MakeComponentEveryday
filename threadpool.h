#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>


// Any ���ͣ����Խ���������������
class Any {
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;
	// ����������������
	template<typename T>
	Any(T data) : m_base(std::make_unique<Derive<T>>(data)) {}
	// �ӻ���ָ����ȡ data

	template<typename T>
	T cast() {
		Derive<T>* pd = dynamic_cast<Derive<T>*>(m_base.get());
		if (pd == nullptr) { throw "type is not matched!"; }
		return pd->m_data;
	}
private:
	class Base {
	public:
		virtual ~Base() = default;
	};

	template<typename T>
	class Derive : public Base {
	public:
		Derive(T& data) : m_data(data) {}
		T m_data;
	};
private:
	std::unique_ptr<Base> m_base;
};
class Result;
class Task {
public:
	Task();
	~Task();
	virtual Any run() = 0;
	void exec();
	void setResult(Result* result);
private:
	Result* m_result;
};

class Semaphore {
public:
	Semaphore(int limit = 0);
	~Semaphore();
	void wait();
	void post();
private:
	int m_limit;
	std::mutex m_mtx;
	std::condition_variable m_cond;
};

class Result {
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result();

	// ����1��setVal��������ȡ����ִ����ķ���ֵ
	void setVal(Any any);
	// ����2��get�������û��������������ȡ task ����ֵ
	Any get();
private:
	Any m_any;
	Semaphore m_sem;
	std::shared_ptr<Task> m_task;
	std::atomic_bool m_isValid;
};

enum class PoolMode {
	MODE_FIXED,
	MODE_CACHED
};

class Thread {
public:
	using ThreadFunc = std::function<void(void)>;
	Thread(ThreadFunc func);
	~Thread();

	// �����߳�
	void start();
private:
	ThreadFunc m_func;
	
};

class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();

	// �����̳߳ع���ģʽ
	void setMode(PoolMode mode); 
	// �����̳߳��������������ֵ
	void setTaskQueThreshold(int threshold);

	// ���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> task);
	// �����̳߳�
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete; 
private:
	// �̺߳���
	void threadFunc();
	bool checkPoolState() const;
public:
	std::vector<std::unique_ptr<Thread>> m_threads; // �߳��б�
	size_t m_initThreadSize; // ��ʼ�߳���

	std::queue<std::shared_ptr<Task>> m_taskQue; // �������
	std::atomic_int m_taskSize; // ���������
	int m_taskQueMaxThreshold; // �������������ֵ

	std::mutex m_taskQueMtx; // ��֤������еİ�ȫ
	std::condition_variable m_notFull; // ��֤������в���
	std::condition_variable m_notEmpty; // ��֤������в���

	PoolMode m_mode; // ��ǰ�̳߳�ģʽ
	std::atomic_bool m_isPoolRunning; // ��ǰ�̳߳�����״̬

	std::atomic_int m_idleThreadSize; // ��¼�����߳�����
};

#endif
