#pragma once
#if !defined(__PROJECT_SYSTEM_H_)
#define __PROJECT_SYSTEM_H_

#if __has_include("Core/pch.h")
	#include "Core/pch.h"
#else
	#include "../Tools.h"
#endif

#include "ODBC/ODBC.h"
#include "Packet.hpp"
#include "Client.hpp"
#include <winnt.h>

/**
* \struct	ProjectFile
*
* \brief	Class Project File For SDK.
*
* \author	PBAX
* \date	05.08.2021
*/

class Level;
class ProjectFile
{
public:
	ProjectFile() {}
	~ProjectFile() {}

	void CreateProject(const std::string &Name);

	static void OpenOrCreateDB(bool ifNeedConnect = true);

	/**
	 * \fn	ProjectFile(const path &CurrProj)
	 *
	 * \brief	Constructor (Path To Current Project)
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	CurrProj	The curr project.
	 */

	ProjectFile(const std::string &CurrProj) { SetCurProject(CurrProj); }

	/**
	 * \fn	auto GetCurrentProject()
	 *
	 * \brief	Gets current project
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \returns	The current project.
	 */

	auto GetCurrentProject() { return CurrentProj; }

	/**
	 * \fn	void Open(const string &Name);
	 *
	 * \brief	Open Project (Table) From DB
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	file	The file.
	 */

	// Name - Project From All_projects.mdb
	//
	// ID_Commit - Came From Packet If It Works On Server-Client (The Same Like "Name" As Mention Above)
	//If Empty It Means Local Logic!
	//
	//
	// User - Can Be "nullptr", It Means That It's Use Local Logic (no use like Internet or something)
	//
	// Type - Can Be ONLY Type::Get_MetaData_Project Or Type::Get_MetaData_Project_Ex!
	//See The Code In CPP File For More Info About It!
	HRESULT Open(const std::string &Name, const std::string &ID_Commit = {},
		const Connection::SharedPtr &User = nullptr,
		const network::Packet::Type &Type = network::Packet::Type::Get_MetaData_Project);

	//DB
	static std::unique_ptr<odbc::ODBC> DataBase;

	static std::unique_ptr<Level> ThisLevel;
private:
	const std::string CurrentProj = {};
	
	void SetCurProject(const std::string &Name);
};
#endif // !__PROJECT_SYSTEM_H_
