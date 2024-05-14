#include "Poller.h"
#include <sys/epoll.h>

/*
epoll的使用
epoll_create 创建fd
epoll_ctl  add/mod/del
epoll_wait
*/

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override; // override 表示由编译器确保父类的同名函数是虚函数

    // 重写基类的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16; // EventList初始长度

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    // 更新channel通道
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};