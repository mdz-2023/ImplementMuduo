#pragma once

#include "noncopyable.h"
#include "Socket.h"
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

    // TcpServer的构造函数中设置的TcpServer::newConnection方法
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = std::move(cb);
    }

    void listen();

    bool listenning() const { return listenning_; }

private:
    void handleRead();

    EventLoop *loop_; // Acceptor用的就是用户定义的那个baseloop，也即mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_; // 在TcpServer::TcpServer()中设置，将新连接打包
    bool listenning_;
};
