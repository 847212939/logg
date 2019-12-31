#pragma once
#include "csocket.h"
#include <functional>
#include <sys/epoll.h>
#include <memory>
#include <string>
#include <atomic>
#include <vector>
#include <mutex>
#include <list>
#include <map>

#define CONNECTS 64
#define CONNECTS_MAX 10000
#define THREADPOLL_COUNT 16
#define RECV_BUF 1024*1024

namespace Heartbeat
{
typedef class Channel Channel;
typedef struct Sthread Sthread;
typedef class Callback Callback;
typedef class Cthreadpool Cthreadpool;
struct Shead
{
  int protocol;
  int len;
  Shead():protocol(-1), len(-1) {}
};

struct Sbody
{
  std::shared_ptr<std::string> str_;
  Channel *channel_;
  Sbody(Channel *channel, std::shared_ptr<std::string> str) :channel_(channel), str_(str) {}
};

class Cinotify
{
  typedef std::list<Channel*> ConnectList;
  typedef std::map<int, Channel*> ConnectMap;
  typedef std::map<int, std::string> untreatedMap;
  typedef std::vector<struct epoll_event> EventList;

public:
  typedef void(Cinotify::*FN)(Channel*);

public:
  Cinotify();
  virtual ~Cinotify();

public:
  bool init();
  void loop();
  void closechannel(Channel *channel);
  void storage(Sthread *ps);
  void setCallback(Callback *ca) { callback_ = ca; }

public:
  std::list<std::shared_ptr<Sbody>> datalist_;
  std::mutex mtx_;
  
private:
  int epollWait(int timer);
  int epollCtl(int fd, 
              uint32_t eventtype, 
              uint32_t flag, 
              int bcaction, 
              Channel *channel
              );
              
  Channel *popConnect();
  void dischannel();
  void initchannel();

  void listenrcb(Channel *channel);
  void acceptrcb(Channel *channel);
  void acceptwcb(Channel *channel);
  ssize_t pushlist(Channel *channel);

  ssize_t recv(Channel *channel, void *container, int len);
  ssize_t send(Channel *channel, void *container, int len);
  void storage();

private:
  std::shared_ptr<Cthreadpool*> threadpool_;
  std::shared_ptr<char*> recv_buf_;
  Callback *callback_;
  EventList events_;
  CSocket sock_;
  ConnectMap active_;
  ConnectList connect_;
  ConnectList recycle_;
  untreatedMap untreateStr_;
  int sumactive_;
  int sumConnect_;
  int sumrecycle_;
  char *p_recv_buf_;
  int recv_buf_len_;
  char c_;
  int epollfd_;
  int callbackSize_;

};//end Cinotify
}//end Heartbeat