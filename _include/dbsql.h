#pragma once 
#include "dbsql.h"
#include <string>
#include <mysql/mysql.h>

namespace Heartbeat
{
class database
{
public:
  database():mysql_(nullptr) {}
  ~database() {}
public:
  bool initConnection(const char *host,
					   const char *user,
					   const char *passwd,
					   const char *db,
					   unsigned int port,
             const char *dbsock);
  bool insert(std::string &Sql);
private:
  MYSQL *mysql_;
};//end database
}//end Heartbeat