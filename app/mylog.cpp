#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <iconv.h>
#include <string.h>

#define OLOG_FILE "/data/logs/syslog_cc.log"
#define ERR_CODE_FILE "/data/logs/err_syslog_cc.log"

void logg(const char* fmt, ...)
{
  FILE*   olog;
  va_list ap;
  time_t  t;
  struct tm* ctm;

  if ((olog = fopen(OLOG_FILE, "a+")) == NULL) 
  {
    fprintf(stderr, "can't open log!\n");
    return;
  }

  t = time(NULL);
  ctm = localtime(&t);
  fprintf
  (
    olog, 
    "%4d-%02d-%02d %02d:%02d:%02d ", 
    ctm->tm_year+1900, 
    ctm->tm_mon+1, 
    ctm->tm_mday,
    ctm->tm_hour, 
    ctm->tm_min, 
    ctm->tm_sec
  );

  va_start(ap, fmt);
  vfprintf(olog, fmt, ap);
  va_end(ap);

  fclose(olog);
}

void log_err_code(const char* fmt, ...)
{
  FILE*   olog;
  va_list ap;
  time_t  t;
  struct tm* ctm;

  if ((olog = fopen(ERR_CODE_FILE, "a+")) == NULL) 
  {
    fprintf(stderr, "can't open log!\n");
    return;
  }

  t = time(NULL);
  ctm = localtime(&t);
  fprintf
  (
    olog, 
    "%4d-%02d-%02d %02d:%02d:%02d ", 
    ctm->tm_year+1900, 
    ctm->tm_mon+1, 
    ctm->tm_mday,
    ctm->tm_hour, 
    ctm->tm_min, 
    ctm->tm_sec
  );

  va_start(ap, fmt);
  vfprintf(olog, fmt, ap);
  va_end(ap);

  fclose(olog);
}

#define PREFER_ASM
const char *field_start(const char *p, int field)
{
#ifndef PREFER_ASM
  unsigned char c;
  while (1) 
  {
    /* skip spaces */
    while (1) 
    {
      c = *(p++);
      if (c > ' ')
        break;
      if (c == ' ')
        continue;
      if (!c) /* end of line */
        return p-1;
      /* other char => new field */
      break;
    }
    /* start of field */
    field--;
    if (!field)
      return p-1;

    /* skip this field */
    while (1) 
    {
      c = *(p++);
      if (c == ' ')
        break;
      if (c > ' ')
        continue;
      if (c == '\0')
        return p-1;
    }

  }
#else
  asm(
      "1:                  \n\t"
      "inc   %0            \n\t"
      "cmpb  $0x20, -1(%0) \n\t"
      "ja    2f            \n\t"
      "jz    1b            \n\t"
      "cmpb  $0x0, -1(%0)  \n\t"
      "jz    3f            \n\t"
      "2:                  \n\t"
      "dec   %1            \n\t"
      "jz    3f            \n\t"
      "4:                  \n\t"
      "inc   %0            \n\t"
      "cmpb  $0x20, -1(%0) \n\t"
      "jz    1b            \n\t"
      "ja    4b            \n\t"
      "cmpb  $0x0, -1(%0)  \n\t"
      "jnz   4b            \n\t"
      "3:                  \n\t"
      "dec   %0            \n\t"
      : "=r" (p)
      : "r" (field), "0" (p)
      );
  return p;
#endif
}

int code_convert(const char *from_charset, 
                const char *to_charset,
                char *inbuf, 
                size_t inlen, 
                char *outbuf, 
                size_t outlen)  
{  
  iconv_t cd;  
  int rc;  
  char **pin = &inbuf;  
  char **pout = &outbuf;  
  cd = iconv_open(to_charset, from_charset);  
  if(cd == 0)
  {
     logg("iconv_open is errno\n");
     return -1;  
  }  
  memset(outbuf, 0, outlen);  
  if(iconv(cd, pin, &inlen, pout, &outlen) == -1)
  {
     logg("iconv is errno\n");
     return -1;  
  }
  iconv_close(cd);  
  return 0;  
}  
  
int u2g(char *inbuf, 
        int inlen, 
        char *outbuf, 
        size_t outlen)  
{  
  return code_convert
  (
    "utf-8", 
    "gb2312", 
    inbuf, 
    inlen, 
    outbuf, 
    outlen
  );  
}  
  
int g2u(char *inbuf, 
        size_t inlen, 
        char *outbuf, 
        size_t outlen)  
{  
  return code_convert
  (
    "gb2312", 
    "utf-8", 
    inbuf, 
    inlen, 
    outbuf, 
    outlen
  );  
}