#include "Acceptor.h"
#include "InetAddress.h"
#include "LogStream.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// 创建一个非阻塞的sockfd
static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP); // SOCK_CLOEXEC标志子进程不接受父进程
    if (sockfd < 0)
    {
        LOG_FATAL << "sockets::createNonblockingOrDie error, errno:" << errno;
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort)
    : loop_(loop)
    , acceptSocket_(createNonblocking()) // 创建套接字socket
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true); // 设置tcp选项
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // bind
    // TcpServer::start()方法中，调用Acceptor.listen()
    // baseLoop =》 acceptChannel(listendfd)
    // 有新用户连接，要执行一个回调，connfd->channel->subloop
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this)); // acceptChannel_只关心读事件
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll(); // 取消fd所有的事件状态
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    LOG_DEBUG << "Acceptor::listen()";
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading(); // 将accpetorChannel注册在baseLoop的epoll
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr); // 获取到已连接addr
    LOG_DEBUG << "connfd=" << connfd << "  peerAddr:" << peerAddr.toIpPort();
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        { // 来了新连接，但是没有对应的回调函数，即无法为这个客户端服务，则直接关闭
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR << "Acceptor::handleRead():acceptSocket_.accept error, errno: " << errno;
        if (errno == EMFILE)
        {
            LOG_ERROR << "Acceptor::handleRead() error, sockfd reached limit!";
        }
    }
}
