#include "config.h"
#include <iostream>
#include "cinotify.h"

using namespace Heartbeat;
int main(int argc, char **argv)
{
  /*std::weak_ptr<Config*> config = Config::config();
  auto p = config.lock();
  if(!(*p)->init())
    return 0;
  std::cout << (*p)->get(Config::WEBAPP, "linkage", 2) << std::endl;
  std::cout << (*p)->get(Config::ANTICC, "title", 2) << std::endl;*/
  Cinotify inotify;
  if(!inotify.init())
    return -1;
  inotify.loop();

  return 0;
}
