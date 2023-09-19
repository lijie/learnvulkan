#pragma once

#include <iostream>
#include <format>

#define DEBUG_LOG(fmt, ...) std::cout << std::format("[{}] " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
