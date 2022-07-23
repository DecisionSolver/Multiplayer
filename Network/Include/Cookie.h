#pragma once
#include "pch.h"
class Cookie
{
private:
	static bool isEnable; // If It Is "False" Then Used Simple Login-Pass Pair!
public:
	struct Struct
	{
		Struct() = default;
		Struct(const std::string &Name, const std::string &Value, const std::string &Domain):
			Name(Name), Value(Value), Domain(Domain)
		{}
		Struct(const nlohmann::json &CookieToStruct):
			Name(CookieToStruct["Cookie"]["Name"]), Domain(CookieToStruct["Cookie"]["Domain"]),
			Value(CookieToStruct["Cookie"]["Value"])
		{}

		// Analogue With Cookies From HTTP
		std::string Name;
		std::string Value;

		// Domain
		std::string Domain;
	};

	// Checking If Some User Has Already Logged In This Server, If Not Send Login Form
	// And Then Make Cookie For Him
	std::string Make(const std::string &uniq_cookie, const std::string &Name, const std::string &Value,
		const std::string &Domain);

	// When Client Has Cookie He Needs To Read It Correctly Here And Then Proccess It Like JSON
	static nlohmann::json ReadCookie(const std::string & Cookie);

	// Check If Some Cookies Are Expired Then Erase It From Cookies
	void ProccessTime();

	// Add In "Cookies" Already Cooked Cookie
	std::string AddCookie(const std::string &uniq_cookie, const std::shared_ptr<Cookie::Struct> &OneCookie);

	bool FindCookie(const std::string &uniq_cookie);

	// Enable/Disable This System
	static void SetEnable(bool OnOff) { isEnable = OnOff; }
	static bool IsEnable() { return isEnable; }
private:
	std::map<std::string, std::shared_ptr<Cookie::Struct>> Cookies;
};