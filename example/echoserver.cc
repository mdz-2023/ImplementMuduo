#include <mymuduo/TcpServer.h>
#include <mymuduo/LogStream.h>

#include <string>
#include <functional>
#include <iostream>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name), loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, 
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置合适的loop线程数量
        // server_.setThreadNum(3); // 设置线程数量一般与CPU核数相等，还有另外的本线程，项目共4个线程。 

        // LOG_DEBUG << "EchoServer create success!";
    }

    void start(){
        server_.start();
    }

private:
    // 连接建立或者断开的回调
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->connected()){
            LOG_INFO << "Connection UP : " << conn->peerAddress().toIpPort();
        }
        else{
            LOG_INFO << "Connection DOWN : " << conn->peerAddress().toIpPort();
        }
    }

    // 可读写事件的回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time){
        std::string msg = buf->retrieveAllAsString();
        std::cout << "server recieved: " << msg;
        conn->send(msg);
        conn->shutdown(); // 关闭写端  底层epoll产生EPOLLHUP事件 =》 closeCallback_ 删除连接
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main(){
    EventLoop loop;
    InetAddress addr(8000, "0.0.0.0");

    EchoServer server(&loop, addr, "myEchoServer");

    server.start();
    loop.loop();
}