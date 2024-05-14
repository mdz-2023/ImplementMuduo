#pragma once

/*
noncopyable 被继承以后，派生类对象可以正常构造和析构
                        但是不可用进行拷贝构造和赋值
*/

class noncopyable{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable) = delete;
protected: // 可以被派生类访问
    noncopyable() = default;
    ~noncopyable() = default;
};