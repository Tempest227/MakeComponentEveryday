#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>
#include <iostream>

const size_t TASK_MAX_THRESHOLD = 1024;
const size_t THREAD_MAX_THRESHOLD = 12;
const size_t THREAD_MAX_IDLE_TIME = 60;
 
enum class PoolMode {
	MODE_FIXED,
	MODE_CACHED
};

class Thread {
public:
	using ThreadFunc = std::function<void(int)>;
	Thread(ThreadFunc func)
		: m_func(func)
		, m_threadId(m_generateId++) {}
	
	~Thread() = default;

	// �����߳�
	void start() {// �����߳���ִ���̺߳���
		std::thread t(m_func, m_threadId);
		t.detach();
	}
	// ��ȡ�߳� ID
	int getId() const { return m_threadId; }
private:
	ThreadFunc m_func;
	static int m_generateId;
	int m_threadId; // �����߳� ID

};

class ThreadPool {
public:
	ThreadPool()
		: m_initThreadSize(0)
		, m_threadMaxThreshold(THREAD_MAX_THRESHOLD)
		, m_curThreadSize(0)
		, m_taskSize(0)
		, m_taskQueMaxThreshold(TASK_MAX_THRESHOLD)
		, m_mode(PoolMode::MODE_FIXED)
		, m_isPoolRunning(false)
		, m_idleThreadSize(0)
	{}
	~ThreadPool() {
		m_isPoolRunning = false;
		// �߳�״̬ �� ������ִ������

		std::unique_lock<std::mutex> lock(m_taskQueMtx);
		m_notEmpty.notify_all();
		m_exitCond.wait(lock, [&]()->bool { return m_curThreadSize == 0; });
	}

	// �����̳߳ع���ģʽ
	void setMode(PoolMode mode) { if (checkPoolState() || m_mode == PoolMode::MODE_FIXED)return; m_mode = mode; }
	// �����̳߳��������������ֵ
	void setTaskQueThreshold(int threshold) { if (checkPoolState() || m_mode == PoolMode::MODE_FIXED)return; m_taskQueMaxThreshold = threshold; }
	// �����̳߳�CACHEDģʽ���߳�������ֵ
	void setThreadSizeThreshold(int threshold) { if (checkPoolState() || m_mode == PoolMode::MODE_FIXED)return; m_threadMaxThreshold = threshold; }
	// ���̳߳��ύ����
	// ʹ�ÿɱ��ģ���̣��� submitTask ���Խ������������������������Ĳ���
	//Result submitTask(std::shared_ptr<Task> task);
	template<typename Func, typename... Args>
	auto submitTask(Func&& func, Args&&... args)->std::future<decltype(func(args...))> {
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType>>(std::bind(std::forward(func)), std::bind(std::forward(args...)));
		std::future<RType> result = task->get_future();

		std::unique_lock<std::mutex> lock(m_taskQueMtx);
		// �߳�ͨ�� �ȴ���������п���
		if (!m_notFull.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool {return m_taskQue.size() < m_taskQueMaxThreshold; })) {
			// �ȴ� 1s �������Բ�����
			std::cerr << "task queue is full, submit task fail." << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>([]()->RType {return RType()});
			(*task)();
			return task->get_future();
		}
		// �п��࣬�������������
		m_taskQue.emplace([task]()->{ (*task)(); });
		m_taskSize++;
		// ���в�Ϊ�գ���m_notEmpty��֪ͨ�������̴߳���
		m_notEmpty.notify_all();
		// cached ģʽ�� ʹ�ó�����������ȽϽ�����������С���죬��Ҫ�������������Ϳ����߳��������ж��Ƿ���Ҫ�����߳�
		if (m_mode == PoolMode::MODE_CACHED
			&& m_taskSize > m_idleThreadSize
			&& m_curThreadSize < m_threadMaxThreshold) {
			// �������߳�
			std::unique_ptr<Thread> t(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
			int id = t->getId();
			m_threads.emplace(id, std::move(t));
			// �������߳�
			m_threads[id]->start();
			// �����̼߳�������
			m_curThreadSize++;
			m_idleThreadSize++;
		}
		return result;
	}
	// �����̳߳�
	void start(int initThreadSize = std::thread::hardware_concurrency()) {
		m_isPoolRunning = true;
		// ��¼��ʼ�߳�����
		m_initThreadSize = initThreadSize;
		m_curThreadSize = m_initThreadSize;
		// �����̶߳���
		for (size_t i = 0; i < m_initThreadSize; i++) {
			std::unique_ptr<Thread> t(new Thread(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1)));
			//m_threads.emplace_back(std::move(t));
			m_threads.emplace(t->getId(), std::move(t));
		}

		// ���������߳�
		for (int i = 0; i < m_initThreadSize; i++) {
			m_threads[i]->start(); // ִ���̺߳���
			m_idleThreadSize++; // ��¼�����߳�����
		}
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	// �̺߳���
	void threadFunc(int threadId) {
		auto lastTime = std::chrono::high_resolution_clock().now();
		// �����������ȫ��ִ����ɣ��̳߳زŻ���
		for (;;) {
			Task task;
			{
				// ��ȡ��
				std::unique_lock<std::mutex> lock(m_taskQueMtx);

				// cached ģʽ�£��п��ܴ����ܶ��̣߳�������ʱ�䳬��60s��Ӧ�ðѶ�����̻߳���
				// ��ǰʱ�� - ��һ���߳�ִ�е�ʱ�� > 60s

				// ��ô�ж��ǳ�ʱ���أ������������ִ�з���
				while (m_taskSize == 0) {
					// �̳߳ؽ����������߳���Դ
					if (!m_isPoolRunning) {
						m_threads.erase(threadId);
						m_exitCond.notify_all();
						return;
					}
					if (m_mode == PoolMode::MODE_CACHED) {
						if (std::cv_status::timeout == m_notEmpty.wait_for(lock, std::chrono::seconds(1))) {
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);

							if (dur.count() > THREAD_MAX_IDLE_TIME && m_curThreadSize > m_idleThreadSize) {
								// �����̣߳��޸ļ�¼�̵߳���ر��������̴߳��߳��б���ɾ��
								// ���⣺�޷�ȷ���̺߳�����Ӧ���̣߳������޷�ɾ���߳��б��е��߳�
								m_threads.erase(threadId);
								m_curThreadSize--;
								m_idleThreadSize--;
								std::cout << "tid:" << std::this_thread::get_id() << "exit" << std::endl;
								return; // �̺߳���ִ���꣬�߳̽���
							}
						}
					}
					else {
						// �ȴ� m_notEmpty ����
						m_notEmpty.wait(lock);
					}
				}

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
				task();
			}
			m_idleThreadSize++;
			lastTime = std::chrono::high_resolution_clock().now(); // �߳�ִ���������ʱ��
		}
	}
	bool checkPoolState() const { return m_isPoolRunning; }
public:
	//std::vector<std::unique_ptr<Thread>> m_threads; // �߳��б�
	std::unordered_map<int, std::unique_ptr<Thread>> m_threads;
	size_t m_initThreadSize; // ��ʼ�߳���
	int m_threadMaxThreshold; // �߳�������ֵ
	std::atomic_size_t m_curThreadSize; // ʵʱ�߳���
	std::atomic_int m_idleThreadSize; // ��¼�����߳�����

	using Task = std::function<void()>;
	std::queue<Task> m_taskQue; // �������
	std::atomic_int m_taskSize; // ���������
	int m_taskQueMaxThreshold; // �������������ֵ

	std::mutex m_taskQueMtx; // ��֤������еİ�ȫ
	std::condition_variable m_notFull; // ��֤������в���
	std::condition_variable m_notEmpty; // ��֤������в���
	std::condition_variable m_exitCond; // �ȴ��߳���Դȫ������

	PoolMode m_mode; // ��ǰ�̳߳�ģʽ
	std::atomic_bool m_isPoolRunning; // ��ǰ�̳߳�����״̬


};


int Thread::m_generateId = 0;
#endif

