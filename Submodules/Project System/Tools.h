#pragma once
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "../Include/pch.h"

/**
 * \def	ToDo(desc)
 *
 * \brief	A macro that defines to do
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param 	desc	Text
 */

#define MacroStr(x) #x
#define MacroStr2(x) MacroStr(x)
#define ToDo(desc) __pragma(message(__FILE__ "(" MacroStr2(__LINE__) "): " #desc))

#if !defined (SAFE_DELETE)

	/**
	 * \def	SAFE_DELETE(p)
	 *
	 * \brief	A macro that defines safe delete
	 *
	 * \author	PBAX
	 * \date	17.02.2020
	 *
	 * \param 	p	A void to process.
	 */

	#define SAFE_DELETE(p) { if (p) { delete (p); (p) = nullptr; } }
#endif
#if !defined (SAFE_DELETE_ARRAY)

	/**
	 * \def	SAFE_DELETE_ARRAY(p)
	 *
	 * \brief	A macro that defines safe delete array
	 *
	 * \author	PBAX
	 * \date	17.02.2020
	 *
	 * \param 	p	A void to process.
	 */

	#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p); (p) = nullptr; } }
#endif
#if !defined (SAFE_RELEASE)

	/**
	 * \def	SAFE_RELEASE(p)
	 *
	 * \brief	A macro that defines safe Release
	 *
	 * \author	PBAX
	 * \date	17.02.2020
	 *
	 * \param 	p	A void to process.
	 */

	#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

#if !defined (SAFE_release)

/**
 * \def	SAFE_release(p)
 *
 * \brief	A macro that defines safe release
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param 	p	A void to process.
 */

	#define SAFE_release(p) { if (p) { (p)->release(); (p) = nullptr; } }
#endif

/*
 * \fn	bool FindSubStr(std::wstring context, std::wstring const from);
 *
 * \brief	Searches for the first sub Wstring
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param 	context	The context.
 * \param 	from   	Source for the.
 *
 * \returns	True if it succeeds, false if it fails.
 */

bool FindSubStr(const std::wstring &context, std::wstring const &from);

/**
 * \fn	bool FindSubStr(std::string context, std::string const from);
 *
 * \brief	Searches for the first sub Astring
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param 	context	The context.
 * \param 	from   	Source for the.
 *
 * \returns	True if it succeeds, false if it fails.
 */

bool FindSubStr(const std::string &context, std::string const &from);

/**
 * \fn	void replaceAll(std::wstring &context, std::wstring const &from, std::wstring const &to, bool OneTime = false,
 * 		 bool FindInEnd = false, bool AlsoDeleteSpace = false);
 *
 * \brief	Replace all (Astring)
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	from		   	Source for the.
 * \param 		  	to			   	to.
 * \param 		  	OneTime		   	(Optional) True to one time.
 * \param 		  	FindInEnd	   	(Optional) True to find in end.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void replaceAll(std::wstring &context, std::wstring const &from, std::wstring const &to, bool OneTime = false,
	bool FindInEnd = false, bool AlsoDeleteSpace = false);

/**
 * \fn	void replaceAll(std::wstring &context, std::wstring const &from, std::wstring const &to, std::wstring const &also,
 * 		 bool OneTime = false, bool FindInEnd = false, bool AlsoDeleteSpace = false);
 *
 * \brief	Replace all (Wstring)
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	from		   	Source for the.
 * \param 		  	to			   	to.
 * \param 		  	also		   	The also.
 * \param 		  	OneTime		   	(Optional) True to one time.
 * \param 		  	FindInEnd	   	(Optional) True to find in end.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void replaceAll(std::wstring &context, std::wstring const &from, std::wstring const &to, std::wstring const &also,
	bool OneTime = false, bool FindInEnd = false, bool AlsoDeleteSpace = false);

/**
 * \fn	void replaceAll(std::string &context, std::string const &from, std::string const &to, bool OneTime = false,
 * 		 bool FindInEnd = false, bool AlsoDeleteSpace = false);
 *
 * \brief	Replace all
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	from		   	Source for the.
 * \param 		  	to			   	to.
 * \param 		  	OneTime		   	(Optional) True to one time.
 * \param 		  	FindInEnd	   	(Optional) True to find in end.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void replaceAll(std::string &context, std::string const &from, std::string const &to, bool OneTime = false,
	bool FindInEnd = false, bool AlsoDeleteSpace = false);

/**
 * \fn	void replaceAll(std::string &context, std::string const &from, std::string const &to, std::string const &also,
 * 		 bool OneTime = false, bool FindInEnd = false, bool AlsoDeleteSpace = false);
 *
 * \brief	Replace all
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	from		   	Source for the.
 * \param 		  	to			   	to.
 * \param 		  	also		   	The also.
 * \param 		  	OneTime		   	(Optional) True to one time.
 * \param 		  	FindInEnd	   	(Optional) True to find in end.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void replaceAll(std::string &context, std::string const &from, std::string const &to, std::string const &also, bool OneTime = false,
	bool FindInEnd = false, bool AlsoDeleteSpace = false);

/**
 * \fn	void deleteWord(std::string &context, std::string const &what, bool OneTime = false,
 * 		 bool FindInEnd = false, bool AlsoDeleteSpace = false);
 *
 * \brief	Deletes the word
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	what		   	The what.
 * \param 		  	OneTime		   	(Optional) True to one time.
 * \param 		  	FindInEnd	   	(Optional) True to find in end.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void deleteWord(std::string &context, std::string const &what, bool OneTime = false, bool FindInEnd = false,
	bool AlsoDeleteSpace = false);

/**
 * \fn	void deleteWord(std::string &context, char const what, char const OnWhat, bool AlsoDeleteSpace = false);
 *
 * \brief	Deletes the word
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	what		   	The what.
 * \param 		  	OnWhat		   	The on what.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void deleteWord(std::string &context, char const what, char const OnWhat, bool AlsoDeleteSpace = false);

/**
 * \fn	void deleteWord(std::string &context, std::string const start, std::string const end,
 * 		 bool OneTime = false, bool AlsoDeleteSpace = true);
 *
 * \brief	Deletes the word
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	start		   	The start.
 * \param 		  	end			   	The end.
 * \param 		  	OneTime		   	(Optional) True to one time.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void deleteWord(std::string &context, std::string const &start, std::string const &end, bool OneTime = false,
	bool AlsoDeleteSpace = true);

/**
 * \enum	ModeProcessString
 *
 * \brief	Values that represent mode for deleting Word Function
 */

enum ModeProcessString { UntilTheBegin = 0, UntilTheEnd };

/**
 * \fn	void deleteWord(std::string &context, std::string const start, ModeProcessString const mode,
 * 		 bool FindInEnd = false, bool AlsoDeleteSpace = true);
 *
 * \brief	Deletes the word
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context		   	The context.
 * \param 		  	start		   	The start.
 * \param 		  	mode		   	The mode.
 * \param 		  	FindInEnd	   	(Optional) True to find in end.
 * \param 		  	AlsoDeleteSpace	(Optional) True to also delete space.
 */

void deleteWord(std::string &context, std::string const &start, ModeProcessString const mode, bool FindInEnd = false,
	bool AlsoDeleteSpace = true);

/**
 * \fn	void getFloat3Text(std::string context, std::string Char2Split, vector<float> &Float3);
 *
 * \brief	Function That Converts From String To Vector3 (Massive)
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param 		  	context   	Source std::string
 * \param 		  	Char2Split	Char that will using in split
 * \param [in,out]	Float3	  	Return MASSIVE converted coords
 */

void getFloat3Text(const std::string &context, const std::string &Char2Split, std::vector<float> &Float3);

/**
 * \fn	void getTextFloat3(std::string &context, std::string Char2Split, vector<float> Float3);
 *
 * \brief	Function That Converts Vector3 (Massive) To String
 *
 * \author	PBAX
 * \date	17.02.2020
 *
 * \param [in,out]	context   	Return STRING that converted coords.
 * \param 		  	Char2Split	Char that will using in split.
 * \param 		  	Float3	  	Massive that will using for convert coords
 */

void getTextFloat3(std::string &context, const std::string &Char2Split, const std::vector<float> &Float3);


#include "tinyxml2.h"
using namespace tinyxml2;

#include "nlohmann/json.hpp"
nlohmann::json XMLtoJSON(tinyxml2::XMLElement *node);
tinyxml2::XMLDocument *JSONtoXML(nlohmann::json js);

float random_floats(float min, float max);
#endif // !__TOOLS_H__

#define Vector3 std::vector<float>
