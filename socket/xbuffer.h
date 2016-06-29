// Author: Hongzhang Liu
// Date:

#ifndef SWIFT_NET_XBUFFER_H__
#define SWIFT_NET_XBUFFER_H__

#include "cocos_socket.h"

#include <stdint.h>
#include <cstdlib>
#include <cstring>

class XBuffer
{
public:
    /// @param size default size of the buffer
    explicit XBuffer(int size);
    ~XBuffer();

    /// reset buffer to empty
    void Clear() { wpos_ = rpos_ = 0; }

    /// to guarantee WriteableBytes() >= @c
    void EnsureCapacity(int c);

    /// call when we have read @len bytes from this buffer
    void ConsumeBytes(int len);

    /// call when we have writen @len bytes to this buffer
    void GotBytes(int len) { wpos_ += len; }

    /// how many bytes can we write to this buffer
    int WriteableBytes() { return size_ - wpos_; }
    /// how many bytes can we read from this buffer
    int ReadableBytes() { return wpos_ - rpos_; }

    /// write pos
    char *WritePos() { return buf_ + wpos_; }
    /// read pos
    char *ReadPos() { return buf_ + rpos_; }

    /// tidy
    void Tidy();

    /// peek @len bytes to @data
    void Peek(void *data, int len);

    /// retrive @len bytes to @data
    void Retrive(void *data, int len);

    /// append data
    void Append(const void *data, int len);

    /// read from @fd
    /// @retval >0 read succeed or EAGAIN/EINTR
    /// @retval 0  connection is closed by peer
    /// @retval <0 something is wrong, -errno is returned
    int ReadFrom(cocos_socket_t fd);

    /// write to @fd
    /// @retval >0 bytes written
    /// @retval 0  EAGAIN or EINTR
    /// @retval <0 something is wrong, -errno is returned
    int WriteTo(cocos_socket_t fd);

private:
    char *buf_;
    int size_;
    int wpos_;
    int rpos_;

	XBuffer(const XBuffer &);
	XBuffer & operator =(const XBuffer &);
};

#endif

