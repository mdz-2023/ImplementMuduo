#pragma once
#include "noncopyable.h"
#include <string>
#include <string.h>
#include <ctime>
#include <fstream>
#include <sstream>

// LOG_INFO << "abcd" << (int)x;

#define LOG_INFO Logger::getInstance(INFO)
#define LOG_ERROR Logger::getInstance(ERROR)
#define LOG_FATAL Logger::getInstance(FATAL)
#define LOG_DEBUG Logger::getInstance(DEBUG)

// 定义日志级别 INFO  ERROR  FATAL  DEBUG
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

class Logger : noncopyable
{
public:
    // 获取唯一实例
    static Logger &getInstance(int logLevel);

    // 重载 << 运算符
    template <typename T>
    Logger &operator<<(const T &value)
    {
        // std::lock_guard<std::mutex> lock(mtx_); // 锁定互斥量
        stream_ << value;
        return *this;
    }
    // 特定于 std::endl 的 operator<<
    Logger &operator<<(std::ostream &(*func)(std::ostream &))
    {
        // 输入了换行符，写入文件并清空流
        log(stream_.str());
        stream_.str(""); // 清空字符串流
        stream_.clear(); // 清除可能设置的任何错误标志
        return *this;
    }

    // // 设置日志级别
    // void setLogLevel(int level);

private:
    // 写日志
    static void log(std::string msg);
    
    static int logLevel_;
    std::ofstream file_;
    std::stringstream stream_;
    Logger() = default;
};
