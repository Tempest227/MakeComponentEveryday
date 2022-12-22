#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>

const size_t TASK_MAX_THRESHOLD = 1024;
//////////////////////// 线程池方法实现
ThreadPool::ThreadPool()
	: m_initThreadSize(0)
	, m_taskSize(0)
	, m_taskQueMaxThreshold(TASK_MAX_THRESHOLD)
	, m_mode(PoolMode::MODE_FIXED)
	, m_isPoolRunning(false)
	, m_idleThreadSize(0)
{}

ThreadPool::~ThreadPool() {}

// 设置线程池工作模式
void ThreadPool::setMode(PoolMode mode) { if (checkPoolState)return; m_mode = mode; }
// 设置线程池任务队列上限阈值
void ThreadPool::setTaskQueThreshold(int threshold) { if (checkPoolState)return; m_taskQueMaxThreshold = threshold; }

// 给线程池提交任务
Result ThreadPool::submitTask(std::shared_ptr<Task> task) {
	// 获取锁
	std::unique_lock<std::mutex> lock(m_taskQueMtx);
	// 线程通信 等待任务队列有空余
	//m_notFull.wait(lock, [&]()->bool {return m_taskQue.size() < m_taskQueMaxThreshold; });
	if (!m_notFull.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return m_taskQue.size() < m_taskQueMaxThreshold; })) {
		// 等待 1s ，条件仍不满足
		std::cerr << "task queue is full, submit task fail." << std::endl;
		return Result(task, false);
	}
	// 有空余，放入任务队列中
	m_taskQue.emplace(task);
	m_taskSize++;
	// 队列不为空，在m_notEmpty上通知，分配线程处理
	m_notEmpty.notify_all();

	return Result(task);
}
// 开启线程池
void ThreadPool::start(int initThreadSize) {
	m_isPoolRunning = true;
	// 记录初始线程数量
	m_initThreadSize = initThreadSize;
	// 创建线程对象
	for (int i = 0; i < m_initThreadSize; i++) {
		std::unique_ptr<Thread> t(new Thread(std::bind(&ThreadPool::threadFunc, this)));
		m_threads.emplace_back(std::move(t));
	}

	// 启动所有线程
	for (int i = 0; i < m_initThreadSize; i++) {
		m_threads[i]->start(); // 执行线程函数
		m_idleThreadSize++; // 记录空闲线程数量
	}
}
// 线程函数，消费任务
void ThreadPool::threadFunc() {
	/*std::cout << "begin threadFunc tid: " << std::this_thread::get_id() << std::endl;
	std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;*/

	for (;;) {
		std::shared_ptr<Task> task;
		{
			//获取锁
			std::unique_lock<std::mutex> lock(m_taskQueMtx);
			
			//等待 m_notEmpty 条件
			m_notEmpty.wait(lock, [&]()->bool {return m_taskQue.size() > 0; });
			m_idleThreadSize--;

			// 取一个任务
			task = m_taskQue.front();
			m_taskQue.pop();
			m_taskSize--;

			// 如果依然有剩余任务，继续通知其它线程执行任务
			if (m_taskQue.size() > 0) m_notEmpty.notify_all();
			// 通知可以继续提交任务
			m_notFull.notify_all();

		}// 释放锁
		// 当前线程处理这个任务
		if (task != nullptr)
		{
			// task->run();
			// 执行任务，把任务的返回值通过setVal给Result
			task->exec();
		}
		m_idleThreadSize++;
	}
}

bool ThreadPool::checkPoolState() const {
	return m_isPoolRunning;
}
//////////////////////// 线程方法实现

Thread::Thread(ThreadFunc func) 
	: m_func(func) {}

Thread::~Thread() {}

void Thread::start() {
	// 创建线程来执行线程函数
	std::thread t(m_func);
	t.detach();
}

//////////////////////// 信号量实现

Semaphore::Semaphore(int limit) : m_limit(0) {}
Semaphore::~Semaphore() {}

void Semaphore::wait() {
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cond.wait(lock, [&]()->bool {return m_limit > 0;});
	m_limit--;
}

void Semaphore::post() {
	std::unique_lock<std::mutex> lock(m_mtx);
	m_limit++;
	m_cond.notify_all();
}

//////////////////////// Result实现
Result::Result(std::shared_ptr<Task> task, bool isValid) 
	: m_isValid(isValid) 
	, m_task(task) {
	m_task->setResult(this); 
}

Result::~Result(){}

Any Result::get() {
	if (!m_isValid) {
		return "";
	}
	m_sem.wait(); // task 没执行完，阻塞用户线程
	return std::move(m_any);
}

void Result::setVal(Any any) {
	m_any = std::move(any);
	m_sem.post();
}

//////////////////////// Task方法实现
Task::Task() : m_result(nullptr) {}
Task::~Task(){}

void Task::setResult(Result* result) {
	m_result = result;
}

void Task::exec() {
	if (m_result) { m_result->setVal(run()); }
}
