#include "connectionpool.h"
#include "iniparser.h"
#include <thread>
#include <functional>
#include <chrono>

std::shared_ptr<Connection> ConnectionPool::getConnection() {
	std::unique_lock<std::mutex> lock(m_queMtx);
	if (m_connectionQue.empty()) { 
		if (std::cv_status::no_timeout ==
			m_cond.wait_for(lock, std::chrono::milliseconds(m_connectionTimeout))
		   ) {
			if (m_connectionQue.empty()) {
				LOG("获取连接超时");
				return nullptr;
			}
		}
	}
	std::shared_ptr<Connection> pConnection(m_connectionQue.front(), [&](Connection* pCon) {
		std::unique_lock<std::mutex> lock(m_queMtx);
		pCon->refreshAliveTime();
		m_connectionQue.push(pCon);
		m_connectionCnt++;
		});
	m_connectionQue.pop();
	m_connectionCnt--;
	m_cond.notify_all();

	return pConnection;
}

void ConnectionPool::scanTask() {

	for (;;) {
		std::this_thread::sleep_for(std::chrono::seconds(m_maxIdleTime));
		std::unique_lock<std::mutex> lock(m_queMtx);
		while (m_connectionCnt > m_initSize) {
			Connection* conn = m_connectionQue.front();
			if (conn->getAliveTime() >= (m_maxIdleTime * 1000)) {
				m_connectionQue.pop();
				m_connectionCnt--;
				delete conn; 
			}
			else { break; }
		}
	}
}


void ConnectionPool::produceTask() {
	for (;;)
	{
		std::unique_lock<std::mutex> lock(m_queMtx);
		while (!m_connectionQue.empty()) { m_cond.wait(lock); }

		if (m_connectionCnt < m_maxSize) {
			Connection* conn = new Connection();
			conn->connect(m_ip, m_port, m_username, m_passwd, m_database);
			conn->refreshAliveTime();
			m_connectionQue.push(conn);
			m_connectionCnt++;
		}
		m_cond.notify_all();
	}
}

ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) return;
	// 创建初始连接
	for (int i = 0; i < m_initSize; i++) {
		Connection* conn = new Connection();
		conn->connect(m_ip, m_port, m_username, m_passwd, m_database);
		conn->refreshAliveTime();
		m_connectionQue.push(conn);
		m_connectionCnt++;
	}

	// 启动生产者线程
	std::thread produce(std::bind(&ConnectionPool::produceTask, this));
	produce.detach();

	// 启动扫描空闲连接线程(空闲时间超过 maxIdleTime)
	std::thread scan(std::bind(&ConnectionPool::scanTask, this));
	scan.detach(); 
}

bool ConnectionPool::loadConfigFile() {
	IniParser parser;
	if (!parser.load("mysqlconf.ini")) return false;

	Section& server = parser["server"];
	Section& profile = parser["profile"]; 
	Section& connection = parser["connection"];

	m_ip = server["ip"];
	m_port = server["port"];

	m_username = profile["username"];
	m_passwd = profile["password"];

	m_initSize = connection["initSize"];
	m_maxSize = connection["maxSize"];
	m_maxIdleTime = connection["maxIdleTime"];
	m_connectionTimeout = connection["connectionTimeout"];
	m_database = connection["database"];

	return true;
}