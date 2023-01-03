#include "connectionpool.h"
#include "iniparser.h"
#include <thread>
ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) return;

	for (int i = 0; i < m_initSize; i++) {
		Connection* conn = new Connection();
		conn->connect(m_ip, m_port, m_username, m_passwd, m_database);
		m_connectionQue.push(conn);
		m_connectionCnt++;
	}
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