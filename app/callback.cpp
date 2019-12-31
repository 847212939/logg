#include "callback.h"
#include <iostream>
#include <memory>
#include "channel.h"
#include "threadpool.h"
#define HANDLEDB_SIZE sizeof(handledb_)/sizeof(handledb_[0])

using namespace std;
using namespace Heartbeat;
Callback::HANDLEDB Callback::handledb_[] = 
{
  &Callback::audit,
  &Callback::machine,
  &Callback::haproxy,
};

void Callback::distribution(Channel *channel)
{
  int protocol = channel->protocol();
  if(protocol >=0 && protocol < HANDLEDB_SIZE)
    (this->*handledb_[protocol])(channel);
}

void Callback::audit(std::shared_ptr<Sbody> &body)
{
  if((*(body->str_)).size() <= 0)
    return;
}

void Callback::audit(Channel *channel)
{

}

void Callback::machine(std::shared_ptr<Sbody> &body)
{
  if((*(body->str_)).size() <= 0)
    return;

}

void Callback::machine(Channel *channel)
{

}

void Callback::haproxy(std::shared_ptr<Sbody> &body)
{
  if((*(body->str_)).size() <= 0)
    return;

  std::shared_ptr<accesses> acc = std::make_shared<accesses>(accesses());
  strHaproxy_.setStr(body->str_);
  if(!strHaproxy_.getAccess(acc))
    return;
  
  body->channel_->channelMtx_.lock();
  body->channel_->accessPlist_.push_back(acc);
  body->channel_->channelMtx_.unlock();
}

void Callback::haproxy(Channel *channel)
{
  vector<shared_ptr<accesses>> accesslist;
  channel->channelMtx_.lock();
  accesslist = std::move(channel->accessPlist_);
  channel->accessPlist_.clear();
  channel->channelMtx_.unlock();
  
  string str ="INSERT INTO accesses_myisam(happentime, sip, dip, dport, hostname, method, url, response_code, response_content_length, refer, user_agent, country, province, city) VALUES";

  vector<shared_ptr<accesses>>::iterator posBegin = accesslist.begin();
  vector<shared_ptr<accesses>>::iterator posEnd = accesslist.end();
  for(posBegin; posBegin != posEnd; ++posBegin)
  {
    if((posBegin + 1) == posEnd)
    {
      str += "( '" + 
          (*posBegin)->happentime + "', '" + (*posBegin)->sip + "', '" + (*posBegin)->dip + "', '" + 
          (*posBegin)->dport + "', '" + (*posBegin)->hostname + "', '" + (*posBegin)->method + "', '" + 
          (*posBegin)->url + "', '" + (*posBegin)->response_code + "', '" + (*posBegin)->response_content_length + "', '" + 
          (*posBegin)->refer + "', '" + (*posBegin)->user_agent + "', '" + (*posBegin)->country + "', '" + 
          (*posBegin)->province + "', '" + (*posBegin)->city + "');";
          break;
    }
    else
    {
      str += "( '" + 
          (*posBegin)->happentime + "', '" + (*posBegin)->sip + "', '" + (*posBegin)->dip + "', '" + 
          (*posBegin)->dport + "', '" + (*posBegin)->hostname + "', '" + (*posBegin)->method + "', '" + 
          (*posBegin)->url + "', '" + (*posBegin)->response_code + "', '" + (*posBegin)->response_content_length + "', '" + 
          (*posBegin)->refer + "', '" + (*posBegin)->user_agent + "', '" + (*posBegin)->country + "', '" + 
          (*posBegin)->province + "', '" + (*posBegin)->city + "'),\n";
    }
  }
  
  //cout <<  str << endl;
  //cout <<  channel->fd() << endl;
  /*if(!dbMysql_.insert(str))
  {
    logg("Database insert failed in Callback::haproxy\n");
    return;
  }*/

}
