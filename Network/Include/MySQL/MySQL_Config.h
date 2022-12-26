#ifndef MYSQL_CONFIG_H
#define MYSQL_CONFIG_H

#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))

#include "mysql connector/include/jdbc/mysql_connection.h"
#include "mysql connector/include/jdbc/mysql_driver.h"
#include "mysql connector/include/jdbc/mysql_error.h"
#include "mysql connector/include/jdbc/cppconn/statement.h"
#include "nlohmann/json.hpp"

#endif // MYSQL_CONFIG_H
