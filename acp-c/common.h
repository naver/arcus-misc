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
#ifndef _COMMON_H_
#define _COMMON_H_

char *msec_to_timestring(uint64_t msec);
int fill_sockaddr(char *host, struct sockaddr_in *addr);
int parse_hex(char *s, uint64_t *val);
int create_proc_name(char **name, char *host, int port, char *trailer);
int sockaddr_from_proc_name(char *name, struct sockaddr_in *addr);
int parse_hostport(char *addr_str, struct sockaddr_in *addr, char **host);
void gettime(uint64_t *msec, struct timeval *ptv, struct timespec *pts);
uint64_t getmsec(void);
uint64_t getusec(void);

void print_log(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

void print_errlog(const char *file, int line, int err, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 4, 5)));

void print_log_hexdump(const char *data, int len, const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 3, 4)));

void print_hexdump(const char *data, int len);

#define ERRLOG(err, ...) do {                     \
  print_errlog(__FILE__, __LINE__, err, __VA_ARGS__); \
} while(0)

#define LOGLEVEL_DEBUG  3
#define LOGLEVEL_INFO   2
#define LOGLEVEL_WARN   1
#define LOGLEVEL_FATAL  0

#define ERROR_DIE(...) do {                         \
  printf(__VA_ARGS__);                              \
  printf(" die %s:%d\n", __FILE__, __LINE__);       \
  abort();                                          \
} while(0)

#define PERROR_DIE(...) do {                                                 \
  printf(__VA_ARGS__);                                                       \
  printf(" %s(%d) die %s:%d\n", strerror(errno), errno, __FILE__, __LINE__); \
  abort();                                                                   \
} while(0)

#endif /* !defined(_COMMON_H_) */
