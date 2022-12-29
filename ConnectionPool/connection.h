#pragma once
#include <mysql.h>
#include <string>
#include <ctime>
#include "public.h"

// 数据库操作类
class Connection
{
public:
	// 初始化数据库连接
	Connection();
	// 释放数据库连接资源
	~Connection();
	// 连接数据库
	bool connect(std::string ip, unsigned short port, std::string user, std::string password,
		std::string dbname);
	// 更新操作 insert、delete、update
	bool update(std::string sql);
	// 查询操作 select
	MYSQL_RES* query(std::string sql);
	clock_t getTime() const;
	void setTime(const clock_t time);
private:
	MYSQL* m_conn; // 表示和MySQL Server的一条连接
	clock_t m_time;
	
};