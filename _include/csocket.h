#pragma once
#include <netinet/in.h>
#include <string>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <unistd.h>

namespace Heartbeat
{
class CSocket
{
public:
  CSocket(std::string strpath);
  virtual ~CSocket() { }

  bool getTcpInfo(struct tcp_info* tcpi) const;
  bool getTcpInfoString(char* buf, int len) const;

  int socket();
  int accept();

private:
  bool shutdownWrite();
  bool listen();
  bool bind();

public:
  bool setNonBlock(int fd); 
  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void setKeepAlive(bool on);

 private:
  struct sockaddr_un addr_;
  std::string path_;
  int sockfd_;
  int fd_;


};//end CSocket
}//end Heartbeat