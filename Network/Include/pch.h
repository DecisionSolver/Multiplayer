#pragma once

//#include "LZ4/lz4.h"
#include "nlohmann/json.hpp"

#include <chrono>
#include <direct.h>
#include <queue>
#include <thread>
#include <atomic>
#include <vector>
#include <tuple>
#include <future>
#include <string>
#include <sstream>
#include <iostream>
#include <list>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <memory>

#include <cryptopp/cryptlib.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>

using namespace std::chrono_literals;

#if __has_include("logger.h")
#include "logger.h"
#endif

const std::string md5_from_file(const std::string &path);
const std::string md5_from_buffer(const std::string &data);
std::string SHA256(std::string data);
std::string String2HEX(const std::string &input);
std::string HEX2String(const std::string &input);
std::string Crypt(const std::string &data);

// example:
//recursive_iterate(j, [](nlohmann::json::const_iterator it)
//{
//	std::cout << *it << std::endl;
//});
//
void recursive_iterate(const nlohmann::json &j, std::function<bool(nlohmann::json::const_iterator)> f);
