#pragma once

#include <yaml.h>
#include <string>
#include <vector>
#include <memory>
#include <map>

#define setbit(x,y) x|=(1<<y)         
#define clrbit(x,y) x&=~(1<<y)

namespace Heartbeat
{
class Config
{
public:
  enum ENTRY_ENUM
  {
    WEBAPP = 0,
    ANTICC,
    AUDITLOG,
    LAST
  };//end ENTRY_ENUM

public:
  bool init();
  static std::weak_ptr<Config*> config();
  const std::string get(ENTRY_ENUM type, const std::string strKey, int terms = 1);
  const std::string getColumn(const std::string strValue, int column);

private:
  class Content
  {
  public:
	  explicit Content(std::string &strKey):key_(std::make_shared<std::string>(strKey)), count_(0) { value_ = nullptr; }
	  void setValue(std::string &strValue) { value_ = std::make_shared<std::string>(strValue); }
    void setCount(int count) { count_ = count; }
    const std::string &key() { return *key_; }
    const int count() { return count_; }
    const std::string value() 
    { 
      if(value_ == nullptr)
        return std::string();
      return *value_; 
    }

  private:
	  std::shared_ptr<std::string> key_;
	  std::shared_ptr<std::string> value_;
	  int count_;
  };//end Content

private:
  enum YAML_ENUM
  {
    ENUM_SS = 0,
    ENUM_SE,
    ENUM_KEY,
    ENUM_VALUE,
    ENUM_BSS,
    ENUM_BEY,
    ENUM_BED,
    ENUM_BMS,
  };//end YAML_ENUM

private:
  typedef std::vector<std::shared_ptr<Content>> container;
  typedef std::shared_ptr<container> containerptr;
  typedef std::shared_ptr<std::string> path;
  typedef std::pair<path, containerptr> entry;

private:
  Config():group_(0), count_(0) {}
  virtual ~Config() 
  {
    for(int i = 0; i < LAST; ++i)
    {
      ENTRY_ENUM type = WEBAPP;
      type = static_cast<ENTRY_ENUM>(i);
      entryMap_[type].second->clear();
    }
    entryMap_.clear();
  }
  bool load(ENTRY_ENUM type);
  entry factory(const std::string &strFile, container &containerList);
  void handleEvent(std::string &strkey, std::string &strValue, int &nBit);
  void handleArr(std::string &strValue, int &nBit);
  void handleKey(std::string &strkey, int &nBit);
  void pushVector(std::string &strkey);

private:
  std::map<ENTRY_ENUM, entry> entryMap_;
  static std::shared_ptr<Config*> config_;
  static std::string strfile_[];
  yaml_parser_t parser_;
  yaml_token_t token_;
  ENTRY_ENUM type_;
  YAML_ENUM Ebit_;
  std::string str_;
  int group_;
  int count_;
};//end Config
}//end Heartbeat
