#pragma once

#include <err.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define COUNTRY_LEN 3
#define PROVINCE_LEN 3
#define CITY_LEN 50

namespace Heartbeat
{
struct ipgeo_loc 
{
  char country[COUNTRY_LEN+1];
  char province[PROVINCE_LEN+1];
  char city[CITY_LEN+1];
  ipgeo_loc()
  { memset(country, 0, sizeof(country)); 
    memset(province, 0, sizeof(province)); 
    memset(city, 0, sizeof(city)); }
};

struct ip_entry 
{
  unsigned char addr[4];
  char code[];
} __attribute__((packed));

struct index_entry 
{
  unsigned char addr[4];
  unsigned char offset[3];
  index_entry()
  { memset(addr, 0, sizeof(addr)); 
    memset(offset, 0, sizeof(offset)); }
};

struct ipgeo 
{
  unsigned char *data;
  size_t data_len;
  index_entry *index;
  int index_count;
  uint32_t begin;
  uint32_t end;
  ipgeo()
  :data(nullptr), 
   data_len(0), 
   index_count(0), 
   begin(0), end(0){}
};

class Carea
{
public:
  Carea() { ipgeo_new("./ipgeo.dat"); } // "/waf/system_service/ipgeo/ipgeo.dat"
  ~Carea() { munmap(ipgeo_.data, ipgeo_.data_len); }
public:
  ipgeo_loc *ipgeo_search(const char *ip);

private:
  ipgeo_loc *read_geo(int idx);
  uint32_t unpack_addr(void *buf);
  int search_index(const char *ip);
  uint32_t unpack_offset(void *buf);
  uint32_t unpack_uint32(void *buf);
  bool ipgeo_new(const char *fname);
private:
  ipgeo ipgeo_;
  ipgeo_loc loc_;
  
};//end Carea
}//end Heartbeat