#pragma once
#include "public.h"
#include "connection.h"
#include <queue>
#include <mutex>
#include <atomic>

class ConnectionPool {
	friend class Singleton<ConnectionPool>;
private:
	bool loadConfigFile();
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
};