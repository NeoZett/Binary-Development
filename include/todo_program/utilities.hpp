#pragma once

#include <chrono>
#include <stdexcept>

static std::string current_timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};

#ifdef _WIN32
    if (gmtime_s(&tm, &time) != 0)
        throw std::runtime_error("Couldn't get UTC time.");
#else
    if (gmtime_r(&time, &tm) == nullptr)
        throw std::runtime_error("Couldn't get UTC time.");
#endif

    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return stream.str();
}