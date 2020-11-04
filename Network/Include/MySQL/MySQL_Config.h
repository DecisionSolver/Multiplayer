#ifndef MYSQL_CONFIG_H
#define MYSQL_CONFIG_H

#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))

//////////////////////////////////////////
// #pragma			    				//
//////////////////////////////////////////
										//
#pragma warning(disable: 4996)			//
										//
//////////////////////////////////////////
// Headers			    				//
//////////////////////////////////////////
										//
#include "mysql_connection.h"			//
#include "mysql_driver.h"				//
#include "mysql_error.h"				//
#include "cppconn/statement.h"			//
#include "nlohmann/json.hpp"			//
										//
//////////////////////////////////////////


#endif // MYSQL_CONFIG_H
