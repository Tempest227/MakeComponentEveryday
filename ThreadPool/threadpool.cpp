#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>

const size_t TASK_MAX_THRESHOLD = 1024;
const size_t THREAD_MAX_THRESHOLD = 12;
const size_t THREAD_MAX_IDLE_TIME = 60;

//////////////////////// 线程池方法实现

ThreadPool::ThreadPool()
	: m_initThreadSize(0)
	, m_threadMaxThreshold(THREAD_MAX_THRESHOLD)
	, m_curThreadSize(0)
	, m_taskSize(0)
	, m_taskQueMaxThreshold(TASK_MAX_THRESHOLD)
	, m_mode(PoolMode::MODE_FIXED)
	, m_isPoolRunning(false)
	, m_idleThreadSize(0)
{}

ThreadPool::~ThreadPool() {
	m_isPoolRunning = false;
	// 线程状态 ： 阻塞和执行任务
	m_notEmpty.notify_all();
	std::unique_lock<std::mutex> lock(m_taskQueMtx);
	m_exitCond.wait(lock, [&]()->bool { return m_curThreadSize == 0; });
}

// 设置线程池工作模式
void ThreadPool::setMode(PoolMode mode) { if (checkPoolState())return; m_mode = mode; }
// 设置线程池任务队列上限阈值
void ThreadPool::setTaskQueThreshold(int threshold) { if (checkPoolState())return; m_taskQueMaxThreshold = threshold; }
// 设置线程池CACHED模式下线程上限阈值
void ThreadPool::setThreadSizeThreshold(int threshold) { if (checkPoolState() || m_mode == PoolMode::MODE_FIXED)return; m_threadMaxThreshold = threshold; }
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
	// cached 模式： 使用场景：任务处理比较紧急并且任务小而快，需要根据任务数量和空闲线程数量，判断是否需要创建线程
	if (m_mode == PoolMode::MODE_CACHED
		&& m_taskSize > m_idleThreadSize
		&& m_curThreadSize < m_threadMaxThreshold) {
		// 创建新线程
		std::unique_ptr<Thread> t(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
		int id = t->getId();
		m_threads.emplace(id, std::move(t));
		// 启动新线程
		m_threads[id]->start();
		// 更新线程计数变量
		m_curThreadSize++;
		m_idleThreadSize++;
	}
	return Result(task);
}
// 开启线程池
void ThreadPool::start(int initThreadSize) {
	m_isPoolRunning = true;
	// 记录初始线程数量
	m_initThreadSize = initThreadSize;
	m_curThreadSize = m_initThreadSize;
	// 创建线程对象
	for (size_t i = 0; i < m_initThreadSize; i++) {
		std::unique_ptr<Thread> t(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
		//m_threads.emplace_back(std::move(t));
		m_threads.emplace(t->getId(), std::move(t));
	}

	// 启动所有线程
	for (int i = 0; i < m_initThreadSize; i++) {
		m_threads[i]->start(); // 执行线程函数
		m_idleThreadSize++; // 记录空闲线程数量
	}
}
// 线程函数，消费任务
void ThreadPool::threadFunc(int threadId) {
	/*std::cout << "begin threadFunc tid: " << std::this_thread::get_id() << std::endl;
	std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;*/
	auto lastTime = std::chrono::high_resolution_clock().now();
	while (m_isPoolRunning) {
		std::shared_ptr<Task> task;
		{
			// 获取锁
			std::unique_lock<std::mutex> lock(m_taskQueMtx);
			
			// cached 模式下，有可能创建很多线程，但空闲时间超过60s，应该把多余的线程回收
			// 当前时间 - 上一次线程执行的时间 > 60s
			
			// 怎么判断是超时返回，还是有任务待执行返回
			while (m_taskSize == 0) {
				if (m_mode == PoolMode::MODE_CACHED) {
					if (std::cv_status::timeout == m_notEmpty.wait_for(lock, std::chrono::seconds(1))) {
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);

						if (dur.count() > THREAD_MAX_IDLE_TIME && m_curThreadSize > m_idleThreadSize) {
							// 回收线程，修改记录线程的相关变量，把线程从线程列表中删除
							// 问题：无法确定线程函数对应的线程，进而无法删除线程列表中的线程
							m_threads.erase(threadId);
							m_curThreadSize--;
							m_idleThreadSize--;
							std::cout << "tid:" << std::this_thread::get_id() << "exit" << std::endl;
							return; // 线程函数执行完，线程结束
						}
					}
				}
				else {
					// 等待 m_notEmpty 条件
					m_notEmpty.wait(lock);
				}
				if (!m_isPoolRunning) {
					m_threads.erase(threadId);
					m_curThreadSize--;
					m_idleThreadSize--;
					m_exitCond.notify_all();
				}
			}

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
		lastTime = std::chrono::high_resolution_clock().now(); // 线程执行完任务的时间
	}
	m_threads.erase(threadId);
	m_exitCond.notify_all();

}

bool ThreadPool::checkPoolState() const {
	return m_isPoolRunning;
}
//////////////////////// 线程方法实现

int Thread::m_generateId = 0;

Thread::Thread(ThreadFunc func) 
	: m_func(func)
	, m_threadId(m_generateId++) {}

Thread::~Thread() {}

void Thread::start() {
	// 创建线程来执行线程函数
	std::thread t(m_func, m_threadId);
	t.detach();
}

int Thread::getId() const { return m_threadId; }

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
