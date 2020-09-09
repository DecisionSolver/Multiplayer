#ifndef MYSQL_CONFIG_H
#define MYSQL_CONFIG_H

#if defined (_DEBUG)
#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))
#endif

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
										//
//////////////////////////////////////////


#endif // MYSQL_CONFIG_H
