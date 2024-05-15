#pragma once
#include "noncopyable.h"
#include <iostream>
#include <string>

class Timestamp
{
private:
    int64_t microSecondsSinceEpoch_;

public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch); // explicit 不允许隐式类型转换
    static Timestamp now();
    std::string toString() const;

    ~Timestamp();
};
