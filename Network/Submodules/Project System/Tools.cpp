#include "Tools.h"

bool FindSubStr(const std::wstring &context, std::wstring const &from)
{
	if (context.empty() || from.empty())
		return false;

	if (context.find(from) != std::wstring::npos)
		//found
		return true;
	else
		//not found
		return false;
}

bool FindSubStr(const std::string &context, std::string const &from)
{
	if (context.empty() || from.empty())
		return false;

	if (context.find(from) != std::string::npos)
		//found
		return true;
	else
		//not found
		return false;
}

void replaceAll(std::wstring &context, std::wstring const &from, std::wstring const &to, bool OneTime, bool FindInEnd, bool AlsoDeleteSpace)
{
	size_t lookHere = FindInEnd ? context.length() : 0;
	size_t foundHere = 0;
	if (OneTime)
	{
		if ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}
	else
	{
		while ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), '	'), context.end());
}
void replaceAll(std::wstring &context, std::wstring const &from, std::wstring const &to, std::wstring const &also, bool OneTime,
	bool FindInEnd, bool AlsoDeleteSpace)
{
	size_t lookHere = FindInEnd ? context.length() : 0;
	size_t foundHere = 0;
	if (OneTime)
	{
		if ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}
	else
	{
		while ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}

	lookHere = FindInEnd ? context.length() : 0;
	foundHere = 0;
	if (OneTime)
	{
		if ((foundHere = FindInEnd ? context.rfind(also, lookHere) : context.find(also, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, also.size(), to);
			lookHere = foundHere + to.size();
		}
	}
	else
	{
		while ((foundHere = FindInEnd ? context.rfind(also, lookHere) : context.find(also, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, also.size(), to);
			lookHere = foundHere + to.size();
		}
	}

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), '	'), context.end());
}

void replaceAll(std::string &context, std::string const &from, std::string const &to, bool OneTime, bool FindInEnd, bool AlsoDeleteSpace)
{
	size_t lookHere = FindInEnd ? context.length() : 0;
	size_t foundHere = 0;
	if (OneTime)
	{
		if ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}
	else
	{
		while ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), '	'), context.end());
}
void replaceAll(std::string &context, std::string const &from, std::string const &to, std::string const &also, bool OneTime,
	bool FindInEnd, bool AlsoDeleteSpace)
{
	size_t lookHere = FindInEnd ? context.length() : 0;
	size_t foundHere = 0;
	if (OneTime)
	{
		if ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}
	else
	{
		while ((foundHere = FindInEnd ? context.rfind(from, lookHere) : context.find(from, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, from.size(), to);
			lookHere = foundHere + to.size();
		}
	}
	lookHere = FindInEnd ? context.length() : 0;
	foundHere = 0;
	if (OneTime)
	{
		if ((foundHere = FindInEnd ? context.rfind(also, lookHere) : context.find(also, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, also.size(), to);
			lookHere = foundHere + to.size();
		}
	}
	else
	{
		while ((foundHere = FindInEnd ? context.rfind(also, lookHere) : context.find(also, lookHere)) != std::string::npos)
		{
			context.replace(foundHere, also.size(), to);
			lookHere = foundHere + to.size();
		}
	}

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), '	'), context.end());
}

void deleteWord(std::string &context, std::string const &what, bool OneTime, bool FindInEnd, bool AlsoDeleteSpace)
{
	std::string::size_type pos = FindInEnd ? context.rfind(what.c_str()) : context.find(what.c_str());
	if (OneTime)
	{
		if (pos != std::string::npos)
		{
			context.erase(pos, what.length());
			pos = FindInEnd ? context.rfind(what.c_str(), pos + 1) : context.find(what.c_str(), pos + 1);
		}
	}
	else
	{
		while (pos != std::string::npos)
		{
			context.erase(pos, what.length());
			pos = FindInEnd ? context.rfind(what.c_str(), pos + 1) : context.find(what.c_str(), pos + 1);
		}
	}

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), '	'), context.end());
}

void deleteWord(std::string &context, char const what, char const OnWhat, bool AlsoDeleteSpace)
{
	for (size_t i = 0; i < context.length(); i++)
	{
		if (context.at(i) == what)
			context[i] = OnWhat;
	}

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), '	'), context.end());
}

void deleteWord(std::string &context, std::string const &start, std::string const &end, bool OneTime, bool AlsoDeleteSpace)
{
	if (OneTime)
	{
		size_t pos1 = 0, pos2 = std::string::npos;
		if ((pos1 = context.find(start, pos1)) != std::string::npos)
		{
			if ((pos2 = context.find(end, pos1)) != std::string::npos)
			{
				pos2 += end.length();
				context.erase(pos1, pos2 - pos1);
			}
		}
	}
	else
	{
		size_t pos1 = 0, pos2 = std::string::npos;
		for (;;)
		{
			if ((pos1 = context.find(start, pos1)) != std::string::npos)
			{
				if ((pos2 = context.find(end, pos1)) != std::string::npos)
				{
					pos2 += end.length();
					context.erase(pos1, pos2 - pos1);
				}
				else break;
			}
			else break;
		}
	}

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), '	'), context.end());
}

void deleteWord(std::string &context, std::string const &start, ModeProcessString const mode, bool FindInEnd, bool AlsoDeleteSpace)
{
	if (mode == ModeProcessString::UntilTheEnd)
		if (context.find(start) != std::string::npos)
			context.erase(context.find(start), context.length());

	if (mode == ModeProcessString::UntilTheBegin)
		if (FindInEnd ? context.rfind(start) : context.find(start) /* with start char */ != std::string::npos)
			context.erase(0, FindInEnd ? context.rfind(start) : context.find(start));

	if (AlsoDeleteSpace)
		context.erase(remove(context.begin(), context.end(), ' '), context.end());
}

#include <iomanip>
#include <boost/algorithm/string.hpp>
void getFloat3Text(const std::string &context, const std::string &Char2Split, std::vector<float> &Float3)
{
	std::vector<std::string> Result;
	boost::split(Result, context, boost::is_any_of(Char2Split));
	for (size_t i = 0; i < Result.size(); i++)
	{
		std::string s = Result[i];
		boost::trim_right(s);
		float ret = 0.f;
		std::stringstream stream;
		stream << std::fixed << std::setprecision(3) << s;
		stream >> ret;
		Float3.push_back(ret);
	}
}

void getTextFloat3(std::string &context, const std::string &Char2Split, const std::vector<float> &Float3)
{
	std::stringstream stream;
	for (size_t i = 0; i < Float3.size(); i++)
	{
		stream << std::fixed << std::setprecision(3) << Float3[i];
		context += i == 0 ? stream.str() : Char2Split + stream.str();
		stream.str({});
	}
}

nlohmann::json XMLtoJSON(tinyxml2::XMLElement *node)
{
	if (!node) return nlohmann::json();
	nlohmann::json Result = {};

	for (;;)
	{
		nlohmann::json j = {}, Args = {};
		std::string key = node->Name();

		// iterate through the attributes
		for (tinyxml2::XMLAttribute *FirstAttr = const_cast<tinyxml2::XMLAttribute *>(node->FirstAttribute());
			FirstAttr; FirstAttr = const_cast<tinyxml2::XMLAttribute *>(FirstAttr->Next()))
		{
			Args[key][FirstAttr->Name()] = FirstAttr->Value();
		}

		if (!node->NoChildren())
		{
			if (j.find(key) == j.end())
				j[key] = XMLtoJSON(node->FirstChildElement());
			else
				j.back() = XMLtoJSON(node->FirstChildElement());
		}
		else
			j[key] = {};

		if (!Args.empty())
		{
			for (auto _j = Args.begin(); _j != Args.end(); ++_j)
			{
				j[_j.key()] = _j.value();
			}
		}

		Result.push_back(j);
		if (node->NextSiblingElement())
			node = node->NextSiblingElement();
		else
			break;
	}

	return Result;
}
tinyxml2::XMLDocument *JSONtoXML(nlohmann::json js)
{
	tinyxml2::XMLDocument *Doc = new tinyxml2::XMLDocument();

	for (nlohmann::json::iterator it = js.begin(); it != js.end(); ++it)
	{
		Doc->InsertEndChild(Doc->NewElement(it.key().c_str()));

		if ((js.is_array() || js.is_object()) && !it.value().is_null())
		{
			for (auto const &_it: it.value().get<nlohmann::json::array_t>())
			{
				if (_it.is_object())
				{
					for (auto const &obj: _it.get<nlohmann::json::object_t>())
					{
						// Add Main Node
						auto ThisNode = Doc->FirstChildElement()->InsertEndChild(Doc->NewElement(obj.first.c_str()));
						if (obj.second.is_array())
						{
							for (auto const &nodes: obj.second.get<nlohmann::json::array_t>())
							{
								if (nodes.is_object())
								{
									for (auto const &node: nodes.get<nlohmann::json::object_t>())
									{
										ThisNode = ThisNode->InsertEndChild(Doc->NewElement(node.first.c_str()));

										if (node.second.is_object())
										{
											// Add Attributes
											for (auto const &attr: node.second.get<nlohmann::json::object_t>())
											{
												if (attr.second.is_string())
													ThisNode->ToElement()->SetAttribute(attr.first.c_str(),
														attr.second.get<nlohmann::json::string_t>().c_str());
											}

											ThisNode = ThisNode->Parent();
										}
									}
								}
							}
						}
						else if (obj.second.is_object())
						{
							// Add Attributes
							for (auto const &attr: obj.second.get<nlohmann::json::object_t>())
							{
								if (attr.second.is_string())
									ThisNode->ToElement()->SetAttribute(attr.first.c_str(),
										attr.second.get<nlohmann::json::string_t>().c_str());
							}
						}
					}
				}
			}
		}
	}

	return Doc;
}

float random_floats(float min, float max)
{
	return (min + 1) + (((float)rand()) / (float)RAND_MAX) * (max - (min + 1));
}
