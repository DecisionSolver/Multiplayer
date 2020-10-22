#pragma once

#include "LZ4/lz4.h"
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

std::string GetLastErrorStr();

using namespace std::chrono_literals;
using asio::ip::tcp;
// for convenience
using json = nlohmann::json;