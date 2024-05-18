#pragma once

#include <vector>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///                      模仿
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |      缓冲区头     |     (CONTENT)    |     可写空间     |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

// 网络库底层的缓冲区类型
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;   // 缓冲区头长度
    static const size_t kInitialSize = 1024; // 默认初始化大小

    Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
    }
    ~Buffer() = default;
    
    // writerIndex_ - readerIndex_
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    // buffer_.size() - writerIndex_
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    // return readerIndex_
    size_t prependableBytes() const
    {
        return readerIndex_; // 随着上层的读取，readerIndex_会变
    }

    // 返回第一个可读的数据地址
    const char *peek() const
    {
        // char * + readerIndex_
        return begin() + readerIndex_;
    }

    // 将readerIndex_向后移动已经读走的len长度的字节
    void retrieve(size_t len)
    {
        if (len < readableBytes())// 长度小于可读大小
        {
            readerIndex_ += len;
        }
        else // 否则即为读取所有
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len); // 第一个可读数据的地址开始的len长度
        retrieve(len);
        return result;
    }


    /// @brief 从原位置data处开始的len个字节数据追加到缓冲区中
    /// @param data 源数据位置
    /// @param len 要追加的字节个数
    void append(const char* /*restrict*/ data, size_t len)
    {
        ensureWritableBytes(len); // 确保容量足够
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    void append(const void* /*restrict*/ data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
        makeSpace(len);
        }
    }

    char* beginWrite(){
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    { return begin() + writerIndex_; }

    // 通过fd读取数据：从fd中read数据，写到缓冲区中的writable区域
    ssize_t readFd(int fd, int* savedErrno);
    // 通过fd发送数据：将buffer的readable区域的数据，写入到fd中
    ssize_t writeFd(int fd, int* savedErrno);

private:
    char* begin()
    { return &*buffer_.begin(); }

    const char *begin() const
    {
        // *buffer_.begin()获取到vector第一个元素，使用&取地址
        return &*buffer_.begin();
    }

    // buffer扩容
     void makeSpace(size_t len)
    {
        // 若可写大小 + 0~可读位置 不够 要写的数据长度，则申请空间
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
        // FIXME: move readable data
        buffer_.resize(writerIndex_+len);
        }
        else // 否则不申请空间，将未读的数据向前移动到紧挨包头之后
        {
        // move readable data to the front, make space inside buffer
        size_t readable = readableBytes(); // 还未读的数据长度
        std::copy(begin()+readerIndex_,
                    begin()+writerIndex_,
                    begin()+kCheapPrepend); // 紧挨包头之后
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};