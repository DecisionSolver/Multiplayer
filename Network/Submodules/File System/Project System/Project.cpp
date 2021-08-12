#include "Project.h"

#include <iomanip>

#if defined (DS_Engine)
#include "Multiplayer/Include/pch.h"
#endif

#if defined (DS_Engine)
#include "Project Manager/File System/File_system.h"
#else
#include "File System/File_system.h"
#endif
#if !defined (DS_Engine)
#include "../Level/Levels.h"
#else
#include "Project Manager/Level/Levels.h"
#endif

using namespace net;
using namespace network;

extern shared_ptr<File_system> FS;
#include "ConnMan.h"

std::unique_ptr<ProjectFile> Project = std::make_unique<ProjectFile>();
std::unique_ptr<Level> ProjectFile::ThisLevel = std::make_unique<Level>();

std::unique_ptr<odbc::ODBC> ProjectFile::DataBase = std::make_unique<odbc::ODBC>();
void ProjectFile::OpenOrCreateDB(bool ifNeedConnect)
{
	auto FilePath = FS->getPathFromType(_TypeOfFile::PROJECT) + "all projects.mdb";
	string DriverString = "Microsoft Access Driver (*.mdb)";

	// Check If We Have a Data Base File
	if (!exists(FilePath))
		DataBase->CreateDataBase(DriverString, FilePath);

	if (ifNeedConnect)
		DataBase->Connect(DriverString, FilePath, "READONLY=false", {});
}

// To Hold Data To Not Recreate It Every Calls "Open"!
std::shared_ptr<Packet> Info_Less = std::make_shared<Packet>(), Info_Full = std::make_shared<Packet>();
nlohmann::json JData_Less, JData_Full;
//

void OnNewTransforms()
{

}

void OnAddNewNode(std::shared_ptr<Level::Node>)
{
	Info_Full->clear();
	JData_Full.clear();
	JData_Full = nlohmann::json();

	Info_Less->clear();
	JData_Less.clear();
	JData_Less = nlohmann::json();
}

void OnDeleteNode(const std::string &)
{
	Info_Full->clear();
	JData_Full.clear();
	JData_Full = nlohmann::json();

	Info_Less->clear();
	JData_Less.clear();
	JData_Less = nlohmann::json();
}

// Name Came From Packet And Means Project Name Of Local DB On Server,
// Type Also Means That It Came From Packet From The Following User (It May Get_MetaData_Project Or Get_MetaData_Project_Ex)
// User Means Who Sent This Packet To Open Project!
HRESULT ProjectFile::Open(const std::string &Name, const std::string &ID_Commit,
	const Connection::SharedPtr &User, const network::Packet::Type &Type)
{
	// Server Must Use The Local Logic!
	// Local Logic

	// Check If Our Main DataBase Is Exist
	ProjectFile::OpenOrCreateDB();

	// Before Open Need To Clear Up Current Project!
	if (ThisLevel)
	{
		// If Already Loaded The Same Project Nothing To Do
#if defined (DS_Engine)
		if (ThisLevel->IsLoaded())
#else
		if (ThisLevel->IsLoaded() && (CurrentProj != Name))
#endif
		{
			ThisLevel->Destroy();
			Info_Full->clear();
			JData_Full.clear();
			JData_Full = nlohmann::json();

			Info_Less->clear();
			JData_Less.clear();
			JData_Less = nlohmann::json();
		}
	}

	auto AllData = Project->DataBase->SelectValues(Name, { "*" });
	if (!AllData.empty() && (AllData.find("Hash Commit") != AllData.end() && !AllData["Hash Commit"].empty()
		&& (ID_Commit.empty() ? (AllData.find("Hash_ID") != AllData.end() && !AllData["Hash_ID"].empty())
			: true)))
	{
		// Compare From The Last To The First Because Optimize (I Guess You Don't Want To Compare
		//		All 2k Records From Start To End xD)
		size_t i = AllData["Hash Commit"].size() - 1;
		for (; i >= 0; i--)
		{
			if (i == std::string::npos)
				break;
			if (AllData["Hash Commit"].at(i) == (ID_Commit.empty() ? (AllData["Hash_ID"].at(i)) :
				(nlohmann::json::value_type)ID_Commit))
			{
				auto Data = AllData["Data Commit"].at(i).is_object() ? AllData["Data Commit"].at(i + 1) :
					AllData["Data Commit"].at(i);

				// Avoid From "SQL_TYPE" Object!
				if (Data.is_string())
					// Get String Here 'Cause All Data Came Only That Type
				{
#if defined (DS_Engine)
					EngineTrace(ThisLevel->Load(Data.back().get<nlohmann::json::string_t>()));
#else
					bool PermToLoadProj = false;

					// If We Want To Load The Project From Local Or The Server Without The User At All (Like From The Code)
					if (!User) PermToLoadProj = true;
					else
					{
						auto Obj = User->m_owner->MySQL_DB->SelectValues("Local", { "_3" }, { "WHERE _N = '" +
							std::to_string(User->GetMetaDB_User()) + "';" });

						if (!Obj.empty() && Obj["_0"] == 1)
							PermToLoadProj = true;
					}

					if ((CurrentProj != Name && PermToLoadProj) || !ThisLevel->IsLoaded())
					{
						if (ThisLevel->Load(Data.back().get<nlohmann::json::string_t>()) == E_FAIL)
						{
							Logger_Error_F("Something Is Wrong With Load/Parse Data %s Project! Aborting!", Name.c_str());
							return E_FAIL;
						}
						else
						{
							if (CurrentProj.empty())
							{
								ThisLevel->SetCB_OnAddNewNode(OnAddNewNode);
								ThisLevel->SetCB_OnDeleteNode(OnDeleteNode);
							}
						}
					}
#endif
					// Client Logic
					// Here's Require The User Who Wants Get This Project From Server! MUST BE CONNECTED TO SERVER BEFORE!
					//
					// Server Can Send To This User Some Info About This Project If It Was Found And Valid!
					if (User)
					{
						// Start Getting All IDs And Store It To Send Everyone Who Wants To Connect
						auto Nodes = ThisLevel->getChild()->GetNodes();

						//if (Info_Less->getSize() == 0 && (JData_Less.empty() && JData_Less.dump() == "null"))
						//{
						if (Type == Packet::Type::Get_MetaData_Project)
						{
							for (const auto &It : Nodes)
							{
								JData_Full["_0"].push_back({ { "ID", It->ID }, {"RName", It->RenderName },
									{ "ModelFName", It->GM->GetModelNameFile() } });
							}
						}
						//}
						//if (Info_Full->getSize() == 0 && (JData_Full.empty() && JData_Full.dump() == "null"))
						//{
						if (Type == Packet::Type::Get_MetaData_Project_Ex)
						{
							for (const auto &It : Nodes)
							{
								auto GM = It->GM;
								std::string Pos, Rot, Scl;

								getTextFloat3(Pos, ",",
									{ GM->GetPositionCord().x, GM->GetPositionCord().y, GM->GetPositionCord().z });
								getTextFloat3(Rot, ",",
									{ GM->GetRotCord().x, GM->GetRotCord().y, GM->GetRotCord().z });
								getTextFloat3(Scl, ",",
									{ GM->GetScaleCord().x, GM->GetScaleCord().y, GM->GetScaleCord().z });

								JData_Full["_0"].push_back({ { "ID", It->ID }, {"RName", It->RenderName },
									{ "ModelFName", GM->GetModelNameFile() } });
								JData_Full["_1"].push_back({
									{ "Pos", Pos },
									{ "Scl", Scl },
									{ "Rot", Rot },
									});
							}
						}
						//}

						if (Type == Packet::Type::Get_MetaData_Project_Ex &&
							(!JData_Full.empty() && JData_Full.dump() != "null"))
						{
							Info_Full->CreatePacket(Type, true, JData_Full);

							User->Send(Info_Full);
							//Info_Full->clear();
							//JData_Full.clear();
							//JData_Full = nlohmann::json();
						}
						if (Type == Packet::Type::Get_MetaData_Project &&
							(!JData_Less.empty() && JData_Less.dump() != "null"))
						{
							Info_Less->CreatePacket(Type, true, JData_Less);

							User->Send(Info_Less);
						}
					}
					if (CurrentProj != Name)
						SetCurProject(Name);

					// Update Our New ID_Commit If It Has
					if (!ID_Commit.empty())
					{
						Project->DataBase->UpdateValues(
							Project->GetCurrentProject(),
							{ "Hash_ID" }, { "" });

						// Set Hash To Current Hash_Commit
						Project->DataBase->UpdateValues(
							Project->GetCurrentProject(),
							{ "Hash_ID" }, { ID_Commit },
							{ " WHERE `Hash Commit` = '" + ID_Commit + "'" });
					}
					return S_OK;
				}
			}
		}
	}
	return E_FAIL;
}

void ProjectFile::CreateProject(const std::string &Name)
{
	string BuffCmp;
	tinyxml2::XMLDocument *ConvertTo = new tinyxml2::XMLDocument();
	if (ConvertTo->Parse(("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
		string(_SCENE_BEGIN) + "\n" + string(_SCENE_END) + "\n").c_str()) == XML_SUCCESS)
	{
		auto Compressed = nlohmann::json::to_msgpack(XMLtoJSON(ConvertTo->FirstChildElement()));
		BuffCmp = string(Compressed.begin(), Compressed.end());
	}

	if (!BuffCmp.empty())
	{
		auto now = std::chrono::system_clock::now();
		std::time_t start_time = std::chrono::system_clock::to_time_t(now);
		tm *buf = new tm();
		localtime_s(buf, &start_time);
		char timedisplay[100];
		size_t len = std::strftime(timedisplay, sizeof(timedisplay), "%Y-%m-%d %H:%M:%S", buf);

		string Date = std::string(timedisplay, len), Hash = md5_from_buffer(Date + BuffCmp);
		Project->DataBase->CreateTable(Name,
			{ "Name Author", "Description", "Hash Commit", "Date Create", "Data Commit", "Hash_ID" },
			{ "TEXT", "LONGTEXT", "TEXT", "TEXT", "LONGTEXT", "TEXT" },
			{ "", "", "", "", "", "" },
			{ {}, {}, {}, {}, {}, {} });
		Project->DataBase->InsertValues(Name,
			{ "Name Author", "Description", "Date Create", "Data Commit", "Hash_ID", "Hash Commit" },
			{ "root", "Initial Commit", Date, BuffCmp, Hash, Hash });

		Project->SetCurProject(Name);
	}
}

void ProjectFile::SetCurProject(const std::string &Name)
{
	const_cast<string &>(CurrentProj) = Name;
}
