#include "iniparser.h"
#include <sstream>
#include <fstream>
#include <iostream>

std::ostream& operator << (std::ostream& os, const Value& v) { os << v.m_value; return os; }


Value::Value() {}
Value::Value(bool value) { m_value = value == true ? "true" : "false"; }
Value::Value(int value) { std::stringstream ss; ss << value; m_value = ss.str(); }
Value::Value(double value) { std::stringstream ss; ss << value; m_value = ss.str(); }
Value::Value(const char* value) { m_value = value; }
Value::Value(const std::string& value) { m_value = value; }

Value::operator bool() { return "true" == m_value; }
Value::operator int() { return std::stoi(m_value); }
Value::operator double() { return std::stod(m_value); }
Value::operator std::string() { return m_value; }

Section& IniParser::operator[](const std::string key) { return m_sections[key]; }

IniParser::IniParser(){}

bool IniParser::load(const std::string& filename) {
	std::ifstream ifs(filename);
	if (ifs.fail()) return false;
	std::string line;
	std::string title;
	while (std::getline(ifs, line)) {
		if (line.empty()) continue;
		else if (line[0] == '[') {
			auto pos = line.find_last_of(']');
			title = line.substr(1, pos - 1);
			trim(title);
		}
		else {
			auto pos = line.find('=');
			auto key = line.substr(0, pos);
			trim(key);
			auto value = line.substr(pos + 1);
			trim(value);
			m_sections[title][key] = value;
		}
	}
	return true;
}

void IniParser::trim(std::string& str) {
	auto pos = str.find_first_not_of(" \n\r");
	str.erase(0, pos);
	pos = str.find_last_not_of(" \n\r");
	str.erase(pos + 1);
}

void IniParser::show() {
	std::stringstream ss;
	for (auto section = m_sections.begin(); section != m_sections.end(); section++) {
		ss << '[' << section->first << ']' << std::endl;
		for (auto it = section->second.begin(); it != section->second.end(); it++) {
			ss << it->first << '=' << it->second << std::endl;
		}
	}
	std::cout << ss.str();
}


