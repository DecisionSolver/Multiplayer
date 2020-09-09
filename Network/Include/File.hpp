#pragma once
#include <vector>
#include <string>

namespace swl
{
	class File
	{
	private:
		std::string filePath;
		std::string fileName;
		std::vector<char> data;

	public:
		File();
		File(std::string path);
		
		std::string& getFilePath();
		std::string& getFileName();
		uint32_t getDataSize();
		char* getFileData();

		void setFileName(const std::string& name);
		void setFileData(const std::vector<char>& _data);

		bool readFile();

		void saveFile();

	};
}

