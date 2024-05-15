#include "logger.h"
#include "timestamp.h"
#include <iostream>

int Logger::logLevel_ = 0;

Logger& Logger::getInstance(int logLevel)
{
    static Logger logger;
    logLevel_ = logLevel;
    return logger;
}

// void Logger::setLogLevel(int level)
// {
//     logLevel_ = level;
// }

// 写日志  [级别信息] time : msg
void Logger::log(std::string msg)
{
    std::string output;
    switch (logLevel_)
    {
    case INFO:
        output += "[INFO] ";
        break;
    case ERROR:
        output += "[ERROR]";
        break;
    case FATAL:
        output += "[FATAL]";
        break;
    case DEBUG:
        output += "[DEBUG]";
        break;
    default:
        break;
    }

    std::string time = Timestamp::now().toString();

    std::string filePath = time.substr(0, 10) + "-log.txt";

    // C++ 文件流
    std::ofstream file(filePath, std::ios::app);
    if (file.is_open())
    {
        file << output << time << " " << msg << std::endl;
        file.close();
    }
    else
    {
        // std::cout << "error : log file not open!" << std::endl;
        return;
    }
    if(logLevel_ == FATAL) {
        exit(-1);
    }
}

// void Logger::LOG_INFO(std::string msg)
// {
//     log(INFO, msg);
// }
// void Logger::LOG_ERROR(std::string msg)
// {
//     std::cout << msg;
//     log(ERROR, msg);
// }
// void Logger::LOG_FATAL(std::string msg)
// {
//     std::cout << msg;
//     log(FATAL, msg);
//     // exit(-1);
// }
// void Logger::LOG_DEBUG(std::string msg)
// {
// #ifndef MUDUO_DEBUG
//     log(DEBUG, msg);
// #endif
// }



// int main(){
//     int a = 18;
//     float pi = 3.14;
//     LOG_DEBUG << "wo shi hao ren:" << a << "pi = " << pi << std::endl;
//     LOG_INFO << "wo shi hao ren:" << a << "pi = " << pi << std::endl;
//     LOG_ERROR << "wo shi hao ren:" << a << "pi = " << pi << std::endl;
//     LOG_FATAL << "wo shi hao ren:" << a << "pi = " << pi << std::endl;
//     return 0;
// }