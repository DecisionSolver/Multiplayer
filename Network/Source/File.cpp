#include "File.hpp"
#include <stdio.h>
#include <iostream>
#include <fstream>

namespace swl
{
	File::File() : filePath{}, fileName {}, data{}
	{
	}
	File::File(std::string path)
	{
		size_t length = 0;

		for (int i = path.size();; i--)
		{
			if (i == 0)
				break;
			else if (path[i] == '\\')
			{
				length--;
				break;
			}
			else
				length++;
		}

		fileName = path.substr(path.size() - length, length);
		filePath = path.substr(0, path.size() - length);
	}

	std::string& File::getFilePath()
	{
		return filePath;
	}

	std::string& File::getFileName()
	{
		return fileName;
	}

	uint32_t File::getDataSize()
	{
		return data.size();
	}

	char* File::getFileData()
	{
		return data.data();
	}

	void File::setFileData(const std::vector<char>& _data)
	{
		data = _data;
	}

	void File::setFileName(const std::string& name)
	{
		fileName = name;
	}

	bool File::readFile()
	{
		std::ifstream fileIn;
		fileIn.open((filePath + fileName), std::ios::binary);
		if (!fileIn.is_open())
			return true;
		int length;
		fileIn.seekg(0, std::ios::end);
		length = static_cast<int>(fileIn.tellg());
		fileIn.seekg(0);
		data.resize(length);
		fileIn.read(data.data(), length);
		fileIn.close();
		return false;
	}
	
	void File::saveFile()
	{
		std::ofstream fileOut;
		fileOut.open("2" + fileName, std::ios::binary);
		fileOut.write(data.data(), data.size());
		fileOut.close();
	}
}
