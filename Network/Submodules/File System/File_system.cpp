#include "File_system.h"

#if !defined (DS_Engine)
	#include "Level/Levels.h"
#else
	class Engine;
	extern shared_ptr<Engine> Application;
	#include "Engine.h"
	#include "Project Manager/Level/Levels.h"
#endif

const std::string File_system::GetFTPPath()
{
	std::string local_root;
	if (path(GetCurrentPath().c_str()).compare(std::string("workspace")) > 0)
		local_root = GetCurrentPath() + "Workspace/";
	else
		local_root = GetCurrentPath();

	return local_root;
}
const std::string File_system::SetPathFTP()
{
	std::string local_root = GetCurrentPath();
	if (path(local_root.c_str()).compare(std::string("workspace")) > 0)
	{
		if (!exists(local_root + "Workspace"))
			create_directories(local_root + "Workspace/");
		local_root += "Workspace/";
	}

	if (!exists(local_root + "updates"))
		create_directories(local_root + "updates/");

	return local_root;
}

void File_system::SetupLogs()
{
	std::string local_root = GetCurrentPath();
	if (path(local_root.c_str()).compare(std::string("workspace")) > 0)
	{
		if (!exists(local_root + "Workspace"))
			create_directories(local_root + "Workspace/");
		local_root += "Workspace/";
	}

#if __has_include("logger.h")
	if (!exists(local_root + "logs"))
		create_directories(local_root + "logs/");

	CppLogger::registerTarget(
		new FileLoggerTarget(local_root + "logs/FTP-server-all.log", LogLevel::LOG_LEVEL_DEBUG));
	CppLogger::registerTarget(
		new FileLoggerTarget(local_root + "logs/FTP-server-err.log", LogLevel::LOG_LEVEL_ERROR));
	CppLogger::registerTarget(
		new FileLoggerTarget(local_root + "logs/FTP-server-crit.log", LogLevel::LOG_LEVEL_CRITICAL));

	CppLogger::registerTarget(
		new FileLoggerTarget(local_root + "logs/ServerTCP-info.log", LogLevel::LOG_LEVEL_INFO));
	CppLogger::registerTarget(
		new FileLoggerTarget(local_root + "logs/ServerTCP-info.log", LogLevel::LOG_LEVEL_DEBUG));
#endif
}

void File_system::Update()
{
#if defined (DS_Engine)
	fileWatcher.update();
#endif
}

File_system::File_system()
{
#if __has_include("logger.h")
	Logger_Debug("C-tor");
#endif

	WorkDir = path(_getcwd(nullptr, UINT16_MAX)).generic_path();
	if (WorkDir.empty())
	{
#if __has_include("logger.h")
		Logger_Critical("Something is wrong with Get Resource Folder or Path!");
#endif
	}

#if __has_include("logger.h")
	Logger_Info_F("Selected Current Path: {}", WorkDir.string());
#endif

	WorkDirSources = WorkDir.generic_string() + ((WorkDir.generic_string().back() == '/')
		? "resource/"
		: "/resource/");
	
	SetupLogs();

	ScanFiles();
	
#if defined (DS_Engine)
	try
	{
		fileWatcher.addWatch(path(getPathFromType(_TypeOfFile::SCRIPTS)).string(), &listener);
	}
	catch (std::exception& e)
	{
		Logger_Error_F("An exception has occurred: {}\n", e.what());
	}
#endif
}

#if defined (DS_Engine)
	#include "Multiplayer/Include/pch.h"
#endif
void File_system::ScanFiles()
{
#if __has_include("logger.h")
	Logger_Debug("ScanFiles");
#endif

	if (!exists(WorkDirSources))
	{
#if defined (DS_Engine)
		MessageBoxA(Application->GetHWND(), "Engine Cannot Work Without Resource Folder!", "ERROR", MB_OK | MB_ICONERROR);
#endif
#if __has_include("logger.h")
		Logger_Fatal("Engine Cannot Work Without Resource Folder!");
#else
		return;
#endif
	}

	auto file = getFilesInFolder(WorkDirSources, true, true);

#if __has_include("logger.h")
	Logger_Info_F("All Files Found In Resource Folder Is: {}", file.size());
#endif

	int c_AllFilesWasAdded = 0;
	for (size_t i = 0; i < file.size(); i++)
	{
		path someFile = file.at(i),
			ext = extension(someFile),
			Fname = someFile.filename();
		auto type = GetTypeFileByExt(someFile);
		std::string Hash = md5_from_file(someFile.string());
		switch (type)
		{
		case MODELS:
			Models.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));
			
			c_AllFilesWasAdded++;
			break;
		case TEXTURES:
			Textures.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));
			
			c_AllFilesWasAdded++;
			break;
		case LEVELS:
			Levels.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));
			
			c_AllFilesWasAdded++;
			break;
		case DIALOGS:
			Dialogs.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));
			
			c_AllFilesWasAdded++;
			break;
		case SOUNDS:
			Sounds.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));
			
			c_AllFilesWasAdded++;
			break;
		case SHADERS:
			Shaders.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));

			c_AllFilesWasAdded++;
			break;
		case UIS:
			Uis.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));
			
			c_AllFilesWasAdded++;
			break;
		case SCRIPTS:
			Scripts.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));

			c_AllFilesWasAdded++;
			break;
		case FONTS:
			Fonts.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));

			c_AllFilesWasAdded++;
			break;
		default:
			None.push_back(make_pair(make_shared<File_system::File>(someFile, ext,
				Fname, (size_t)file_size(someFile), type, false, Hash), file.at(i)));
			break;
		}
	}
#if __has_include("logger.h")
	Logger_Info_F("Count Files Added To Engine Is: {}", c_AllFilesWasAdded);
#endif
}

void File_system::RescanFilesByType(const _TypeOfFile &Type)
{
	// Get New Files
	GetFileByType(Type).clear();
	auto Files = getFilesInFolder(getPathFromType(Type), true, true);
	for (size_t i = 0; i < Files.size(); i++)
	{
		GetFile(Files.at(i));
	}
}

_TypeOfFile File_system::GetTypeFileByExt(const path &File)
{
	auto Ext = File.extension().string();
	boost::to_lower(Ext);

	if (Ext == ".obj" || Ext == ".3ds" || Ext == ".fbx")
		return MODELS;
	else if (Ext == ".hlsl" || Ext == ".fx" || Ext == ".vs" || Ext == ".ps")
		return SHADERS;
	else if (Ext == ".dds" || Ext == ".png"
		|| Ext == ".bmp" || Ext == ".mtl"
		|| Ext == ".jpg")
		return TEXTURES;
	else if (Ext == ".wav")
		return SOUNDS;
	else if (Ext == ".lua")
		return SCRIPTS;
	else if (Ext == ".ttf" || Ext == ".ttc")
		return FONTS;
	else if (Ext == ".xml")
	{
		if (!File.has_branch_path() && !File.has_parent_path() && !File.has_root_directory() &&
			!File.has_root_name() && !File.has_root_path() && File.has_filename())
		{
			string WExt = File.string();
			deleteWord(WExt, File.extension().string());
			auto Obj = Find(WExt);
			auto path = Obj->Path.string();
			boost::to_lower(path);
			if (contains(path, "ui"))
				return UIS;
			if (contains(path, "maps"))
				return LEVELS;
			if (contains(path, "text"))
				return DIALOGS;
		}
		string lower = File.string();
		boost::to_lower(lower);
		if (contains(lower, "ui"))
			return UIS;
		if (contains(lower, "maps"))
			return LEVELS;
		if (contains(lower, "text"))
			return DIALOGS;
	}
	else
		return NONE;
	return NONE;
}

string File_system::getPathFromType(const _TypeOfFile &T)
{
	string New = WorkDirSources.string() + ((WorkDirSources.string().back() == '/') ? "" : "/");
	switch (T)
	{
	case MODELS:
		return New + "models/";
	case TEXTURES:
		return New + "textures/";
	case LEVELS:
		return New + "maps/";
	case DIALOGS:
		return New + "text/";
	case SOUNDS:
		return New + "sounds/";
	case SHADERS:
		return New + "shaders/";
	case UIS:
		return New + "ui/";
	case SCRIPTS:
		return New + "scripts/";
	case FONTS:
		return New + "ui/fonts/";
	case PROJECT:
		return New;
	case NONE:
		return "";
	}

	return New;
}

shared_ptr<File_system::File> File_system::Find(const path &File, bool AlsoAddFile)
{
	if (File.empty())
		return make_shared<File_system::File>();

	if (!File.has_extension())
		AlsoAddFile = true;

	if (AlsoAddFile)
	{
		shared_ptr<File_system::File> NewObj = make_shared<File_system::File>();
		auto AllFiles = getFilesInFolder(WorkDirSources, true, true);
		for (size_t i = 0; i < AllFiles.size(); i++)
		{
			bool NeedToAdd = false;
			auto this_file = AllFiles.at(i).string();
			boost::to_lower(this_file);
			const path Files = this_file;
			// Models
			if (((!File.has_branch_path() && !File.has_root_path()) || !File.has_extension()) ||
				Files.string().find("models/") != string::npos)
			{
				if (contains(Files.string(), File.string() + (!File.has_extension() ? ".obj" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::MODELS;
						NewObj->Ext = ".obj";

						NeedToAdd = true;
					}
					else
						return F;
				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".3ds" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::MODELS;
						NewObj->Ext = ".3ds";

						NeedToAdd = true;
					}
					else
						return F;

				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".fbx" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::MODELS;
						NewObj->Ext = ".fbx";

						NeedToAdd = true;
					}
					else
						return F;
				}
			}

			// Textures
			if (((!File.has_branch_path() && !File.has_root_path()) || !File.has_extension()) ||
				Files.string().find("textures/") != string::npos)
			{
				if (contains(Files.string(), File.string() + (!File.has_extension() ? ".dds" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::TEXTURES;
						NewObj->Ext = ".dds";

						NeedToAdd = true;
					}
					else
						return F;

				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".png" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{	// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::TEXTURES;
						NewObj->Ext = ".png";

						NeedToAdd = true;
					}
					else
						return F;
				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".bmp" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::TEXTURES;
						NewObj->Ext = ".bmp";

						NeedToAdd = true;
					}
					else
						return F;
				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".jpg" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::TEXTURES;
						NewObj->Ext = ".jpg";

						NeedToAdd = true;
					}
					else
						return F;
				}
			}

			// Shaders
			if (((!File.has_branch_path() && !File.has_root_path()) || !File.has_extension()) ||
				Files.string().find("shaders/") != string::npos)
			{
				if (contains(Files.string(), File.string() + (!File.has_extension() ? ".hlsl" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::SHADERS;
						NewObj->Ext = ".hlsl";

						NeedToAdd = true;
					}
					else
						return F;
				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".fx" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::SHADERS;
						NewObj->Ext = ".fx";

						NeedToAdd = true;
					}
					else
						return F;
				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".vs" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::SHADERS;
						NewObj->Ext = ".vs";

						NeedToAdd = true;
					}
					else
						return F;
				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".ps" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::SHADERS;
						NewObj->Ext = ".ps";

						NeedToAdd = true;
					}
					else
						return F;
				}
			}

			// Sounds
			if (((!File.has_branch_path() && !File.has_root_path()) || !File.has_extension()) || 
				Files.string().find("sounds/") != string::npos)
			{
				if (contains(Files.string(), File.string() + (!File.has_extension() ? ".wav" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::SOUNDS;
						NewObj->Ext = ".wav";
					
						NeedToAdd = true;
					}
					else
						return F;
				}
			}

			// Maps, UI and etc
			if (((!File.has_branch_path() && !File.has_root_path()) || !File.has_extension()) || 
				Files.string().find("ui/") != string::npos ||
				Files.string().find("maps/") != string::npos ||
				Files.string().find("text/") != string::npos)
			{
				if (contains(Files.string(), File.string() + (!File.has_extension() ? ".xml" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						if (contains(NewObj->Path.string(), "ui/"))
							NewObj->TypeOfFile = _TypeOfFile::UIS;
						else if (contains(NewObj->Path.string(), "maps/"))
							NewObj->TypeOfFile = _TypeOfFile::LEVELS;
						else if (contains(NewObj->Path.string(), "text/"))
							NewObj->TypeOfFile = _TypeOfFile::DIALOGS;
						NewObj->Ext = ".xml";
						NeedToAdd = true;
					}
					else
						return F;
				}
			}

			// Scripts
			if (((!File.has_branch_path() && !File.has_root_path()) || !File.has_extension()) || 
				Files.string().find("scripts/") != string::npos)
			{
				if (contains(Files.string(), File.string() + (!File.has_extension() ? ".lua" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::SCRIPTS;
						NewObj->Ext = ".lua";
					
						NeedToAdd = true;
					}
					else
						return F;
				}
			}

			// Fonts
			if (((!File.has_branch_path() && !File.has_root_path()) || !File.has_extension()) || 
				Files.string().find("fonts/") != string::npos)
			{
				if (contains(Files.string(), File.string() + (!File.has_extension() ? ".ttf" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::FONTS;
						NewObj->Ext = ".ttf";

						NeedToAdd = true;
					}
					else
						return F;
				}
				else if (contains(Files.string(), File.string() + (!File.has_extension() ? ".ttc" : "")))
				{
					auto F = GetFileByPath(Files);
					if (!F || F->FName.empty())
					{
						// If need to add it to engine
						NewObj->TypeOfFile = _TypeOfFile::FONTS;
						NewObj->Ext = ".ttc";
						
						NeedToAdd = true;
					}
					else
						return F;
				}
			}
			
			if (NeedToAdd)
			{
				NewObj->Path = Files.string();
				NewObj->FName = Files.filename().string();
				NewObj->Size = (size_t)file_size(NewObj->Path);
				NewObj->Hash = md5_from_file(NewObj->Path.string());
				return NewObj;
			}
		}
	}
	else
	{
		switch (GetTypeFileByExt(File))
		{
		case MODELS:
			for (const auto &elem: Models)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case TEXTURES:
			for (const auto &elem: Textures)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case LEVELS:
			for (const auto &elem: Levels)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case DIALOGS:
			for (const auto &elem: Dialogs)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case SOUNDS:
			for (const auto &elem: Sounds)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case SHADERS:
			for (const auto &elem: Shaders)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case UIS:
			for (const auto &elem: Uis)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case SCRIPTS:
			for (const auto &elem: Scripts)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
			break;
		case FONTS:
			for (const auto &elem: Fonts)
			{
				if (contains(elem.second.filename().string(), File.filename().string()))
					return elem.first;
			}
		}
	}
	return make_shared<File_system::File>();
}

std::vector<pair<shared_ptr<File_system::File>, path>> File_system::GetFileByType(const _TypeOfFile &T)
{
	switch (T)
	{
	case MODELS:
		return Models;
	case TEXTURES:
		return Textures;
	case LEVELS:
		return Levels;
	case DIALOGS:
		return Dialogs;
	case SOUNDS:
		return Sounds;
	case SHADERS:
		return Shaders;
	case UIS:
		return Uis;
	case SCRIPTS:
		return Scripts;
	case FONTS:
		return Fonts;
	case NONE:
		return None;
	}

	return vector<pair<shared_ptr<File_system::File>, path>>();
}

shared_ptr<File_system::File> File_system::GetFileByPath(const path &File)
{
	string lower = File.lexically_normal().string();

	switch (GetTypeFileByExt(lower))
	{
	case MODELS:
		for (auto it: Models)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case TEXTURES:
		for (auto it: Textures)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case LEVELS:
		for (auto it: Levels)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case DIALOGS:
		for (auto it: Dialogs)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case SOUNDS:
		for (auto it: Sounds)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case SHADERS:
		for (auto it: Shaders)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case UIS:
		for (auto it: Uis)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case SCRIPTS:
		for (auto it: Scripts)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	case FONTS:
		for (auto it: Fonts)
		{
			string Cmp = it.second.string();
			boost::to_lower(Cmp);
			if ((contains(Cmp, lower)))
				return it.first;
		}
		break;
	}

	return shared_ptr<File_system::File>();
}

std::pair<bool, std::shared_ptr<File_system::File>> File_system::IsSame(const std::string &FileName, std::string &Hash)
{
	if (FileName.empty() || Hash.empty()) return { false, nullptr };
	
	boost::to_lower(Hash);

	if (!path(FileName).has_extension())
	{
		auto Obj = Find(FileName, false);
		if (Obj && (Obj->Size > 0 && !Obj->Hash.empty()))
		{
			if (contains(Obj->Hash, Hash))
				return { true, Obj };
			else
				return { false, nullptr };
		}
		else
			return { false, nullptr };
	}

	// Get _TypeOfFile From FileName
	auto T = GetTypeFileByExt(FileName);
	switch (T)
	{
	case MODELS:
		for (const auto &It: Models)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case TEXTURES:
		for (const auto &It: Textures)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case LEVELS:
		for (const auto &It: Levels)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case DIALOGS:
		for (const auto &It: Models)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case SOUNDS:
		for (const auto &It: Sounds)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case SHADERS:
		for (const auto &It: Shaders)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case UIS:
		for (const auto &It: Uis)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case SCRIPTS:
		for (const auto &It: Scripts)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	case FONTS:
		for (const auto &It: Fonts)
		{
			auto Str = It.first->Hash;
			boost::to_lower(Str);
			if (contains(Str, Hash))
				return { true, It.first };
		}
		break;
	default:
		break;
	}
	return { false, nullptr };
}

shared_ptr<File_system::File> File_system::GetFile(path File)
{
	if (File.empty()) return shared_ptr<File_system::File>();
	
	auto copy = File.generic();
	File.swap(copy);
	string Fname = File.generic_string();
	boost::to_lower(Fname);

	// It means that we have the full-path like this "C:/SommePath/Somme.obj" only with lower cases
	if (File.has_branch_path() && File.has_parent_path() && File.has_root_directory() &&
		File.has_root_name() && File.has_root_path() && File.has_extension() && File.has_filename())
	{
		auto Obj = GetFileByPath(Fname);
		if (Obj && Obj->Size > 0)
			return Obj;
		if (File.has_extension())
			deleteWord(Fname, File.extension().string());

		// if it was empty try to add to engine
		Find(Fname);
	}

	// It means that we have the path like this "Somme" or with full-path "C:/SommePath/Somme"
	else if (File.has_filename())
	{
		if (File.has_extension())
			deleteWord(Fname, File.extension().string());

		auto Obj = Find(Fname);
		if (Obj && Obj->Size > 0)
			return Obj;
	}

#if defined (DS_Engine)
	pair<string, vector<pair<bool, string>>> ListTextures;
#endif
	return AddFile(File
#if defined (DS_Engine)
		, ListTextures
#endif
	);
}

#if defined (DS_Engine)
#include "Project Manager/Level/Model/Models.h"
void getListTexturesFromModel(aiNode *node, const aiScene *pScene, vector<string> &ListTextures)
{
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		auto mesh = pScene->mMeshes[node->mMeshes[i]];
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial *material = pScene->mMaterials[mesh->mMaterialIndex];
			for (auto type = 0; (aiTextureType)type <= AI_TEXTURE_TYPE_MAX; type++)
			{
				for (UINT i = 0; i < material->GetTextureCount((aiTextureType)type); i++)
				{
					aiString str = {};
					material->GetTexture((aiTextureType)type, i, &str);
					if (str.C_Str() && str.data)
						ListTextures.push_back(path(str.C_Str()).filename().string());
				}
			}
		}

	}
	for (UINT i = 0; i < node->mNumChildren; i++)
		getListTexturesFromModel(node->mChildren[i], pScene, ListTextures);
}
#endif

shared_ptr<File_system::File> File_system::OnlyAddFile(const path &File)
{
	auto T = GetTypeFileByExt(File);
	std::string Hash = md5_from_file(File.string());

	// Copy File
	std::string pathType = getPathFromType(GetTypeFileByExt(File));
	if (!exists(pathType + "/" + File.string()))
		create_directory(pathType + path(pathType).filename().string());

	boost::filesystem::copy(File, pathType + path(pathType).filename().string());

	switch (T)
	{
	case MODELS:
		Models.push_back({ make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string() });
		return Models.back().first;
		break;
	case TEXTURES:
		Textures.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Textures.back().first;
		break;
	case LEVELS:
		Levels.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Levels.back().first;
		break;
	case DIALOGS:
		Dialogs.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Dialogs.back().first;
		break;
	case SOUNDS:
		Sounds.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Sounds.back().first;
		break;
	case SHADERS:
		Shaders.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Shaders.back().first;
		break;
	case UIS:
		Uis.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Uis.back().first;
		break;
	case SCRIPTS:
		Scripts.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Scripts.back().first;
		break;
	case FONTS:
		Fonts.push_back(make_pair(make_shared<File_system::File>(File.string(), File.extension().string(),
			File.filename().string(), (size_t)file_size(File), T, false, Hash), File.string()));
		return Fonts.back().first;
		break;
	}
	return shared_ptr<File_system::File>();
}
#if defined (DS_Engine)
	shared_ptr<File_system::File> File_system::AddFile(path File, pair<string, vector<pair<bool, string>>> &ListTextures)
#else
	shared_ptr<File_system::File> File_system::AddFile(path File)
#endif
{
	_TypeOfFile T = NONE;
	string PathFile, ext;

	auto copy = File.generic();
	File.swap(copy);

	string Fname = File.generic_string();
	boost::to_lower(Fname);

	// Try To Find This File In Resources Of Engine
	auto _Obj = Find(Fname);

#if defined (DS_Engine)
	// If Only Dialog Load Models/Textures
	if (File.empty() && !ListTextures.second.empty())
	{
		shared_ptr<File_system::File> _Obj;
		vector<string> tmp;
		int ID = 0;
		bool Finded = false;

		for (auto _It: ListTextures.second)
		{
			auto Obj = _It.second;
			if (Obj.empty()) continue;

			Fname = path(Obj).string();
			boost::to_lower(Fname);

			if (path(Obj).has_root_path() && _It.first) // Don't Add Necessary File
				tmp.push_back(Fname);
		}

		for (auto It = ListTextures.second.begin(); It != ListTextures.second.end(); It++)
		{
			auto Obj = It->second;
			if (Obj.empty() || !It->first || !path(Obj).has_root_path()) continue;

			Fname = path(Obj).filename().string();
			boost::to_lower(Fname);

			auto it = std::find_if(ListTextures.second.begin(),
				ListTextures.second.end(), [&](const pair<bool, string> &val)
			{
				if (path(val.second).filename().string() == Fname)
					return true;
				else
				{
					ID++;
					return false;
				}
			});
			if (it != ListTextures.second.end())
				Finded = true;

			path _File = path(Obj);
			
			// Try To Find It In Resource Of Engine
			_Obj = Find(_File.filename().string(), false);

			if (_Obj && _Obj->Size == 0 && !_Obj->HasTextures) // Find Our Undoned Files In AllFiles
			{
				// Copy Them And Change Them To Set Full-Path And Other
				bool IsCreated = false;

				ext = path(ListTextures.first).extension().string();
				string delExt = ListTextures.first,
					pathType = getPathFromType(GetTypeFileByExt(Fname));
				// Replace Ext
				deleteWord(delExt, ext);
				if (!exists(pathType + delExt + "/" + Fname))
				{
					if (_Obj->Path.empty())
					{
						path Path = pathType + delExt + "/" + Fname;

						Textures.push_back(make_pair(make_shared<File_system::File>(Path.string(), ext, Fname,
							(size_t)file_size(_File), T, false, md5_from_file(Path.string())), Path.string()));

						IsCreated = true;
						if (!exists(pathType + delExt))
							create_directory(pathType + delExt);

						boost::filesystem::copy(_File, Path);
					}
					else
					{
						if (!exists(pathType + delExt))
							create_directory(pathType + delExt);

						boost::filesystem::copy(_File, path(_Obj->Path));
					}

					if (!IsCreated)
					{
						_Obj->HasTextures = true;
						_Obj->Size = (size_t)file_size(_File);
					}
				}
			}
			if (!ListTextures.second.empty() && ListTextures.second.size() > 1) // It means not 0 or 1
			{
				It = ListTextures.second.erase(ListTextures.second.begin() + ID);
				if (It == ListTextures.second.end())
					It = ListTextures.second.begin();
			}
			if (ListTextures.second.size() == 1) // Just erase the first
				It = ListTextures.second.erase(ListTextures.second.begin());
			if (ListTextures.second.empty())
				break;
		}
		return _Obj;
	}
#endif

	// It means that we have the path like this "Somme" or with full-path
	if (!File.empty() && File.has_filename() && _Obj && _Obj->Size == 0)
	{
		Fname = File.filename().string();
		PathFile = File.parent_path().string();

		T = GetTypeFileByExt(Fname);

		if (T == _TypeOfFile::NONE)
		{
#if __has_include("logger.h")
			Logger_Error_F("File: {} Isn't Supported By Engine\n", Fname);
#endif
			return shared_ptr<File_system::File>(); // Unsupported File!
		}

		ext = File.extension().string();
		boost::to_lower(ext);
		string delExt = Fname, pathType = getPathFromType(T); // Replace Ext
		deleteWord(delExt, ext);
		path Path = pathType + delExt + "/" + Fname;

#if defined (DS_Engine)
		// Try To Find Some Textures From File And Add It To Queue Engine To Model
		
		if (T == _TypeOfFile::MODELS)
		{
			auto importer = new Assimp::Importer;

			auto pScene = importer->ReadFile(File.string(), 0);
			if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode || !pScene->HasMeshes())
			{
#if __has_include("logger.h")
				Logger_Error_F("Some Trouble With \"{}\" File!\n", Fname);
#endif
				return shared_ptr<File_system::File>();
			}
			else
			{
				vector<string> tmpList;
				getListTexturesFromModel(pScene->mRootNode, pScene, tmpList);
				if (!tmpList.empty())
					ListTextures.first = Fname;
				else
				{
#if __has_include("logger.h")
					Logger_Error_F("\"{}\" File Doesn't Have Any Materials (Or Textures)!\n", Fname);
#endif
				}
				for (auto It: tmpList)
				{
					PathFile = getPathFromType(GetTypeFileByExt(It));
					string delExt = Fname; // Replace Ext
					deleteWord(delExt, ext);
					if (!exists(path(PathFile + delExt + "/" + It)))
						ListTextures.second.push_back(make_pair(false, It));
				}
			}
			// Copy New File To Resource In Folder Of Type File
			if (!exists(Path))
			{
				if (!exists(pathType + delExt))
					create_directory(pathType + delExt);

				boost::filesystem::copy(File, path(Path));
				Models.push_back(make_pair(make_shared<File_system::File>(Path.string(), ext, Fname,
					exists(path(pathType + delExt)) ? (size_t)file_size(Path) : 0, T, false,
					md5_from_file(File.string())), Path.string()));
			}
		}
#endif

		delExt = Path.filename().string(); // Replace Ext
		deleteWord(delExt, Path.extension().string());

#if defined (DS_Engine)
		// Check If It Hasn't Around Model Then Add Dialog That File Textures Need To Be Find
		for (size_t i = 0; i < ListTextures.second.size(); i++)
		{
			Fname = path(ListTextures.second.at(i).second).filename().string();
			T = GetTypeFileByExt(Fname);
			if (T != _TypeOfFile::TEXTURES)
				continue;

			pathType = getPathFromType(T);
			auto Path = pathType + delExt + "/" + Fname;
			try
			{
				if (!exists(pathType + delExt))
					create_directory(pathType + delExt);

				if (exists(path(PathFile + "/" + Fname)))
					boost::filesystem::copy(path(PathFile + "/" + Fname), path(Path));

				Textures.push_back(make_pair(make_shared<File_system::File>(Path, ext, Fname,
					(exists(path(PathFile + "/" + Fname)) ? (size_t)file_size(Path) : 0), T, false,
					md5_from_file(PathFile + "/" + Fname)), path(Path).string()));
			}
			catch (boost::filesystem::filesystem_error const &e)
			{
#if __has_include("logger.h")
				Logger_Error_F("{}", e.what());
#endif
				return shared_ptr<File_system::File>();
			}
		}
#endif
		return shared_ptr<File_system::File>();
	}
	else if (_Obj && _Obj->Size > 0)
		return _Obj;
	else if (!_Obj && (_Obj->Size <= 0 || !exists(_Obj->Path)))
	{
		if (!exists(_Obj->Path))
		{
#if __has_include("logger.h")
			Logger_Error_F("\"{}\" Doesn't Exist Or Not Found!\n", Fname);
#endif
			return shared_ptr<File_system::File>();
		}
	}

	_Obj = OnlyAddFile(File);
	if (!_Obj && _Obj->Path.empty() && _Obj->Size == 0)
	{
#if __has_include("logger.h")
		Logger_Error_F("\"{}\" not supported!\n", Fname);
#endif
	}
	else
		return _Obj;

	return shared_ptr<File_system::File>();
}

vector<path> File_system::getFilesInFolder(const path &Folder, bool Recursive, bool onlyFile)
{
	vector<path> files;

	if (!Recursive && !onlyFile)
		for (directory_iterator it(Folder); it != directory_iterator(); ++it)
		{
			auto str = it->path();
			if (is_directory(str))
				files.push_back(str.normalize());
		}
	else if (Recursive && onlyFile)
		for (recursive_directory_iterator it(Folder); it != recursive_directory_iterator(); ++it)
		{
			auto str = it->path();
			if (!is_directory(str))
				files.push_back(str.normalize());
		}
	else if (onlyFile && !Recursive)
		for (directory_iterator it(Folder); it != directory_iterator(); ++it)
		{
			auto str = it->path();
			if (!is_directory(str))
				files.push_back(str.normalize());
		}
	else if (!onlyFile && Recursive)
		for (recursive_directory_iterator it(Folder); it != recursive_directory_iterator(); ++it)
		{
			auto str = it->path();
			if (is_directory(str))
				files.push_back(str.normalize());
		}

	return files;
}
vector<path> File_system::getFilesInFolder(const path &Folder)
{
	vector<path> files;
	for (directory_iterator it(Folder); it != directory_iterator(); ++it)
	{
		auto File = it->path();
		files.push_back(File.normalize());
	}

	return files;
}

string File_system::getDataFromFile(const string &File, const string &start, const string &end)
{
	if (File.empty())
		return "";

	try
	{
		string Returned_val;
		std::ifstream streamObj = std::ifstream(File.c_str());
		streamObj >> noskipws;
		if (streamObj.is_open())
		{
			copy(istream_iterator<char>(streamObj), istream_iterator<char>(), std::back_inserter(Returned_val));

			if (!Returned_val.empty())
				if (!start.empty() & !end.empty())
				{
					deleteWord(Returned_val, start, end);
					return Returned_val;
				}
				else
					return Returned_val;
		}
	}
	catch (std::ifstream::failure e)
	{
#if __has_include("logger.h")
		Logger_Error_F("Catch With Error:!\nMessage: {}\nError Code: {}",
			e.code().message(), e.code().value());
#endif
	}
	return "";
}

vector<string> File_system::getDataFromFileVector(const string &File, bool LineByline)
{
	vector<string> Returned_val;
	string Cache;
	auto streamObj = std::ifstream(File.c_str(), std::ifstream::binary);
	if (LineByline)
		while (!streamObj.eof())
		{
			getline(streamObj, Cache);
			Returned_val.push_back(Cache);
		}
	else
		while (!streamObj.eof())
		{
			streamObj >> Cache;
			Returned_val.push_back(Cache);
		}

	if (!Returned_val.empty())
		return Returned_val;

	return vector<string>();
}
bool File_system::ReadFileMemory(const LPCSTR &filename, size_t &FileSize, vector<BYTE> &FilePtr)
{
	try
	{
		if (!exists(filename) || file_size(filename) == 0) return false;
		std::ifstream is(filename, ios::binary);
		if (is)
		{
			is.unsetf(std::ios::skipws);
			is.seekg(0, is.end);
			FileSize = static_cast<size_t>(is.tellg());
			is.seekg(0, is.beg);

			FilePtr.reserve(FileSize + 1);

			// read the data:
			FilePtr.insert(FilePtr.begin(),
				std::istream_iterator<BYTE>(is),
				std::istream_iterator<BYTE>());
			FilePtr.push_back('\0');
		}
	}
	catch (std::ifstream::failure e)
	{
#if __has_include("logger.h")
		Logger_Error_F("Catch With Error:!\nMessage: {}\nError Code: {}",
			e.code().message(), e.code().value());
#endif
	}
	return true;
}

path File_system::WorkDirSources = {};

boost::property_tree::ptree File_system::LoadSettingsFile()
{
	path p = (GetCurrentPath() + "settings.cfg");
	try
	{
		if (!boost::filesystem::exists(p)) return boost::property_tree::ptree();

#if __has_include("logger.h")
		Logger_Debug_F("Trying To Load Settings From: {}", p.string());
#endif

		vector<BYTE> File; size_t Size = 0;
		boost::property_tree::ptree fData;
		if (File_system::ReadFileMemory(p.string().c_str(), Size, File))
		{
			string Data = reinterpret_cast<char *>(File.data());

			std::istringstream ini(Data);
			boost::property_tree::ini_parser::read_ini(ini, fData);
		}

		return fData;
	}
	catch (const boost::property_tree::ptree_error &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("Error Occured: {}", e.what());
#endif
		return boost::property_tree::ptree();
	}
}

void File_system::SaveSettings(const vector<pair<string, string>> &ToFile)
{
	path p(GetCurrentPath() + "settings.cfg");

#if __has_include("logger.h")
	Logger_Debug_F("Trying To Save Settings To: {}", p.string());
#endif
	try
	{
		boost::property_tree::ptree fData;

		for (const auto &Auto: ToFile)
		{
			fData.put<string>(Auto.first, Auto.second);
		}

		boost::property_tree::ini_parser::write_ini(p.string(), fData);
#if __has_include("logger.h")
		Logger_Debug("Saved Successfully");
#endif
	}
	catch (const boost::property_tree::ptree_error &e)
	{
#if __has_include("logger.h")
		Logger_Error_F("Error Occured: {}", e.what());
#endif
	}
}

#if defined (DS_Engine)
#include "Script System/CLua.h"
extern shared_ptr<CLua> lua;
void File_system::UpdateListener::handleFileAction(FW::WatchID watchid, const FW::String &dir,
	const FW::String &filename, FW::Action action)
{
	path ChangedDir = dir;
	if (ChangedDir == getPathFromType(_TypeOfFile::SCRIPTS) && action == FW::Action::Modified)
		lua->Update(filename.c_str());
}
#endif
