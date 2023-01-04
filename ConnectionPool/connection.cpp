#include <mysql.h>
#include <string>

#include "connection.h"
#include "connectionpool.h"

// 初始化数据库连接

Connection::Connection() {
	m_conn = mysql_init(nullptr);
}
// 释放数据库连接资源
Connection::~Connection() {
	if (m_conn != nullptr) mysql_close(m_conn);
}
// 连接数据库
bool Connection::connect(std::string ip, unsigned short port, std::string user, std::string password,
	std::string dbname) {
	MYSQL* p = mysql_real_connect(m_conn, ip.c_str(), user.c_str(),
		password.c_str(), dbname.c_str(), port, nullptr, 0);
	return p != nullptr;
}
// 更新操作 insert、delete、update
bool Connection::update(std::string sql) {
	if (mysql_query(m_conn, sql.c_str())) {
		LOG("更新失败:" + sql);
		return false;
	}
	return true;
}
// 查询操作 select
MYSQL_RES* Connection::query(std::string sql) {
	if (mysql_query(m_conn, sql.c_str())) {
		LOG("查询失败:" + sql);
		return nullptr;
	}
	return mysql_use_result(m_conn);
}

time_t Connection::getAliveTime() const { return time(0) - m_aliveTime; }

void Connection::refreshAliveTime() { m_aliveTime = time(0); }