#include "cinotify.h"
#include "threadpool.h"
#include "mylog.h"
#include <unistd.h>
#include "channel.h"
#include <string.h>
#include <iostream>

using namespace std;
using namespace Heartbeat;
Cinotify::Cinotify()
 :epollfd_(epoll_create(1)),
  sock_("/tmp/unix_socket")
{
  events_.resize(64);
  callback_ = nullptr;
  sumConnect_ = 0;
  sumactive_ = 0;
  sumrecycle_ = 0;
}

Cinotify::~Cinotify()
{
  ::close(epollfd_);
  dischannel();
  untreateStr_.clear();
}

//主循环 循环epoll_wait()
void Cinotify::loop()
{
  while(1)
  {
    epollWait(-1);
  }
}

//从主要初始化函数几乎所有东西都是在这里面初始化的
bool Cinotify::init()
{
  recv_buf_ = std::make_shared<char*>(new char[RECV_BUF]);
  p_recv_buf_ = *recv_buf_;
  recv_buf_len_ = 0;
  c_ = 0;
  //线程池初始化
  threadpool_ = make_shared<Cthreadpool*>(new Cthreadpool(*this));
  (*threadpool_)->init(THREADPOLL_COUNT);
  callbackSize_ = (*threadpool_)->callbackSize_;
  //连接池初始化
  initchannel();

  //建立socket句柄
  int sockfd = -1;
  sockfd = sock_.socket();
  if(sockfd < 0)
  {
    return false;
  }

  //取出连接池里面的连接
  Channel *channel = popConnect();
  if(!channel)
    return false;
  
  //设置句柄和回调函数
  channel->setfd(sockfd);
  channel->setrcb(&Cinotify::listenrcb);
  
  //加入epoll中
  if(epollCtl(sockfd, EPOLL_CTL_ADD, 
              EPOLLIN|EPOLLRDHUP|EPOLLET, 
              0, channel) < 0)
  {
    return false;
  }
  return true;
}

//回到函数负责侦听回调
void Cinotify::listenrcb(Channel *channel)
{
  int fd = sock_.accept();
  if(fd < 0)
    return;
  
  Channel *c = popConnect();
  if(!c)
    return;
  c->setfd(fd);
  c->setrcb(&Cinotify::acceptrcb);

  if(epollCtl(fd, EPOLL_CTL_ADD, 
              EPOLLIN|EPOLLRDHUP|EPOLLET, 
              0, c) < 0)
  {
    return;
  }
}

//回收连接池new出来的内存
void Cinotify::dischannel()
{
  while(!recycle_.empty())
  {
    auto pos = recycle_.front();
    recycle_.pop_front();
    if(pos)
      delete pos;
  }
}

//初始换连接池
void Cinotify::initchannel()
{
  for(int i = 0; i < CONNECTS; ++i)
  {
    Channel *channel = new Channel;
    ++sumConnect_;
    ++sumrecycle_;
    //可用连接
    connect_.push_back(channel);
    //回收连接
    recycle_.push_back(channel);
  }
  logg("The number of connection pools is CONNECTS int Cinotify::dischannel\n");
}

//取出一个连接池的一个连接
Channel *Cinotify::popConnect()
{
  if(sumConnect_ > 0)
  {
    auto pos = connect_.front();
    --sumConnect_;
    connect_.pop_front();
    return pos;
  }
  else if(sumrecycle_ < CONNECTS_MAX)
  {
    Channel *channel = new Channel;
    ++sumrecycle_;
    recycle_.push_back(channel);
    return channel;
  }
  else
  {
    logg("The number of connection pools is greater than CONNECTS_MAX in Cinotify::popConnect\n");
    return nullptr;
  }
}

//此函数只在主线程中调用负责回收连接池里面的连接
void Cinotify::closechannel(Channel *channel)
{
  int fd = channel->fd();
  if(fd < 0)
    return;
  
  //断开连接是否有没插入的数据进行插入
  storage();
  if(active_[fd] == channel)
  {
    //活跃连接断开时释放
    active_.erase(fd);
    --sumactive_;
  }

  //接受文件是如果没收完整时的容器
  untreateStr_[fd].clear();
  untreateStr_.erase(fd);
  //关闭套接字
  channel->close();
  //加入空闲队列中
  connect_.push_back(channel);
  ++sumConnect_;
  
}

//活跃连接MAP对应的里面的vector是否有数据
void Cinotify::storage()
{
  ConnectMap::iterator posBegin = active_.begin();
  ConnectMap::iterator posEnd = active_.end();
  for(posBegin; posBegin != posEnd; ++posBegin)
  {
    if(posBegin->second->accessPlist_.size() <= 0)
    {
      continue;
    }
    else
    {
      int protocol = posBegin->second->protocol();
      if(callback_ == nullptr)
        continue;
      else if(protocol < 0 || protocol > callbackSize_)
        continue;
      else
        callback_->distribution(posBegin->second);
    }
  }
}

//处理线程入口
void Cinotify::storage(Sthread *ps)
{
  int protocol = -1;
  ConnectMap::iterator posBegin;
  ConnectMap::iterator posEnd;
  while(1)
  {
pos:
    this_thread::sleep_for(chrono::milliseconds(200));

    posBegin = active_.begin();
    posEnd = active_.end();
    for(posBegin; posBegin != posEnd; ++posBegin)
    {
      
      if(posBegin->second->accessPlist_.size() <= 0)
        continue;
      else
      {
        protocol = posBegin->second->protocol();
        if(callback_ == nullptr)
          goto pos;
        else if(protocol < 0 || protocol > callbackSize_ )
          continue;
        else
          callback_->distribution(posBegin->second);
      }
    }
  }
}

int Cinotify::epollCtl(int fd, uint32_t eventtype, 
        uint32_t flag, int bcaction, Channel *channel)
{
  struct epoll_event ev;    
  memset(&ev, 0, sizeof(ev));

  if(eventtype == EPOLL_CTL_ADD)
  {
    ev.events = flag;
    channel->setevent(flag);
  }
  else if(eventtype == EPOLL_CTL_MOD)
  {
    /*0增加标记，1删除标记，2完全覆盖 */
    ev.events = channel->event();
    if(bcaction == 0)
      ev.events |= flag;
    else if(bcaction == 1)
      ev.events &= ~flag;
    else
      ev.events = flag;            
    channel->setevent(ev.events);
  }
  else
  {
    return  0;
  } 

  ev.data.ptr = channel;
  if(::epoll_ctl(epollfd_, eventtype, fd, &ev) < 0)
  {
    logg("epoll_ctl errno is %d in Cinotify::epollCtl\n", errno);
    return -1;
  }
  return 0;
}

int Cinotify::epollWait(int timer)
{
  int numEvents = ::epoll_wait(epollfd_, 
                            &*events_.begin(), 
                            static_cast<int>(events_.size()), 
                            timer);
  if(numEvents < 0)
  {
    if(errno != EINTR) 
    {
      logg("epoll_wait errno is %d in Cinotify::epollWait\n", errno); 
      return -1;
    }
  }
  if(0 == numEvents)
  {
    /*超时*/
    return 0;
  }

  //二倍增长vector
  if (static_cast<size_t>(numEvents) == events_.size())
  {
    events_.resize(events_.size()*2);
  }
  
  Channel *channel = nullptr;
  uint32_t revents = 0;

  for(int i = 0; i < numEvents; ++i)
  {
    channel = (Channel*)(events_[i].data.ptr);
    revents = events_[i].events;

    //程序关键点活跃连接数加入map中
    active_[channel->fd()] = channel;
    ++sumactive_;

    if(revents & EPOLLIN)
    {
      if(channel->readcb != nullptr)
        (this->*(channel->readcb))(channel);
    }
    if(revents & EPOLLOUT)
    {
      //客户端关闭，如果服务器端挂着一个写通知事件，则这里个条件是可能成立的
      if(revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
      {
        //有机会在做处理吧
        return 0;
      }
      else
      {
        if(channel->writecb != nullptr)
          (this->*(channel->writecb))(channel);
      }
      
    }
  } 
  return 0;
}

ssize_t Cinotify::recv(Channel *channel, void *container, int len)
{
  ssize_t n = ::recv(channel->fd(), container, len, 0);
  if(n == 0)
  {
    //客户端断开链接
    //close之后该文件描述符就会从epoll干掉
    logg("The client disconnects fd is %d in Cinotify::recv\n", channel->fd());
    closechannel(channel);
    return -1;
  }
  if(n < 0)
  {
    //此调用被信号所中断
    if(EINTR == errno)
    {
      logg("readFileSize::read() EINTR == errno in Cinotify::recv\n");
      return -1;
    }
    //当使用非阻塞I/O 时(O_NONBLOCK), 若无数据可读取则返回此值
    if(EAGAIN == errno)
    {
      logg("readFileSize::read() EAGAIN == errno in Cinotify::recv\n");
      return -1;
    }
    //参数fd 非有效的文件描述词, 或该文件已关闭
    if(EBADF == errno)
    {
      logg("readFileSize::read() EBADF == errno in Cinotify::recv\n");
      return -1;
    }
  }
  return n;
}

ssize_t Cinotify::send(Channel *channel, void *container, int len)
{
  ssize_t n = 0;
  for ( ;; )
  {
    n = ::send(channel->fd(), container, len, 0);
    if(n > 0)
    {
      return n;
    }
    if(n == 0 )
    {
      return 0;
    }
    if(errno == EAGAIN)
    {
      //内核缓冲区满了
      return -1;
    }
    if(errno == EINTR)
    {
      logg("send errno == EINTR in Cinotify::send\n");
    }
    else
    {
      logg("send errno is %d in Cinotify::send\n", errno);
      return -2;
    }
  }
}

//只在主线程中调用
ssize_t Cinotify::pushlist(Channel *channel)
{
  int bodyLen = 0, n = recv_buf_len_;
  int headsize = sizeof(Shead);

  while(1)
  {
    Shead *phead = (Shead *)p_recv_buf_;
    bodyLen = phead->len;
    if(bodyLen <= 0)
      break;

    if(phead->protocol >= 0 && phead->protocol <= callbackSize_)
      channel->setprotocol(phead->protocol);
    else
      break;
    
    if(n <= bodyLen)
    {
      p_recv_buf_ = (*recv_buf_) + (recv_buf_len_ - n - headsize);
      c_ = p_recv_buf_[n - 1];
      p_recv_buf_[n - 1] = '$';
      untreateStr_[channel->fd()] = string(p_recv_buf_, n + headsize);
      p_recv_buf_ = *recv_buf_;
      recv_buf_len_ = 0;
      return n;
    }

    n -= headsize;
    p_recv_buf_ += headsize;
    shared_ptr<Sbody> body = make_shared<Sbody>(Sbody(channel, 
                             make_shared<string>(string(p_recv_buf_, bodyLen))));
    //加锁加入队列中
    mtx_.lock();
    datalist_.push_back(body);
    mtx_.unlock();
    //唤醒线程处理数据
    (*threadpool_)->call();
    
    n -= bodyLen;
    p_recv_buf_ += bodyLen;
    if(n <= 0)
    {
      p_recv_buf_ = *recv_buf_;
      recv_buf_len_ = 0;
      return 0;
    }
    else if(n <= headsize && n > 0)
    {
      p_recv_buf_ = (*recv_buf_) + (recv_buf_len_ - n);
      c_ = p_recv_buf_[n - 1];
      p_recv_buf_[n - 1] = '$';
      untreateStr_[channel->fd()] = string(p_recv_buf_, n);
      p_recv_buf_ = *recv_buf_;
      recv_buf_len_ = 0;
      return 0;
    }
    else
    {
      continue;
    }
  }
  logg("A suspicious package body,Discard length is == %d in Cinotify::pushlist\n", n);
  return 0;
}

//只在主线程中调用 精华收包代码ET模式
void Cinotify::acceptrcb(Channel *channel)
{
  ssize_t ret = 0;
  int fd = channel->fd();
  if(fd == -1)
    return;

  int headsize = sizeof(Shead);
  while(1)
  {
    ret = untreateStr_[fd].size();
    if(ret > 0)
    {//恢复结尾保证数据完整性
      char *tail = (char*)untreateStr_[fd].c_str();
      tail[ret - 1] = c_;
      memcpy((*recv_buf_), tail, ret);
      untreateStr_[fd].clear();
    }
    
    recv_buf_len_ = recv(channel, (*recv_buf_) + ret, RECV_BUF - ret);
    if(recv_buf_len_ <= 0)
      break;
    
    if(recv_buf_len_ + ret <= headsize && recv_buf_len_ + ret > 0)
    { //第一个是结构的时候转换成char* 的时候最后一个字符很可能是零结尾做一个标记作用取出时恢复
      c_ = p_recv_buf_[recv_buf_len_ - 1]; 
      p_recv_buf_[recv_buf_len_ - 1] = '$';
      untreateStr_[fd] = string(p_recv_buf_, recv_buf_len_);
      p_recv_buf_ = *recv_buf_;
      recv_buf_len_ = 0;
      continue;
    }
    
    if(pushlist(channel) <= 0)
      break;
    if(recv_buf_len_ < RECV_BUF - ret)
      break;
  }

}

//写回调函数
void Cinotify::acceptwcb(Channel *channel)
{

}



