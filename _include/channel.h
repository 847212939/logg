#pragma once
#include "cinotify.h"
#include <unistd.h>
#include "parsestr.h"

namespace Heartbeat
{
class Channel
{
public:
  Channel()
  {
    fd_ = -1;
    events_ = 0;
    protocol_ = 0;
    readcb = nullptr;
    writecb = nullptr;
  }

  virtual ~Channel() 
  {
    if(fd_ > 0)
      ::close(fd_);
  }

  void close()
  {
    ::close(fd_);
    fd_ = -1;
    events_ = 0;
    protocol_ = 0;
    readcb = nullptr;
    writecb = nullptr;
  }

public:
  void setfd(int fd) { fd_ = fd; }
  void setrcb(Cinotify::FN cb) { readcb = cb; }
  void setwcb(Cinotify::FN cb) { writecb = cb; }
  void setevent(uint32_t events) { events = events_; }
  void setprotocol(int protocol) { protocol_ = protocol; }
  uint32_t event() { return events_; }
  int protocol() { return protocol_; }
  int fd() { return fd_; }

public:
  std::vector<std::shared_ptr<accesses>> accessPlist_;
  std::mutex channelMtx_;
  Cinotify::FN readcb;
  Cinotify::FN writecb;

private:
  uint32_t events_;
  int protocol_;
  int fd_;
};//end Channel
}//end Heartbeat