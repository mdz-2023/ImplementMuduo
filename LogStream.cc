#include "LogStream.h"

std::mutex LogStream::fileMutex;

LogStream::LogStream(const int logLevel, const char *file, const int line, const char* func)
    : logLevel_(logLevel)
{
    ssEnd_ << " --FILE:" << file << " LINE:" << line << " FUNC:" << func;
}

// 在此处将ss_中所有内容写入文件
LogStream::~LogStream()
{
    if (ss_.str().empty())
        return;
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

    {
        std::unique_lock<std::mutex> ulock(fileMutex);
        // C++ 文件流
        std::ofstream file(filePath, std::ios::app);
        if (file.is_open())
        {
            file << output << time << " " << ss_.str() << ssEnd_.str() << std::endl;
            file.close();
        }
        else
        {
            // std::cout << "error : log file not open!" << std::endl;
            return;
        }
    }

    if (logLevel_ == FATAL)
    {
        exit(-1);
    }
}
