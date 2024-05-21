#pragma once

#include "noncopyable.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "InetAddress.h"
#include "Callbacks.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

// 已连接socket的通信链路
/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * => TcpConnetion 设置回调 => Channel => Poller => Channel的回调操作
 * 专门描述一个已建立连接的相应信息 
*/
class TcpConnection: noncopyable,
                      public std::enable_shared_from_this<TcpConnection> // 运行本类的对象产生智能指针
{
public:
    TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    std::string getTcpInfoString() const;


    // 用户调用，给客户端发送数据
    void send(const std::string& message);
    // 关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    // called when TcpServer accepts a new connection
    // 连接建立
    void connectEstablished();   // should be called only once
    // called when TcpServer has removed me from its map
    // 连接销毁
    void connectDestroyed();  // should be called only once

private:
    //            初始化       建立连接    调用shutdown      关闭完底层soket
    enum StateE { kConnecting, kConnected, kDisconnecting,  kDisconnected};

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();
    
    const char* stateToString() const;
    // void startReadInLoop();
    // void stopReadInLoop();

    void setState(StateE s) { state_ = s; }

    EventLoop* loop_; // TcpConnection都是在subLoop里管理的
    const std::string name_;
    std::atomic_int state_; 
    bool reading_;

    // 这里和mainLoop中的Acceptor很像，而TcpConnection在subLoop中
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_; // 来自TcpServer的回调，用于在Server中删除本conn

    size_t highWaterMark_; // 防止本端发送过快对面接收不及

    Buffer inputBuffer_; // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
};


