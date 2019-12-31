#ifndef OUTPUT_LOG_H
#define OUTPUT_LOG_H
#include <iostream>

void logg(const char* fmt, ...);
void log_err_code(const char* fmt, ...);
const char *field_start(const char *p, int field);

int code_convert(const char *from_charset,
                 const char *to_charset,
                 char *inbuf,size_t inlen,
                 char *outbuf,size_t outlen);
int u2g(char *inbuf,int inlen,char *outbuf,size_t outlen);
int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen);
#endif
