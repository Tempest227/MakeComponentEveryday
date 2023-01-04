#pragma once
#include "public.h"
#include "connection.h"
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>


class ConnectionPool {
	friend class Singleton<ConnectionPool>;
public:
	std::shared_ptr<Connection> getConnection();
private:
	bool loadConfigFile();

	void produceTask();
	void scanTask();
private:
	ConnectionPool();
	ConnectionPool(const ConnectionPool&) = delete;
	ConnectionPool(const ConnectionPool&&) = delete;
	ConnectionPool& operator=(const ConnectionPool&) = delete;
private:
	std::string m_ip;
	int m_port;
	std::string m_username;
	std::string m_passwd;
	int m_initSize;
	int m_maxSize;
	int m_maxIdleTime;
	int m_connectionTimeout;
	std::string m_database;

	std::queue<Connection*> m_connectionQue;
	std::mutex m_queMtx;
	std::atomic_int m_connectionCnt;
	std::condition_variable m_cond;
};