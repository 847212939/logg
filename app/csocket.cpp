#include "csocket.h"
#include "mylog.h"
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <endian.h>
#include <sys/ioctl.h> 

using namespace std;
using namespace Heartbeat;
CSocket::CSocket(string strpath):path_(strpath)
{
  memset(&addr_, 0, sizeof(struct sockaddr_un));
  addr_.sun_family = AF_UNIX;
  strcpy(addr_.sun_path, strpath.c_str());
}

int CSocket::socket()
{
  sockfd_ = ::socket(PF_UNIX, SOCK_STREAM, 0);
  if (sockfd_ < 0)
  {
    logg("socket errno is %d in CSocket::socket\n", errno);
    return -1;
  }
  
  setReuseAddr(true);
  if(!setNonBlock(sockfd_))
    return -1;
  unlink(path_.c_str());

  if(!bind())
    return -1;
  if(!listen())
    return -1;
  return sockfd_;
}

bool CSocket::bind()
{
  int ret = ::bind(sockfd_, (struct sockaddr*)&addr_, sizeof(addr_));
  if (ret < 0)
  {
    logg("bind errno is %d in CSocket::bind\n", errno);
    return false;
  }
  return true;
}

bool CSocket::listen()
{
  int ret = ::listen(sockfd_, SOMAXCONN);
  if (ret < 0)
  {
    logg("listen errno is %d in CSocket::listen\n", errno);
    return false;
  }
  return true;
}

int CSocket::accept()
{
#if VALGRIND || defined (NO_ACCEPT4)
  fd_ = ::accept(sockfd_, NULL, NULL);
  setNonBlock(fd_);
#else
  fd_ = ::accept4(sockfd_, NULL, NULL, SOCK_NONBLOCK);
#endif
  if (fd_ < 0)
  {
    int savedErrno = errno;
    //cout << "Socket::accept";
    switch (savedErrno)
    {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO: // ???
      case EPERM:
      case EMFILE: // per-process lmit of open file desctiptor ???
        // expected errors
        errno = savedErrno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        logg("unexpected error of ::accept %d inCSocket::accept\n", savedErrno);
        break;
      default:
        logg("unknown error of ::accept %d in CSocket::accept\n", savedErrno);
        break;
    }
  }
  return fd_;
}

bool CSocket::setNonBlock(int fd) 
{    
  int nb=1; //0：清除，1：设置
  //FIONBIO：设置/清除非阻塞I/O标记：0：清除，1：设置  
  if(ioctl(fd, FIONBIO, &nb) == -1)
  {
    logg("ioctl errno is %d in CSocket::setNonBlock\n", errno);
    return false;
  }
  return true;
}

void CSocket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, 
              IPPROTO_TCP, 
              TCP_NODELAY, 
              &optval, 
              static_cast<socklen_t>(sizeof optval)
              );
}

//主要是解决TIME_WAIT这个状态导致bind()失败的问题
void CSocket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, 
              SOL_SOCKET, 
              SO_REUSEADDR, 
              &optval, 
              static_cast<socklen_t>(sizeof optval)
              );
}

void CSocket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, 
                         SOL_SOCKET, 
                         SO_REUSEPORT,
                         &optval, 
                         static_cast<socklen_t>(sizeof optval)
                         );
  if (ret < 0 && on)
  {
    logg("SO_REUSEPORT is not supported in CSocket::setReusePort\n");
    //LOG_SYSERR << "SO_REUSEPORT failed.";
  }
#else
  if (on)
  {
    logg("SO_REUSEPORT is not supported in CSocket::setReusePort\n");
    //LOG_ERROR << "SO_REUSEPORT is not supported.";
  }
#endif
}

void CSocket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, 
                SO_KEEPALIVE, &optval, 
                static_cast<socklen_t>(sizeof optval));
}

bool CSocket::shutdownWrite()
{
  if (::shutdown(sockfd_, SHUT_WR) < 0)
  {
    logg("shutdown is error in CSocket::shutdownWrite\n");
    //LOG_SYSERR << "sockets::shutdownWrite";
    return false;
  }
  return true;
}

bool CSocket::getTcpInfo(struct tcp_info* tcpi) const
{
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0, len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool CSocket::getTcpInfoString(char* buf, int len) const
{
  struct tcp_info tcpi;
  bool ok = getTcpInfo(&tcpi);
  if (ok)
  {
    snprintf(buf, len, "unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u",
             tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
             tcpi.tcpi_rto,          // Retransmit timeout in usec
             tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
             tcpi.tcpi_snd_mss,
             tcpi.tcpi_rcv_mss,
             tcpi.tcpi_lost,         // Lost packets
             tcpi.tcpi_retrans,      // Retransmitted packets out
             tcpi.tcpi_rtt,          // Smoothed round trip time in usec
             tcpi.tcpi_rttvar,       // Medium deviation
             tcpi.tcpi_snd_ssthresh,
             tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
  }
  return ok;
}
