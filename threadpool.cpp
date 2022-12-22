#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>

const size_t TASK_MAX_THRESHOLD = 1024;
//////////////////////// �̳߳ط���ʵ��
ThreadPool::ThreadPool()
	: m_initThreadSize(0)
	, m_taskSize(0)
	, m_taskQueMaxThreshold(TASK_MAX_THRESHOLD)
	, m_mode(PoolMode::MODE_FIXED)
	, m_isPoolRunning(false)
	, m_idleThreadSize(0)
{}

ThreadPool::~ThreadPool() {}

// �����̳߳ع���ģʽ
void ThreadPool::setMode(PoolMode mode) { if (checkPoolState)return; m_mode = mode; }
// �����̳߳��������������ֵ
void ThreadPool::setTaskQueThreshold(int threshold) { if (checkPoolState)return; m_taskQueMaxThreshold = threshold; }

// ���̳߳��ύ����
Result ThreadPool::submitTask(std::shared_ptr<Task> task) {
	// ��ȡ��
	std::unique_lock<std::mutex> lock(m_taskQueMtx);
	// �߳�ͨ�� �ȴ���������п���
	//m_notFull.wait(lock, [&]()->bool {return m_taskQue.size() < m_taskQueMaxThreshold; });
	if (!m_notFull.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool {return m_taskQue.size() < m_taskQueMaxThreshold; })) {
		// �ȴ� 1s �������Բ�����
		std::cerr << "task queue is full, submit task fail." << std::endl;
		return Result(task, false);
	}
	// �п��࣬�������������
	m_taskQue.emplace(task);
	m_taskSize++;
	// ���в�Ϊ�գ���m_notEmpty��֪ͨ�������̴߳���
	m_notEmpty.notify_all();

	return Result(task);
}
// �����̳߳�
void ThreadPool::start(int initThreadSize) {
	m_isPoolRunning = true;
	// ��¼��ʼ�߳�����
	m_initThreadSize = initThreadSize;
	// �����̶߳���
	for (int i = 0; i < m_initThreadSize; i++) {
		std::unique_ptr<Thread> t(new Thread(std::bind(&ThreadPool::threadFunc, this)));
		m_threads.emplace_back(std::move(t));
	}

	// ���������߳�
	for (int i = 0; i < m_initThreadSize; i++) {
		m_threads[i]->start(); // ִ���̺߳���
		m_idleThreadSize++; // ��¼�����߳�����
	}
}
// �̺߳�������������
void ThreadPool::threadFunc() {
	/*std::cout << "begin threadFunc tid: " << std::this_thread::get_id() << std::endl;
	std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;*/

	for (;;) {
		std::shared_ptr<Task> task;
		{
			//��ȡ��
			std::unique_lock<std::mutex> lock(m_taskQueMtx);
			
			//�ȴ� m_notEmpty ����
			m_notEmpty.wait(lock, [&]()->bool {return m_taskQue.size() > 0; });
			m_idleThreadSize--;

			// ȡһ������
			task = m_taskQue.front();
			m_taskQue.pop();
			m_taskSize--;

			// �����Ȼ��ʣ�����񣬼���֪ͨ�����߳�ִ������
			if (m_taskQue.size() > 0) m_notEmpty.notify_all();
			// ֪ͨ���Լ����ύ����
			m_notFull.notify_all();

		}// �ͷ���
		// ��ǰ�̴߳����������
		if (task != nullptr)
		{
			// task->run();
			// ִ�����񣬰�����ķ���ֵͨ��setVal��Result
			task->exec();
		}
		m_idleThreadSize++;
	}
}

bool ThreadPool::checkPoolState() const {
	return m_isPoolRunning;
}
//////////////////////// �̷߳���ʵ��

Thread::Thread(ThreadFunc func) 
	: m_func(func) {}

Thread::~Thread() {}

void Thread::start() {
	// �����߳���ִ���̺߳���
	std::thread t(m_func);
	t.detach();
}

//////////////////////// �ź���ʵ��

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

//////////////////////// Resultʵ��
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
	m_sem.wait(); // task ûִ���꣬�����û��߳�
	return std::move(m_any);
}

void Result::setVal(Any any) {
	m_any = std::move(any);
	m_sem.post();
}

//////////////////////// Task����ʵ��
Task::Task() : m_result(nullptr) {}
Task::~Task(){}

void Task::setResult(Result* result) {
	m_result = result;
}

void Task::exec() {
	if (m_result) { m_result->setVal(run()); }
}
