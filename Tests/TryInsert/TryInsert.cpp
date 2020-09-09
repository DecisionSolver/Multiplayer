#include <iostream>
#include "MySQL_Client.h"
#include "MySQL_Impl.h"
#include <vector>
#include <string>
#include <memory>

using namespace std;

shared_ptr<mysql::MYSQLCLIENT> User;

int main()
{
	User = make_shared<mysql::MYSQLCLIENT>();
	User->Connect("gb_x_lolola32", "55b2zzada", "mysql101.1gb.ru", "gb_x_lolola32");
	
	//User->TryInsertValues("Local", { "Login", "Pass" },
	//	{ "PBAX", "_SUCKMYDICK_" });
	auto Obj = User->TrySelectValues("Local", { "Login", "Pass" }, " WHERE Login = 'HERE' AND Pass = 'AGAIN'");
	int I = 0;
	printf(("\nsize: " + to_string(Obj.size()) + "\n").c_str());
	for (size_t i = 0; i < Obj.size(); i++)
	{
		printf(("[" + to_string(i) + "] = " + Obj.at(i).first + "\n").c_str());

		for (auto It : Obj.at(i).second)
		{
			printf(("\t\t[" + to_string(I) + "] = " + It + "\n").c_str());
		}
	}
}