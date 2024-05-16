#pragma once

#include "noncopyable.h"
#include "LogStream.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "Callbacks.h"
#include "InetAddress.h"

#include <functional>
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>

// 对外的服务器编程使用的类
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    const std::string &ipPort() const { return ipPort_; }
    const std::string &name() const { return name_; }
    EventLoop *getLoop() const { return loop_; }

    void setThreadNum(int numThreads);

    //
    std::shared_ptr<EventLoopThreadPool> threadPool()
    {
        return threadPool_;
    }

    /// Starts the server if it's not listening.
    void start();

    void setThreadInitCallback(const ThreadInitCallback &cb)
    {
        threadInitCallback_ = cb;
    }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // the acceptor loop, 用户定义的loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;              // 运行在mainLoop，监听新连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop pre thread

    ConnectionCallback connectionCallback_;       // 有新链接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
    ThreadInitCallback threadInitCallback_;       // loop线程初始化的回调

    std::atomic_int started_;
    int nextConnId_;

    ConnectionMap connections_; // 保存所有的连接
};
