#include "threadpool.h"
#include "mylog.h"
#include <iostream>
#include "channel.h"

using namespace std;
using namespace Heartbeat;
bool Cthreadpool::shutDown = false;
Cthreadpool::CALLBACK Cthreadpool::callbacks_[] = 
{
  &Callback::audit,
  &Callback::machine,
  &Callback::haproxy,
};

bool Cthreadpool::init(int num)
{
  cinotify_.setCallback(&callback_);
  callbackSize_ = sizeof(callbacks_)/sizeof(callbacks_[0]);

  for(int i = 0; i < num; ++i)
  {
    Sthread *ps = new Sthread;
    thread *pt = nullptr;
    if(i == num - 1)
      pt = new thread(&Cthreadpool::storage, this, ps);
    else
      pt = new thread(&Cthreadpool::entrance, this, ps);
    ps->th_ = pt;
    threadlist_.push_back(ps);
  }

lblfor:

  vector<Sthread*>::iterator pos = threadlist_.begin();
  for(; pos != threadlist_.end(); ++pos)
  {
    if((*pos)->off_ == false)
    {
      /*1毫秒=1000微妙 */
      usleep(100 * 1000);
      goto lblfor;
    }
  }
  logg("Thread pool initialization is complete in Cthreadpool::init\n");    
  return true;
}

void Cthreadpool::entrance(Sthread *ps)
{
  while(true)
  {
    unique_lock<mutex> lck(cinotify_.mtx_, defer_lock);
    lck.lock();
    while(0 == cinotify_.datalist_.size() && false == shutDown)
    {
      if(ps->off_ == false)
        ps->off_  = true;
      condition_.wait(lck);
    }

    if(true == shutDown)
    {
      lck.unlock();
      break;
    }

    auto body = cinotify_.datalist_.front();
    cinotify_.datalist_.pop_front();
    lck.unlock();

    ++count_;
    int protocol = body->channel_->protocol();
    if(protocol >= 0 && protocol <= callbackSize_)
      (callback_.*(callbacks_[protocol]))(body);
    --count_;
  }
}

void Cthreadpool::storage(Sthread *ps)
{
  if(ps->off_ == false)
    ps->off_  = true;
  cinotify_.storage(ps);
}

void Cthreadpool::call()
{
  condition_.notify_one();
  if(count_ == THREADPOLL_COUNT)
  {
    time_t currtime = time(NULL);
    if(currtime - lasttime_ > 10)
    {
      lasttime_ = currtime;
      //logg("The number of threads is too small to be busy in Cthreadpool::call\n");
    }
  }
}

Cthreadpool::~Cthreadpool()
{
  vector<Sthread*>::iterator pos = threadlist_.begin();
  for(; pos != threadlist_.end(); ++pos)
  {
    if(*pos)
    {
      if((*pos)->th_)
      {
        (*pos)->th_->join();
        delete (*pos)->th_;
      }
      delete *pos;
    }
  }
  logg("Thread exits normally in Cthreadpool::~Cthreadpool\n");
  threadlist_.clear();
}

