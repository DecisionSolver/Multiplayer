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
#include <asio.hpp>

#include <cryptlib.h>
#include <md5.h>
#include <files.h>
#include <hex.h>

using namespace std::chrono_literals;
using asio::ip::tcp;
// for convenience
using json = nlohmann::json;

#if __has_include("logger.h")
#include "logger.h"
#define HAS_LOGGER 1
#endif

const std::string md5_from_file(const std::string &path);
const std::string md5_from_buffer(const std::string &data);
