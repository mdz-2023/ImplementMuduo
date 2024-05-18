#include "Buffer.h"
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>


// 
/**
 * 从fd中read数据，写到缓冲区中的writable区域
 * Poller工作在LT模式，数据不会丢
 * Buffer缓冲区有打小，但是从fd读数据的时候不知道tcp数据最终大小
 * 所以使用 iovec + extrabuf 使用额外空间
*/
ssize_t Buffer::readFd(int fd, int *savedErrno)
{
    char extrabuf[65536] = {0}; // 栈上64k额外数据空间，防止读fd的时候buffer_数组不够用

    /**
     * struct iovec
     * {
     *    void *iov_base;	// Pointer to data.  
     *    size_t iov_len;	// Length of data.  
     *  };
     * 将iovcnt个iovec传入ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
     * 可以在从fd读取的时候，顺序依次写入每个iovec中
    */
    iovec vec[2]; // 创建两个iovec
    const size_t writable = writableBytes(); // Buffer底层缓冲区剩余的可写大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1; 
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0){
        *savedErrno = errno;
    }
    else if(n < writable){ // Buffer底层缓冲区剩余的可写大小 已经足够
        writerIndex_ += n;
    }
    else{ // 使用了extrabuf，需要将其移动到Buffer中
        writerIndex_ = buffer_.size(); // buffer_已经写满
        // 将extrabuf中数据移动到buffer_中
        append(extrabuf, n - writable);
    }
    return n;
}

/**
 * 将buffer的readable区域的数据，写入到fd中
*/
ssize_t Buffer::writeFd(int fd, int *savedErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n <= 0){
        *savedErrno = errno;
    }
    return n;
}
