#pragma once

#include <iostream>
#include <format>

#define LVK_LOG(fmt, prefix, ...) std::cout << std::format("[{}][{}] " fmt "\n", prefix, __FUNCTION__, ##__VA_ARGS__)
#define DEBUG_LOG(fmt, ...) LVK_LOG(fmt, "DEBUG", ##__VA_ARGS__)
#define ERROR_LOG(fmt, ...) LVK_LOG(fmt, "ERROR", ##__VA_ARGS__)
