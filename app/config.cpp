#include "config.h"
#include "mylog.h"
#include <iostream>
#include <algorithm>

using namespace std;
using namespace Heartbeat;
string Config::strfile_[]
{
  string("/waf/config/misc/webapp.yaml"),
  string("/waf/config/misc/anticc.yaml"),
  string("/waf/config/misc/auditlog_input.conf"),
};

Config::entry Config::factory
( 
  const string &strFile, 
  Config::container &containerList
)
{
  return std::move(pair<path, containerptr>
  (
    make_shared<string>(strFile), 
    make_shared<container>(containerList)
  ));
}

bool Config::init()
{
  ENTRY_ENUM type = WEBAPP;
  for(int i = 0; i < LAST; ++i)
  {
    container c;
    type = static_cast<ENTRY_ENUM>(i);
    entryMap_[type] = factory(strfile_[i], c);
    if(!load(type))
      return false;
    logg("%s load success in Config::init()\n",strfile_[i].c_str());
  }
  return true;
}

shared_ptr<Config*> Config::config_ = nullptr;
weak_ptr<Config*> Config::config()
{
  if(0 == config_.use_count())
  {
    config_ = make_shared<Config*>(new Config);
  }
  return config_;
}

const string Config::get(ENTRY_ENUM type, const string strKey, int terms)
{
  int n = 0;
  string str = to_string(terms);
  vector<std::shared_ptr<Content>>::iterator pos;
  if(terms > 1)
    pos = find_if(entryMap_[type].second->begin(), entryMap_[type].second->end(), 
    [&str, &strKey, &n](shared_ptr<Content> &ptrC)
    {
      if(ptrC->key() == str)
      {
        ++n;
        if(n > 1)
          return true;
        str = strKey;
      }
      return false;
    });
  else
    pos = find_if(entryMap_[type].second->begin(), entryMap_[type].second->end(), 
    [&strKey](shared_ptr<Content> &ptrC)
    {
      if(ptrC->key() == strKey)
        return true;
      return false;
    });
                  
  if(pos == entryMap_[type].second->end())
    return string();
  return (*pos)->value();
}

const std::string getColumn(const std::string strValue, int column)
{
  if(strValue.size() == 0 || column < 1)
    return string();
  int n = 1;
  string::iterator pos;
  string str = static_cast<string>(strValue);
  pos = find_if(str.begin(), str.end(),
  [&column, &n](char &c)
  {
    if(c == ' ')
      ++n;
    if(n == column)
      return true;
    return false;
  });

  if(n == column && ++pos != str.end())
    return str.substr(pos - str.begin());
  return string();
}

void Config::pushVector(string &strkey)
{
  shared_ptr<Content> ptr(make_shared<Content>(Content(strkey)));
  if(str_.size() > 0) 
    ptr->setValue(str_);

  ptr->setCount(count_);
  entryMap_[type_].second->push_back(ptr);
  str_.clear();
  count_ = 0;
}

void Config::handleEvent(string &strkey, string &strValue, int &nBit)
{
  if(0 == strValue.size())
  {
    clrbit(nBit,ENUM_VALUE);
    return;
  }
  str_ = strValue;
  ++count_;
  pushVector(strkey);
  clrbit(nBit,ENUM_VALUE);
}

void Config::handleArr(string &strValue, int &nBit)
{   
  if(str_.size() > 0)
  {
    str_ += " ";
    str_ += strValue;
  }
  else
  {
    str_ = strValue;
  }
  ++count_;
  clrbit(nBit,ENUM_BEY);
}

void Config::handleKey(string &strkey, int &nBit)
{
  if(str_.size() > 0) 
  {
    pushVector(strkey);
    return;
  }
  clrbit(nBit,ENUM_KEY);
}

bool Config::load(ENTRY_ENUM type)
{
  string str;
  string strKey;
  int nBit = 0;
  FILE *fh = nullptr;
  type_ = type;   
  if( (fh = fopen(entryMap_[type].first->c_str(), "r")) == nullptr)
  {
    logg("fopen is errno in Config::load\n");
    return false;
  }
  if(!yaml_parser_initialize(&parser_))
  {
    logg("yaml_parser_initialize is errno in Config::load\n");
	  return false;
  }

  yaml_parser_set_input_file(&parser_, fh);
  do 
  {
	yaml_parser_scan(&parser_, &token_);
	switch (token_.type)
	{
	case YAML_STREAM_START_TOKEN:
    setbit(nBit, ENUM_SS);
    Ebit_ = ENUM_SS;
	  break;
	case YAML_STREAM_END_TOKEN:
    handleKey(strKey, nBit);
	  break;
	case YAML_BLOCK_SEQUENCE_START_TOKEN: 
    setbit(nBit, ENUM_BSS);
    Ebit_ = ENUM_BSS;
	  break;
	case YAML_BLOCK_END_TOKEN:
    setbit(nBit, ENUM_BED);
    Ebit_ = ENUM_BED;
	  break;
	case YAML_BLOCK_MAPPING_START_TOKEN:
    setbit(nBit, ENUM_BMS);
    Ebit_ = ENUM_BMS;
	  break;
	case YAML_KEY_TOKEN:
    setbit(nBit, ENUM_KEY);
    Ebit_ = ENUM_KEY;
    ++group_;
    if(2 == group_)
    {
      ++count_;
      pushVector(strKey);
      --group_;
    }
    break;
	case YAML_VALUE_TOKEN:
    setbit(nBit, ENUM_VALUE); 
    Ebit_ = ENUM_VALUE;
	  break;
  case YAML_BLOCK_ENTRY_TOKEN:
    setbit(nBit, ENUM_BEY);
    Ebit_ = ENUM_BEY;
	  break;
  case YAML_SCALAR_TOKEN:
  {
    str = (char*)token_.data.scalar.value;
    switch (Ebit_)
    {
    case ENUM_KEY:
      handleKey(strKey, nBit);
      strKey = str;
      break;
    case ENUM_VALUE:
      handleEvent(strKey, str, nBit);
      group_ = 0;
      break;
    case ENUM_BEY:
      handleArr(str, nBit);
      group_ = 0;
      break;
    default:
      group_ = 0;
      break;
    }
    break;
  }
	default:
      break;
	}
	if(token_.type != YAML_STREAM_END_TOKEN)
	  yaml_token_delete(&token_);
  } while (token_.type != YAML_STREAM_END_TOKEN);

  yaml_token_delete(&token_);
  yaml_parser_delete(&parser_);
  fclose(fh);
  return true;
}