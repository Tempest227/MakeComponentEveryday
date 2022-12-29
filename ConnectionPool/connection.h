#pragma once
#include <mysql.h>
#include <string>
#include <ctime>
#include "public.h"

// ���ݿ������
class Connection
{
public:
	// ��ʼ�����ݿ�����
	Connection();
	// �ͷ����ݿ�������Դ
	~Connection();
	// �������ݿ�
	bool connect(std::string ip, unsigned short port, std::string user, std::string password,
		std::string dbname);
	// ���²��� insert��delete��update
	bool update(std::string sql);
	// ��ѯ���� select
	MYSQL_RES* query(std::string sql);
	clock_t getTime() const;
	void setTime(const clock_t time);
private:
	MYSQL* m_conn; // ��ʾ��MySQL Server��һ������
	clock_t m_time;
	
};