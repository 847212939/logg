#include "parsestr.h"
#include <algorithm>
#include <string.h>
#include <time.h>

using namespace std;
using namespace Heartbeat;
bool SourceStr::getAccess(std::shared_ptr<accesses> &acc)
{
  int ret = 0;

  happentime(acc->happentime);
  if((ret = sip(acc->sip, ret)) == 0)
    return false;
  if((ret = dipAndPort(acc->dip, acc->dport, ret)) == 0)
    return false;
  if((ret = response_code(acc->response_code, ret)) == 0)
    return false;
  if((ret = response_content_length(acc->response_content_length, ret)) == 0)
    return false;
  if((ret = method_hostname_url(acc->method, acc->hostname,  acc->url, ret)) == 0)
    return false;
  if((ret = refer(acc->refer, ret)) == 0)
    return false;
  if((ret = user_agent(acc->user_agent, ret)) == 0)
    return false;

  auto loc = area_.ipgeo_search((acc->sip).c_str());
  if(loc != nullptr)
  {
    acc->country = loc->country;
    acc->province = loc->province;
    acc->city = loc->city;
  }

  return true;
}

size_t SourceStr::findCharacter(string &str, const char &character, int whichOne)
{
  int n = 0;
  std::string::iterator pos;
  pos = find_if(str.begin(), str.end(),
  [&whichOne, &character, &n](char &c)
  {
    if(c == character)
      ++n;
    if(n == whichOne)
      return true;
    return false;
  });
  
  //返回数组的下标从0开始；
  if(n == whichOne && pos != str.end())
    return (pos - str.begin());
  return 0;
}

size_t SourceStr::findRCharacter(string &str, const char &character, int whichOne)
{
  int n = 0;
  std::string::reverse_iterator pos;
  pos = find_if(str.rbegin(), str.rend(),
  [&whichOne, &character, &n](char &c)
  {
    if(c == character)
      ++n;
    if(n == whichOne)
      return true;
    return false;
  });
  
  //返回数组的下标从0开始；
  if(n == whichOne && pos != str.rend())
    return (str.rend() - pos);
  return 0;
}

size_t SourceStr::findFromStart(string &str, const char &character, int start)
{
  if((str.begin() + start) >= str.end() || start < 0)
    return 0;
  
  std::string::iterator pos;
  pos = find_if((str.begin() + (start + 1)), str.end(),
  [&character](char &c)
  {
    if(c == character)
      return true;
    return false;
  });
  
  //返回数组的下标从0开始；都是相对于整个字符串的长度
  if(pos != str.end())
    return (pos - str.begin());
  return 0;
}

void SourceStr::happentime(string &str)
{
  time_t  t;
	struct tm* ctm;
  char buf[64] = "";
	t = time(NULL);
	ctm = localtime(&t);
  snprintf(buf, sizeof(buf), 
          "%4d-%02d-%02d %02d:%02d:%02d", 
		      ctm->tm_year+1900, 
          ctm->tm_mon+1, 
          ctm->tm_mday,
	 	      ctm->tm_hour, 
          ctm->tm_min, 
          ctm->tm_sec);
  str = string(buf);
}

int SourceStr::sip(string &str, int start)
{
  int ret = 0;

  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  start = findCharacter(*shared_str, ' ', 5);
  if(0 == start)
    return ret;

	size_t end  = findFromStart(*shared_str, '/', start);
  if(0 == end)
   return ret;
  ret = end;

  string sstr = (*shared_str).substr(start + 1, end - start - 1);
  end = findRCharacter(sstr, ':');
  if(0 == end)
    return ret;

  str = sstr.substr(0, end - 1);
  return ret;
}

int SourceStr::dipAndPort(std::string &strDip, std::string &strPort, int start)
{
  int ret = 0;

  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  start = findFromStart(*shared_str, '/', start);
  if(0 == start)
    return ret;
  size_t end = findFromStart(*shared_str, ' ', start);
  if(0 == end)
    return ret;

  string sstr = (*shared_str).substr(start + 1, end - start - 1);
  start = findRCharacter(sstr, ':');
  if(0 == start)
    return ret;

  ret = end;
   
  strDip = sstr.substr(0, start - 1);
  strPort = sstr.substr(start, end - start - 1);
  return ret;
}

int SourceStr::response_code(std::string &str, int start)
{
  int ret = 0;

  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  start = findFromStart(*shared_str, ' ', start);
  if(0 == start)
    return ret;

  size_t end = findFromStart(*shared_str, ' ', start);
  if(0 == end)
    return ret;
  
  str = (*shared_str).substr(start + 1, end - start - 1);
  ret = end;
  return ret;
}

int SourceStr::response_content_length(std::string &str, int start)
{
  int ret = 0;

  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  start = findFromStart(*shared_str, '/', start);
  if(0 == start)
    return ret;

  size_t end = findFromStart(*shared_str, ' ', start);
  if(0 == end)
    return ret;

  str = (*shared_str).substr(start + 1, end - start - 1);
  ret = end;
  return ret;
}

int SourceStr::method_hostname_url(std::string &strMethod, std::string &strHostname, std::string &strUrl, int start)
{
  int ret = 0;

  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  start = findFromStart(*shared_str, '"', start);
  if(0 == start)
    return ret;

  if(!strncmp((*shared_str).c_str() + (start + 1), "<BADREQ>", strlen("<BADREQ>")))
    return ret;

  size_t end = findFromStart(*shared_str, ' ', start);
  if(0 == end)
    return ret;
  
  strMethod = (*shared_str).substr(start + 1, end - start - 1);

  start = end;
  end = findFromStart(*shared_str, ' ', start);
  if(0 == end)
    return ret;

  size_t http = findFromStart(*shared_str, '{', end);
  if(0 == http)
    return ret;
  
  if(!strncmp((*shared_str).c_str() + (http -2), "-", 1))
    strUrl = "http://";
  else
    strUrl = "https://";

  if(0 == hostname(strHostname, http))
    return ret;
  strUrl += strHostname + (*shared_str).substr(start + 1, end - start - 1);
  ret = http;
  return ret;
}

int SourceStr::hostname(std::string &str, int start)
{
  int ret = 0;
  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  size_t end = findFromStart(*shared_str, '|', start);
  if(0 == end)
    return ret;
  
  str = (*shared_str).substr(start + 1, end - start - 1);
  ret = start;
  return ret;
}

int SourceStr::refer(std::string &str, int start)
{
  int ret = 0;

  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  start = findFromStart(*shared_str, '|', start);
  if(0 == start)
    return ret;

  size_t end = findFromStart(*shared_str, '|', start);
  if(0 == end)
    return ret;
  
  str = (*shared_str).substr(start + 1, end - start - 1);
  ret= end;
  return ret;
}

int SourceStr::user_agent(std::string &str, int start)
{
  int ret = 0;
  std::shared_ptr<std::string> shared_str = str_.lock();
  if(shared_str == nullptr)
    return ret;

  size_t end = findFromStart(*shared_str, '}', start);
  if(0 == end)
    return ret;
  
  str = (*shared_str).substr(start + 1, end - start - 1);
  ret = end;
  return ret;
}


  