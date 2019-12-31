#include "area.h"

using namespace Heartbeat;
ipgeo_loc *Carea::ipgeo_search(const char *ip)
{
  int idx;
  if((idx = search_index(ip)) < 0) 
  {
	  //warnx("ipgeo_search/search_index: %s", ip);
	  return nullptr;
  }
  return read_geo(idx);
}

bool Carea::ipgeo_new(const char *fname)
{
  int fd;
  struct stat st;
  if((fd = open(fname, O_RDONLY)) == -1) 
  {
    warn("ipgeo_new/open: ");
	  return false;
  }
  
  if(fstat(fd, &st) == -1) 
  {
	  warn("ipgeo_new/fstat: ");
	  return false;
  }
  
  if((ipgeo_.data = (unsigned char*)mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == NULL) 
  {
	  warn("ipgeo_new/mmap: ");
	  return false;
  }
  close(fd);

  ipgeo_.data_len = st.st_size;
  ipgeo_.begin = unpack_uint32(ipgeo_.data);
  ipgeo_.end = unpack_uint32(ipgeo_.data + 4);
  ipgeo_.index = (index_entry *)(ipgeo_.data + ipgeo_.begin);
  ipgeo_.index_count = (ipgeo_.end - ipgeo_.begin) / sizeof(index_entry) + 1;
  return true;
}

ipgeo_loc *Carea::read_geo(int idx)
{
  char *s;
  char *code;
  int offset;
  ip_entry *ent;
  offset = unpack_offset(ipgeo_.index[idx].offset);
  ent = (ip_entry *)(ipgeo_.data + offset);

  if(ent->code[0] == 1)
	code = (char *)(ipgeo_.data + unpack_offset(ent->code + 1));
  else
	code = ent->code;

  snprintf(loc_.country, COUNTRY_LEN+1, code);

  code += strlen(code) + 1;
  if ((s = strchr(code, '|')) == NULL) 
	return NULL;
  snprintf(loc_.province, PROVINCE_LEN+1, "%.*s", (int)(s - code), code);
  snprintf(loc_.city, CITY_LEN+1, s + 1);

  return &loc_;
}

uint32_t Carea::unpack_addr(void *buf)
{
  return unpack_uint32(buf);
}

int Carea::search_index(const char *ip)
{
  in_addr in;
  uint32_t addr;
  int imin, imax;
  if((inet_pton(AF_INET, ip, &in)) == 0) 
  {
	  //warnx("ip_to_integer: %s", ip);
	  return -1;
  }
  addr = ntohl(in.s_addr);
  imin = 0;
  imax = ipgeo_.index_count - 1;
  /* Return directly if last entry is matched. */
  if(addr >= unpack_addr(ipgeo_.index[imax].addr))
	return imax;

  while(imax > imin + 1) 
  {
	int imid = (imin + imax) / 2;
	uint32_t entry = unpack_addr(ipgeo_.index[imid].addr);
	if(addr == entry) 
  {
	  imin = imid;
	  break;
	}
		
	if(addr > entry)
	  imin = imid;
	else
	  imax = imid;
  }
  return imin;
}

uint32_t Carea::unpack_uint32(void *buf)
{
  unsigned char *b = (unsigned char *)buf;
  return (b[0] << 0) | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
}

uint32_t Carea::unpack_offset(void *buf)
{
  unsigned char *b = (unsigned char *)buf;
  return (b[0] << 0) | (b[1] << 8) | (b[2] << 16);
}
