#pragma once
#include "area.h"
#include <string>
#include <memory>

namespace Heartbeat
{
struct accesses
{
  std::string happentime;
  std::string sip;
  std::string dip;
  std::string dport;
  std::string hostname;
  std::string method;
  std::string url;
  std::string response_code;
  std::string response_content_length;
  std::string refer;
  std::string user_agent;
  std::string country;
  std::string province;
  std::string city;
  accesses():happentime(""), sip(""), dip(""), dport(""), 
             hostname(""), method(""), url(""), response_code(""), 
             response_content_length(""), refer(""), user_agent(""), 
             country(""), province(""), city("") {}
};

class SourceStr
{
public:
  SourceStr() {} 
  virtual ~SourceStr() {}
  void setStr(std::shared_ptr<std::string> &str) { str_ = str; }
public:
  bool getAccess(std::shared_ptr<accesses> &acc);

private:
  size_t findFromStart(std::string &str, const char &character, int start = 0);
  size_t findCharacter(std::string &str, const char &character, int whichOne = 1);
  size_t findRCharacter(std::string &str, const char &character, int whichOne = 1);

private:
  void happentime(std::string &str);
  int sip(std::string &str, int start = 0);
  int refer(std::string &str, int start = 0);
  int hostname(std::string &str, int start = 0);
  int user_agent(std::string &str, int start = 0);
  int response_code(std::string &str, int start = 0);
  int response_content_length(std::string &str, int start = 0);
  int dipAndPort(std::string &strDip, std::string &strPort, int start = 0);
  int method_hostname_url(std::string &strMethod, std::string &strHostname, std::string &strUrl, int start = 0);

private:
  Carea area_;
  std::weak_ptr<std::string> str_;
  
};//end SourceStr
}//end Heartbeat
