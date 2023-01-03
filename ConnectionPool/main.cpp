#include "connection.h"
#include "public.h"
#include "iniparser.h"
#include "connectionpool.h"


int main()
{
	/*Connection conn;
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,age,sex) values('%s', %d, '%s')", "zhang san", 20, "male");
	conn.connect("127.0.0.1", 3306, "root", "123.com", "chat");
	conn.update(sql);*/

	/*Value v1;
	Value v2 = 1;
	Value v3 = 1.2;
	Value v4 = "123.com";
	Value v5 = std::string("1234");

	int i = v2;
	double d = v3;
	const char* s = v4;
	std::string str = v5;*/

	/*IniParser parser;
	parser.load("mysqlconf.ini");
	parser.show();*/
	//std::string s = "   123.co M sdfjl\nfjsdl\rlfjd  \r";
	//parser.trim(s);

	ConnectionPool* pool = Singleton<ConnectionPool>::getInstance();
	//pool->loadConfigFile();

	return 0;
}