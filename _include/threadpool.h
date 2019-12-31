#pragma once
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "cinotify.h"
#include "callback.h"

namespace Heartbeat
{
struct Sthread
{
  bool off_;
  std::thread *th_;
  Sthread():off_(false) {}
};

class Cthreadpool
{
typedef void (Callback::*CALLBACK)(std::shared_ptr<Sbody>&);
public:
  void call();
  bool init(int num);

public:
  Cthreadpool(Cinotify &cinotify):cinotify_(cinotify)
  {
    count_ = 0;
    lasttime_ = 0;
    callbackSize_ = 0;
  }
  virtual ~Cthreadpool();

public:
  static CALLBACK callbacks_[];
  static bool shutDown;
  int callbackSize_;

private:
  void entrance(Sthread *ps);
  void storage(Sthread *ps);

private:
  std::vector<Sthread*> threadlist_;
  std::condition_variable condition_;
  Callback callback_;
  std::atomic<int> count_;
  Cinotify &cinotify_;
  time_t lasttime_;

};//end Cthreadpoll
}//end Heartbeat

