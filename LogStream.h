#pragma once
#include <sstream>
#include <fstream>

class LogStream {
private:  
    std::stringstream ss_; // 组合一个stringstream对象  
    std::fstream* fs_;
    int logLevel_;
public:  
    LogStream(fstream* fs) : fs_(fs){}

    // 在此处将ss_中所有内容写入文件
    ~LogStream(){
        if (fs_.is_open())
        {
            fs_ << ss_.str();
            fs_.close();
        }
        else
        {
            std::cout << "error : log file not open!" << std::endl;
            return;
        }
    }

    // 代理到std::stringstream的函数 
    template<typename T>   
    MyStringStream& operator<<(const T& value) {  
        ss_ << value;  
        return *this;  
    }  

    // 获取stringstream的内容  
    std::string str() const {  
        return ss_.str();  
    }  
};