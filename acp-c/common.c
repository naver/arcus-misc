/*
 * acp-c : Arcus C Client Performance benchmark program
 * Copyright 2013-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

uint64_t
getmsec(void)
{
  struct timeval tv;
  uint64_t msec;
  
  gettimeofday(&tv, NULL);
  msec = ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec/1000);
  return msec;
}

uint64_t
getusec(void)
{
  struct timeval tv;
  uint64_t usec;
  
  gettimeofday(&tv, NULL);
  usec = ((uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec);
  return usec;
}

void
gettime(uint64_t *msec, struct timeval *ptv, struct timespec *pts)
{
  struct timeval tv;
  if (0 != gettimeofday(&tv, NULL)) {
    PERROR_DIE("gettimeofday");
  }
  if (msec) {
    *msec = ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec/1000);
  }
  if (ptv) {
    *ptv = tv;
  }
  if (pts) {
    pts->tv_sec = tv.tv_sec;
    pts->tv_nsec = (long)tv.tv_usec * 1000;
  }
}

int
parse_hostport(char *addr_str, struct sockaddr_in *addr, char **host_out)
{
  char *host, *c, *dup;
  int port, err = -1;
  
  dup = strdup(addr_str);
  if (dup == NULL) {
    print_log("failed to allocate address buffer. len=%ld",
      strlen(addr_str));
    goto exit;
  }
  c = dup;
  host = c;
  while (*c != ':' && *c != '\0')
    c++;
  if (*c == '\0') {
    print_log("bad address string %s", addr_str);
    goto exit;
  }
  *c++ = '\0';
  errno = 0;
  port = strtol(c, NULL, 10);
  if (errno != 0) {
    ERRLOG(errno, "error while parsing port number (strtol). address=%s",
      addr_str);
    goto exit;
  }
  if (port <= 0 || port >= 64*1024) {
    print_log("bad port %d in address string %s", port, addr_str);
    goto exit;
  }
  memset(addr, 0, sizeof(*addr));
  if (0 != fill_sockaddr(host, addr)) {
    print_log("cannot find the address for host %s", host);
    goto exit;
  }
  addr->sin_port = htons(port);
  err = 0;
exit:
  if (err == 0 && host_out != NULL)
    *host_out = host;
  else if (dup)
    free(dup);
  return err;
}

int
create_proc_name(char **name, char *host, int port, char *trailer)
{
  uint64_t usec;
  pid_t pid;
  struct timeval tv;
  int s, len, trailer_len = 0;
  char *client_name;
  char buf[256];

  if (host == NULL) {
    if (0 != gethostname(buf, sizeof(buf))) {
      ERRLOG(errno, "gethostname");
      return -1;
    }
    host = buf;
  }
  if (0 != gettimeofday(&tv, NULL)) {
    ERRLOG(errno, "gettimeofday");
    return -1;
  }
  usec = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
  pid = getpid();

  len = 512;
  if (trailer) {
    trailer_len = strlen(trailer);
    len += trailer_len;
  }
  client_name = malloc(len);
  if (client_name == NULL) {
    ERRLOG(errno, "malloc");
    return -1;
  }
  memset(client_name, 0, len);
  s = snprintf(client_name, len, "%s:%d-%d-%llx", host, port, pid,
    (long long unsigned)usec);
  if (s <= 0 || s >= len) {
    free(client_name);
    return -1;
  }  
  if (trailer) {
    len -= s;
    if (trailer_len+2 >= len) {
      free(client_name);
      return -1;
    }
    client_name[s++] = '-';
    memcpy(&client_name[s], trailer, trailer_len+1);
  }
  
  *name = client_name;
  return 0;
}

int
sockaddr_from_proc_name(char *name, struct sockaddr_in *addr)
{
  /* name always starts like host:port-... */
  char *c = name, *colon = NULL, *dash = NULL;
  int port, err = -1;
  
  while (*c != '\0' && *c != ':')
    c++;
  if (*c == ':') {
    colon = c;
    while (*c != '\0' && *c != '-')
      c++;
    if (*c == '-') {
      dash = c;
      /* null terminate temporarily... */
      *colon = '\0';
      *dash = '\0';
      c = colon+1;
      port = 0;
      while (*c != '\0') {
        if (*c >= '0' && *c <= '9') {
          port = port * 10 + (*c - '0');
        }
        else {
          port = -1;
          break;
        }
        c++;
      }
      if (port > 0 && port < 64*1024 && 0 == fill_sockaddr(name, addr)) {
        addr->sin_port = htons(port);
        err = 0;
      }
    }
  }
  if (colon)
    *colon = ':';
  if (dash)
    *dash = '-';
  return err;
}

int
parse_hex(char *s, uint64_t *val)
{
  uint32_t num;
  
  *val = 0;
  while (*s != '\0') {
    if (*s >= '0' && *s <= '9') {
      num = *s - '0';
    }
    else if (*s >= 'a' && *s <= 'f') {
      num = *s - 'a' + 0xa;
    }
    else {
      return -1;
    }
    *val *= 16;
    *val += num;
    s++;
  }
  return 0;
}

void
print_log(const char *fmt, ...)
{
  time_t t;
  struct tm tm;
  va_list ap;

  /* XXX fix multiple printf's later */
  
  t = time(NULL);
  localtime_r(&t, &tm);
  printf("[%04d/%02d/%02d-%02d:%02d:%02d] ",
    tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

void
print_errlog(const char *file, int line, int err, const char *fmt, ...)
{
  time_t t;
  struct tm tm;
  va_list ap;
  
  /* XXX fix multiple printf's later */
  
  t = time(NULL);
  localtime_r(&t, &tm);
  printf("[%04d/%02d/%02d-%02d:%02d:%02d] ",
    tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf(" %s(%d) %s:%d\n", strerror(err), err, file, line);
}

void
print_hexdump(const char *data, int len)
{
  const uint8_t *d;
  int off, trunc, i;
  
  trunc = 0;
  if (len > 1024) {
    len = 1024;
    trunc = 1;
  }
  /* offset  00 11 22 33 44 55 66 77  88 99 aa bb cc dd ee ff  ascii */
  d = (const uint8_t*)data;
  off = 0;
  while (len-off >= 16) {
    printf("%04x  %02x %02x %02x %02x %02x %02x %02x %02x"
      "  %02x %02x %02x %02x %02x %02x %02x %02x |",
      off, d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
      d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
    for (i = 0; i < 16; i++) {
      if (d[i] >= 33 && d[i] <= 126)
        printf("%c", d[i]);
      else
        printf(".");
    }
    d += 16;
    off += 16;
    printf("|\n");
  }
  if (len-off > 0) {
    printf("%04x ", off);
    for (i = 0; i < 16; i++) {
      if (i < len-off)
        printf(" %02x", d[i]);
      else
        printf("   ");
      if (i == 7)
        printf(" ");
    }
    printf(" |");
    for (i = 0; i < 16; i++) {
      if (i < len-off) {
        if (d[i] >= 33 && d[i] <= 126)
          printf("%c", d[i]);
        else
          printf(".");
      }
      else
        printf(" ");
    }
    printf("|\n");
  }
  if (trunc)
    printf("...\n");
}

void
print_log_hexdump(const char *data, int len, const char *fmt, ...)
{
  time_t t;
  struct tm tm;
  va_list ap;

  /* XXX Worry about speed later */
  
  t = time(NULL);
  localtime_r(&t, &tm);
  printf("[%04d/%02d/%02d-%02d:%02d:%02d] ",
    tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\ndata=%p len=%d\n", data, len);
  print_hexdump(data, len);
}

char *
msec_to_timestring(uint64_t msec)
{
  static char buf[512];
  time_t t;
  struct tm tm;
  
  t = msec/1000;
  localtime_r(&t, &tm);

  snprintf(buf, sizeof(buf), "[%04d/%02d/%02d-%02d:%02d:%02d.%03d]",
    tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec,
    (int)(msec%1000));
  
  return buf;
}

int
fill_sockaddr(char *host, struct sockaddr_in *addr)
{
  /* */
  struct addrinfo hints;
  struct addrinfo *res, *info;
  int s;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; /* IPv4 */
  res = NULL;
  s = getaddrinfo(host, NULL, NULL, &res);
  if (s == 0) {
    info = res;
    while (info) {
      if (info->ai_family == AF_INET && info->ai_addr &&
        info->ai_addrlen >= sizeof(*addr)) {
        /* Use the first address we find */
        struct sockaddr_in *in = (struct sockaddr_in*)info->ai_addr;
        const char *ip;
        char buf[INET_ADDRSTRLEN*2];
        
        addr->sin_family = in->sin_family;
        addr->sin_addr = in->sin_addr;

        ip = inet_ntop(AF_INET, (const void*)&addr->sin_addr, buf, sizeof(buf));
        print_log("Address for host. host=%s addr=%s canonicalname=%s",
          host, ip ? ip : "null",
          info->ai_canonname ? info->ai_canonname : "null");
        break;
      }
      info = info->ai_next;
    }
    if (info == NULL) {
      print_log("No addresses for the host. host=%s", host);
      s = -1;
    }
  }
  else {
    if (s == EAI_SYSTEM) {
      ERRLOG(errno, "Failed to get the address of the host. host=%s",
        host);
    }
    else {
      print_log("Failed to get the address of the host. host=%s error=%s",
        host, gai_strerror(s));
    }
    s = -1;
  }

  if (res)
    freeaddrinfo(res);
  return s;
}
