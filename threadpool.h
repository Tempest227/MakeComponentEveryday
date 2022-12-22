#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>


// Any 类型：可以接受任意类型数据
class Any {
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;
	// 接收任意类型数据
	template<typename T>
	Any(T data) : m_base(std::make_unique<Derive<T>>(data)) {}
	// 从基类指针提取 data

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

	// 问题1：setVal方法，获取任务执行完的返回值
	void setVal(Any any);
	// 问题2：get方法，用户调用这个方法获取 task 返回值
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

	// 启动线程
	void start();
private:
	ThreadFunc m_func;
	
};

class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();

	// 设置线程池工作模式
	void setMode(PoolMode mode); 
	// 设置线程池任务队列上限阈值
	void setTaskQueThreshold(int threshold);

	// 给线程池提交任务
	Result submitTask(std::shared_ptr<Task> task);
	// 开启线程池
	void start(int initThreadSize = 4);

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete; 
private:
	// 线程函数
	void threadFunc();
	bool checkPoolState() const;
public:
	std::vector<std::unique_ptr<Thread>> m_threads; // 线程列表
	size_t m_initThreadSize; // 初始线程数

	std::queue<std::shared_ptr<Task>> m_taskQue; // 任务队列
	std::atomic_int m_taskSize; // 任务的数量
	int m_taskQueMaxThreshold; // 任务队列上限阈值

	std::mutex m_taskQueMtx; // 保证任务队列的安全
	std::condition_variable m_notFull; // 保证任务队列不满
	std::condition_variable m_notEmpty; // 保证任务队列不空

	PoolMode m_mode; // 当前线程池模式
	std::atomic_bool m_isPoolRunning; // 当前线程池运行状态

	std::atomic_int m_idleThreadSize; // 记录空闲线程数量
};

#endif
