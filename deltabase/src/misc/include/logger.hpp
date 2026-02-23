//
// Created by poproshaikin on 10.12.25.
//

#ifndef DELTABASE_LOGGER_HPP
#define DELTABASE_LOGGER_HPP
#include <chrono>
#include <iostream>

namespace misc
{
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    class Logger
    {
        static std::string
        getNowStr()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&t);

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch()) % 1000;

            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        static std::string
        getLevelStr(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARN:
                return "WARN";
            case LogLevel::ERROR:
                return "ERROR";
            default:
                return "";
            }
        }

    public:
        static void
        log(const std::string& msg, LogLevel level)
        {
            std::cout << getLevelStr(level) << " " << getNowStr() << std::endl;
            std::cout << msg << std::endl;
            std::cout << std::endl;
        }

        static void
        debug(const std::string& msg)
        {
            log(msg, LogLevel::DEBUG);
        }

        static void
        info(const std::string& msg)
        {
            log(msg, LogLevel::INFO);
        }

        static void
        warn(const std::string& msg)
        {
            log(msg, LogLevel::WARN);
        }

        static void
        error(const std::string& msg)
        {
            log(msg, LogLevel::ERROR);
        }
    };
}

#endif //DELTABASE_LOGGER_HPP