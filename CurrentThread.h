#pragma once
#include <unistd.h>
       #include <sys/syscall.h>

namespace CurrentThread
{
    // __thread表示thread_local，限制为当前线程的全局变量
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}