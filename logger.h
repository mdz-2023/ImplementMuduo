#pragma once 
#include "noncopyable.h"
#include <string>
#include <ctime>
#include <fstream>
#include <sstream>

// #define LOG_INFO(LogmsgFormat, ...) \
//     do{ \
//         Logger &logger = Logger::getInstance(); \
//         Logger.setLogLevel(INFO); \
//         char buf[1024] = {0}; \
//         snprintf(buf, 1024, logmasFormat, ##__VA_ARGS__); \
//         logger.log(buf); \
//     } while(0)

// 定义日志级别 INFO  ERROR  FATAL  DEBUG
enum LogLevel{
    INFO,   // 普通信息
    ERROR,  // 错误信息 
    FATAL,  // core信息
    DEBUG,  // 调试信息
};

class Logger : noncopyable{
public:
    // 获取唯一实例
    static Logger* getInstance();
    
    // 设置日志级别
    void setLogLevel(int level);

    // 静态方法，方便用户使用
    static void LOG_INFO(std::string msg);
    static void LOG_ERROR(std::string msg);
    static void LOG_FATAL(std::string msg);
    static void LOG_DEBUG(std::string msg);
private:
    // 写日志
    static void log(int level, std::string msg);
    int logLevel_;
    Logger() = default;
};
