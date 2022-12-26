#include "Cookie.h"
#include "SFML/System/Time.hpp"
#include "SFML/System/Clock.hpp"

#include "cryptopp/gzip.h"

bool Cookie::isEnable = true;

// By Default It's 4 Mins
sf::Time TimeToGiveUp = sf::seconds(240.f);

sf::Clock _clock; // starts the clock

std::string Cookie::Make(const std::string &uniq_cookie, const std::string &Name, const std::string &Value,
	const std::string &Domain)
{
	if (!isEnable) return "";

	auto it = Cookies.find(uniq_cookie);
	if (it == Cookies.end()) // Wasn't Found
	{
		Cookies.insert({ uniq_cookie, std::make_shared<Cookie::Struct>(Name, Value, Domain) });
	}

	auto Crypted = nlohmann::json::to_ubjson({ Name, { Value, Domain } });

	std::string msg = reinterpret_cast<char *>(Crypted.data());
	msg.resize(Crypted.size());

	return String2HEX(msg);
}

nlohmann::json Cookie::ReadCookie(const std::string &Cookie)
{
	if (!isEnable)
	{
		return nlohmann::json();
	}
	return nlohmann::json::from_ubjson(HEX2String(Cookie));
}

sf::Time elapsed = sf::Time();

void Cookie::ProccessTime()
{
	/*
	if (!isEnable) return;

	for (std::vector<std::shared_ptr<Cookie::Struct>>::iterator it = Cookies.begin(); it != Cookies.end(); )
	{
		elapsed += clock.getElapsedTime();
		if (elapsed.asSeconds() > TimeToGiveUp.asSeconds())
		{
			clock.restart();
			elapsed = sf::Time();
			it = Cookies.erase(it);
		}
	}
	clock.restart();
	*/
}

std::string Cookie::AddCookie(const std::string &uniq_cookie, const std::shared_ptr<Cookie::Struct> &OneCookie)
{
	if (uniq_cookie.empty() || !OneCookie)
	{
		return "";
	}

	Cookies.insert({ uniq_cookie, OneCookie });

	nlohmann::json BuildCookie;
	BuildCookie["Cookie"] = { { "Unique_ID", uniq_cookie }, {"Name", OneCookie->Name }, { "Domain", OneCookie->Domain },
	{ "Value", OneCookie->Value } };

	auto Crypted = nlohmann::json::to_ubjson(BuildCookie);

	std::string msg = reinterpret_cast<char *>(Crypted.data());
	msg.resize(Crypted.size());

	return String2HEX(msg);
}

bool Cookie::FindCookie(const std::string &uniq_cookie)
{
	if (uniq_cookie.empty()) return false;

	auto it = Cookies.find(uniq_cookie);
	if (it == Cookies.end()) // Wasn't Found
	{
		return false;
	}
	else
	{
		return true;
	}
	return false;
}