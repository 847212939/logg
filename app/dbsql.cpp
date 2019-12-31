#include "mylog.h"
#include "dbsql.h"

using namespace std;
using namespace Heartbeat;
bool database::initConnection(const char *host,
					   const char *user,
					   const char *passwd,
					   const char *db,
					   unsigned int port,
             const char *dbsock)
{
  try 
  {
    if ((mysql_ = ::mysql_init(nullptr)) == nullptr)
    {
      logg("mysql_init is failed in dataBase::initConnection\n");
      return false;
    }
    // localhost:服务器 root为账号密码 test为数据库名 3306为端口    
    if (!::mysql_real_connect(mysql_, host, user, passwd, db, port, dbsock, 0))
    {
      logg("mysql_real_connect is failed in dataBase::initConnection\n");
      return false;
    }

    return true;
  }
  catch (...)
  {
    logg("mysql exception in dataBase::initConnection\n");
    return false;
  }
}

bool database::insert(std::string &Sql)
{
  if(mysql_ == nullptr)
    return false;

  if(0 != ::mysql_query(mysql_, Sql.c_str()))
    return false;

  return true;
}