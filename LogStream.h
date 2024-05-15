#pragma once
#include "Timestamp.h"
#include <sstream>
#include <string>
#include <mutex>
#include <fstream>

// 定义日志级别 INFO  ERROR  FATAL  DEBUG
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

#define LOG_INFO LogStream(INFO)
#define LOG_ERROR LogStream(ERROR)
#define LOG_FATAL LogStream(FATAL)
#define LOG_DEBUG LogStream(DEBUG)

// 每个LogStream对象都是一行Log日志
// 在对象析构的时候存入文件

class LogStream
{
public:
    LogStream(int logLevel);

    // 在此处将ss_中所有内容写入文件
    ~LogStream();

    // 代理到std::stringstream的函数
    template <typename T>
    LogStream &operator<<(const T &value);

private:
    std::stringstream ss_; // 组合一个stringstream对象
    // 静态变量，全局只有一个，在多线程的多个实例化对象中也可保持线程安全
    static std::mutex fileMutex;
    int logLevel_;
};

// 代理到std::stringstream的函数
template <typename T>
LogStream& LogStream::operator<<(const T &value)
{
    ss_ << value;
    return *this;
}