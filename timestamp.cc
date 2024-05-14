#include "timestamp.h"
#include <time.h>
#include <sstream>
#include <iomanip>  

Timestamp::Timestamp() : microSecondsSinceEpoch_(0)
{
}
Timestamp::Timestamp(int64_t microSecondsSinceEpochArg) : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
{
}
Timestamp Timestamp::now()
{
    // struct timeval tv;
    // gettimeofday(&tv, NULL);
    // int64_t seconds = tv.tv_sec;
    // return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
    time_t ti = time(NULL); // 获取当前时间
    return Timestamp(ti);
}
// return yyyy/MM/dd hh:mm:ss
std::string Timestamp::toString() const
{
    // C++写法，格式化输出太麻烦
    std::string time;
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    std::ostringstream oss;
    oss << std::setw(4) << (tm_time->tm_year + 1900) << "-"  
        << std::setw(2) << std::setfill('0') << (tm_time->tm_mon + 1) << "-"  
        << std::setw(2) << std::setfill('0') << tm_time->tm_mday << " "  
        << std::setw(2) << std::setfill('0') << tm_time->tm_hour << ":"  
        << std::setw(2) << std::setfill('0') << tm_time->tm_min << ":"  
        << std::setw(2) << std::setfill('0') << tm_time->tm_sec;
    time = oss.str();
    return time;

    // C语言写法
    char buf[128] = {0};
    sprintf(buf, "%4d-%02d-%02d %02d:%2d:%02d",
                tm_time->tm_year + 1900,
                tm_time->tm_mon + 1,
                tm_time->tm_mday,
                tm_time->tm_hour,
                tm_time->tm_min,
                tm_time->tm_sec);
    return buf;
}

Timestamp::~Timestamp()
{
}