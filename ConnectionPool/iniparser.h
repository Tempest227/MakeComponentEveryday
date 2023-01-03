#pragma once
#include <string>
#include <unordered_map>

class Value {
	friend std::ostream& operator << (std::ostream& os,const Value& v);
public:
	Value();
	Value(bool value);
	Value(int value);
	Value(double value);
	Value(const char* value);
	Value(const std::string& value);

	operator bool();
	operator int();
	operator double();
	operator std::string();

private:
	std::string m_value;
};


typedef std::unordered_map<std::string, Value> Section;
typedef std::unordered_map<std::string, Section> sections;


class IniParser {
public:
	IniParser();
	bool load(const std::string& filename = "");
	void show();
	Section& operator[](const std::string key);
private:
	void trim(std::string& str);
private:
	sections m_sections;
};