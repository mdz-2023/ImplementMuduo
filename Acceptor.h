#pragma once

#include "noncopyable.h"
#include "Sokect.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

// Acceptor用的就是用户定义的那个baseloop，也即mainLoop
class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = std::move(cb);
    }

    void listen();

    bool listening() const { return listenning_; }

private:
    void handleRead();

    EventLoop *loop_; // Acceptor用的就是用户定义的那个baseloop，也即mainLoop
    Sokect acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};
