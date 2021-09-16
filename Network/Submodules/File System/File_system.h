/**
 * \file	File_system.h.
 *
 * \brief	Declares the file system class
 */

#pragma once
#if !defined(__FILE_SYSTEM_H__)
#define __FILE_SYSTEM_H__

#if __has_include("Core/pch.h")
	#include "Core/pch.h"
#else
	#include "Tools.h"
#endif

#include <fstream>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <Boost/iostreams/filtering_streambuf.hpp>
#include <Boost/iostreams/copy.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
using namespace boost::algorithm;

#include <boost/format.hpp>

#include "FileWatcher/FileWatcher/FileWatcher.h"

using namespace std;
using namespace boost::filesystem;
using namespace boost::iostreams;

/**
 * \enum	_TypeOfFile
 *
 * \brief	Values that represent type of files
 */

enum _TypeOfFile { MODELS = 1, TEXTURES, LEVELS, DIALOGS, SOUNDS, SHADERS, UIS, SCRIPTS, FONTS, PROJECT, NONE };

/**
 * \class	File_system
 *
 * \brief	A file system.
 *
 * \author	PBAX
 * \date	25.02.2020
 */

class Level;
class File_system
{
#if defined (DS_Engine)
private:
	class UpdateListener: public FW::FileWatchListener
	{
	public:
		UpdateListener() {}
		void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename,
			FW::Action action);
	};
	UpdateListener listener;
	FW::FileWatcher fileWatcher;
#endif

public:
	/** \brief	The work dir */
	path WorkDir;
	static path WorkDirSources;
	/** \brief	The Project For SDK */

	/**
	 * \struct	AllFile
	 *
	 * \brief	Struct Of Files.
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 */

	struct File
	{
		File() {}
		File(const path &new_Path, const path &new_Ext, const path &new_File, const size_t &new_Size,
			const _TypeOfFile &new_TypeOfFile, bool if_HasTextures = false, const std::string &hash = {}):
			Path(new_Path), Ext(new_Ext), FName(new_File),
			Size(new_Size), TypeOfFile(new_TypeOfFile), HasTextures(if_HasTextures),
		Hash(hash) {}

		// Full Path To Required File
		path Path = {}, Ext = {}, FName = {};

		bool HasTextures = false;

		size_t Size = 0;

		std::string Hash = {};

		_TypeOfFile TypeOfFile = _TypeOfFile::NONE;
	};
	vector<pair<shared_ptr<File>, path/*IDPath*/>>
		Models,
		Textures,
		Levels,
		Dialogs,
		Sounds,
		Shaders,
		Uis,
		Scripts,
		Fonts,
		None;
public:
	void Update();

	/**
	 * \fn	File_system::File_system();
	 *
	 * \brief	Default Ctor Which Scan Files And Get Main Path
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 */

	 File_system();
	~File_system() {}

	/**
	 * \fn	void File_system::ScanFiles();
	 *
	 * \brief	Scan Files And Add To Engine
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 */

	void ScanFiles();

	/**
	 * \fn	void File_system::RescanFilesByType(const _TypeOfFile &T);
	 *
	 * \brief	Rescan Files By Type
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 *
	 * \param 	T	A _TypeOfFile to process.
	 */

	void RescanFilesByType(const _TypeOfFile &T);

	/**
	 * \fn	shared_ptr<AllFile::File> File_system::GetFile(path File);
	 *
	 * \brief	Gets a file
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 *
	 * \param 	File	The file.
	 *
	 * \returns	The file.
	 */

	shared_ptr<File> GetFile(path File);

	/**
	 * \fn	shared_ptr<AllFile::File> File_system::AddFile(path File,
		pair<string, vector<pair<bool, string>>> &ListTextures = pair<string, vector<pair<bool, string>>>());
	 *
	 * \brief	Adds a file
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 *
	 * \param 	File	The file.
	 *
	 * \returns	A shared_ptr&lt;AllFile::File&gt;
	 */

#if defined (DS_Engine)
	shared_ptr<File> AddFile(path File, pair<string, vector<pair<bool, string>>> &ListTextures);
#else
	shared_ptr<File> AddFile(path File);
#endif

	// Arg "File" Need To Have fullpath!
	shared_ptr<File_system::File> OnlyAddFile(const path &File);
	
	/**
	 * \fn	vector<pair<shared_ptr<AllFile::File>, path>> File_system::GetFileByType(const _TypeOfFile &T);
	 *
	 * \brief	Get File By Type
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 *
	 * \param 	T	A _TypeOfFile to process.
	 *
	 * \returns	The file by type.
	 */

	vector<pair<shared_ptr<File>, path>> GetFileByType(const _TypeOfFile &T);

	/**
	 * \fn	vector<path> File_system::getFilesInFolderW(path Folder);
	 *
	 * \brief	Get Massive Files In Resource Folder
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	Folder	Pathname of the folder. boost::path
	 *
	 * \returns	Massive Files Of Needed Folder.
	 */

	vector<path> getFilesInFolder(const path &Folder);

	/**
	 * \fn	vector<path> File_system::getFilesInFolder(const path &Folder, bool Recursive = false, bool onlyFile = false);
	 *
	 * \brief	Get Massive Files In Resource Folder
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	Folder   	Pathname of the folder. boost::path
	 * \param 	Recursive	(Optional) True to process recursively, false to process locally only.
	 * \param 	onlyFile 	(Optional) True to only file.
	 *
	 * \returns	Massive Files Of Needed Folder.
	 */

	vector<path> getFilesInFolder(const path &Folder, bool Recursive = false, bool onlyFile = false);

	/**
	 * \fn	string File_system::getDataFromFile(const string &File, const string &start = "<!--", const string &end = "-->");
	 *
	 * \brief	Get Data (In String) From File
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	File 	Needed File.
	 * \param 	start	(Optional) The start.
	 * \param 	end  	(Optional) The end.
	 *
	 * \returns	String Data File.
	 *
	 * ### param 	LineByline	True to line byline.
	 */

	string getDataFromFile(const string &File, const string &start = "<!--", const string &end = "-->");

	/**
	 * \fn	vector<string> File_system::getDataFromFileVector(const string &File, bool LineByline);
	 *
	 * \brief	Get Data (In Massive String) From File
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	File	  	Needed File.
	 * \param 	LineByline	True To Read Line By Line.
	 *
	 * \returns	Massive Strings File.
	 */

	vector<string> getDataFromFileVector(const string &File, bool LineByline);

	/**
	 * \fn	bool File_system::ReadFileMemory(const LPCSTR &filename, size_t &FileSize, vector<BYTE> &FilePtr);
	 *
	 * \brief	Reads file memory
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 		  	filename	Filename of the file. Ansi
	 * \param [in,out]	FileSize	If non-null, size of the file.
	 * \param [in,out]	FilePtr 	If non-null, the file pointer.
	 *
	 * \returns	True if it succeeds, false if it fails.
	 */

	static bool ReadFileMemory(const LPCSTR &filename, size_t &FileSize, vector<BYTE> &FilePtr);

	/**
	 * \fn	_TypeOfFile File_system::GetTypeFileByExt(const path &File);
	 *
	 * \brief	Constructor
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	File	The file.
	 */

	_TypeOfFile GetTypeFileByExt(const path &File);

	/**
	 * \fn	shared_ptr<File> File_system::GetFileByPath(const path &File);
	 *
	 * \brief	Gets file by path
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	File	The file.
	 *
	 * \returns	The file by path.
	 */

	shared_ptr<File> GetFileByPath(const path &File);

	// Checking If Given Hash Has In File System And FileName Exists And Has The Same Hash!
	// Returns Found File By Hash
	std::pair<bool, std::shared_ptr<File_system::File>> IsSame(const std::string &FileName, std::string &Hash);

	/**
	 * \fn	auto static File_system::GetCurrentPath()
	 *
	 * \brief	Gets current path
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \returns	The current path.
	 */

	string GetCurrentPath() { return WorkDir.generic_string() + ((WorkDir.generic_string().back() == '/') ? "" : "/"); }

	/**
	 * \fn	string File_system::getPathFromType(const _TypeOfFile &T);
	 *
	 * \brief	Gets path from type
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	T	A _TypeOfFile to process.
	 *
	 * \returns	The path from type.
	 */

	static string getPathFromType(const _TypeOfFile &T);

	/**
	 * \fn	void File_system::CreateProject(const std::string &Name)
	 *
	 * \brief	Create Project
	 *
	 * \author	PBAX
	 * \date	29.02.2020
	 *
	 * \param 	Name The Current In DB Table.
	 */

	boost::property_tree::ptree LoadSettingsFile();
	void SaveSettings(const vector<pair<string, string>> &ToFile);

	const string getWorkDirSource() { return WorkDirSources.generic_string(); }
protected:
	/**
	 * \fn	shared_ptr<File_system::AllFile::File> File_system::Find(path File);
	 *
	 * \brief	Searches for the first match for the given path
	 *
	 * \author	PBAX
	 * \date	25.02.2020
	 *
	 * \param 	File	The file.
	 *
	 * \returns	A shared_ptr<File_system::AllFile::File>;
	 */

	shared_ptr<File_system::File> Find(const path &File, bool AlsoAddFile = true);
};
#endif // !__FILE_SYSTEM_H__
