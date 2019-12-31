#pragma once
#include <list>
#include <mutex>
#include <memory>
#include "parsestr.h"
#include "cinotify.h"
#include "mylog.h"
#include "dbsql.h"
 
namespace Heartbeat
{
typedef class Channel Channel;

class Callback
{
typedef void(Callback::*HANDLEDB)(Channel*);
public:
  Callback() 
  { 
    /*if(!dbMysql_.initConnection("127.0.0.1", "mywaf", "mywaf", "mywaf", 0, "/data/mysql/mysql.sock"))
    {
      logg("Mysql initialization error in Callback::Callback\n");
      exit(-1);
    } */
  }
  ~Callback() {}

public:
  void distribution(Channel *channel);

  void audit(std::shared_ptr<Sbody> &body);
  void machine(std::shared_ptr<Sbody> &body);
  void haproxy(std::shared_ptr<Sbody> &body);

private:
  void audit(Channel *channel);
  void machine(Channel *channel);
  void haproxy(Channel *channel);

private:
  database dbMysql_;
  SourceStr strAudit_;
  SourceStr strMachine_;
  SourceStr strHaproxy_;
  static HANDLEDB handledb_[];

};//end Callback
}//endl Heartbeat
