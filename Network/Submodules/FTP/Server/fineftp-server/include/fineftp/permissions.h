#pragma once

namespace fineftp
{
	enum class FilePermission : int
	{
		FileRead   = (1 << 0),  /**< Download files */
		FileAppend = (1 << 1),  /**< Upload files that get appended to existing files */
		FileDelete = (1 << 2),  /**< Delete existing files or overwrite an existing file */
		FileRename = (1 << 3),  /**< Rename existing files */

		DirList    = (1 << 4),  /**< Retrieve the content of directories */
		DirDelete  = (1 << 5),  /**< Delete existing directories */
		DirRename  = (1 << 6),  /**< Rename existing directories */

		FileAll = (FileRead | FileAppend | FileDelete | FileRename),
		DirAll  = (DirList | DirDelete | DirRename),
		None = 0
	};

	enum class UserPermission : int
	{
		FileWrite = (1 << 0),  /**< Upload files as new files */

		DirCreate = (1 << 1),  /**< Create new directories */

		All = (FileWrite | DirCreate),
		None = 0
	};

	inline FilePermission operator~   (FilePermission a)                    { return (FilePermission)~(int)a; }
	inline FilePermission operator|   (FilePermission a,  FilePermission b) { return (FilePermission)((int)a | (int)b); }
	inline FilePermission operator&   (FilePermission a,  FilePermission b) { return (FilePermission)((int)a & (int)b); }
	inline FilePermission operator^   (FilePermission a,  FilePermission b) { return (FilePermission)((int)a ^ (int)b); }
	inline FilePermission& operator|= (FilePermission& a, FilePermission b) { return (FilePermission&)((int&)a |= (int)b); }
	inline FilePermission& operator&= (FilePermission& a, FilePermission b) { return (FilePermission&)((int&)a &= (int)b); }
	inline FilePermission& operator^= (FilePermission& a, FilePermission b) { return (FilePermission&)((int&)a ^= (int)b); }

	inline UserPermission operator~   (UserPermission a)                    { return (UserPermission)~(int)a; }
	inline UserPermission operator|   (UserPermission a,  UserPermission b) { return (UserPermission)((int)a | (int)b); }
	inline UserPermission operator&   (UserPermission a,  UserPermission b) { return (UserPermission)((int)a & (int)b); }
	inline UserPermission operator^   (UserPermission a,  UserPermission b) { return (UserPermission)((int)a ^ (int)b); }
	inline UserPermission& operator|= (UserPermission& a, UserPermission b) { return (UserPermission&)((int&)a |= (int)b); }
	inline UserPermission& operator&= (UserPermission& a, UserPermission b) { return (UserPermission&)((int&)a &= (int)b); }
	inline UserPermission& operator^= (UserPermission& a, UserPermission b) { return (UserPermission&)((int&)a ^= (int)b); }
}