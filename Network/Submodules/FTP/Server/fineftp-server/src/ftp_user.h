#pragma once

#include <fineftp/permissions.h>
#include <string>
#include <nlohmann/json.hpp>

namespace fineftp
{
	struct FtpUser
	{
		FtpUser(const unsigned long long id, const std::string& password, const UserPermission user_permissions, const nlohmann::json& files_permissions)
			: id_                (id)
			, password_          (password)
			, user_permissions_  (user_permissions)
			, files_permissions_ (files_permissions)
		{}

		const unsigned long long id_;
		std::string              password_;
		UserPermission           user_permissions_;
		nlohmann::json           files_permissions_;
	};

	//Checks object(file or folder) for NOT having chosen permissions(actually they're restrictions)
	//How it works:
	//It parses object_path by going deeper in user's files_permissions
	//If directory in object_path is presented in subjson it goes in that dir in subjson and substracts next part from object_path
	//Else (and if it's file) it checks for ".AllFolderFiles" and for file's permissions (if presented) and returns true or false if it corresponds with chosen permissions
	inline bool hasObjectPermissionsImpl(const std::string& object_path, const FilePermission permissions, nlohmann::json& subjson)
	{
		if (subjson.empty() || subjson.find(".AllFolderFiles") == subjson.end())
			return false;

		if (object_path.empty())
		{
			if (!(subjson[".AllFolderFiles"] & (int)permissions))
				return true;
			else
				return false;
		}

		size_t div_pos = object_path.find('/', 0);
		std::string object_name = object_path.substr(0, div_pos);

		if (!subjson[object_name].is_null())
		{
			if (div_pos != std::string::npos)
			{
				if (!(subjson[".AllFolderFiles"] & (int)permissions) && !(subjson[object_name][1] & (int)permissions))
					return hasObjectPermissionsImpl(object_path.substr(div_pos + 1), permissions, subjson[object_name][0]);
				else
					return false;
			}
			else
			{
				if ((subjson[object_name].is_array() ? !(subjson[object_name][1] & (int)permissions) : !(subjson[object_name] & (int)permissions)) && !(subjson[".AllFolderFiles"] & (int)permissions))
					return true;
				else
					return false;
			}
		}
		else if (!(subjson[".AllFolderFiles"] & (int)permissions))
			return true;
		else
			return false;
	}
}