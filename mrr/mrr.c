/*
 * mrr - network latency checker
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
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <stdarg.h>
#include <sys/resource.h>
#ifdef UNUSED_TEST_CODE
/* setaffinity */
#define _GNU_SOURCE 1
#define __USE_GNU 1
#endif
#include <pthread.h>
#include <sched.h>

#define CHECK_RTO_RETRANS 1
#if CHECK_RTO_RETRANS
#include <linux/tcp.h>
#endif

char *args_addr = "localhost";
int args_port = 20120;
int args_backlog = 128;
int args_s = 0;
int args_sleep = 2000;
int args_m = 100;
int args_t = 10;

#define ERROR_DIE(...) do {                         \
  log_msg( __VA_ARGS__);                            \
  printf("die %s:%d\n", __FILE__, __LINE__);        \
  exit(1);                                          \
} while(0)

#define PERROR_DIE(...) do {                                                \
  log_msg( __VA_ARGS__);                                                    \
  printf("%s(%d) die %s:%d\n", strerror(errno), errno, __FILE__, __LINE__); \
  exit(1);                                                                  \
} while(0)

void parse_args(int argc, char *argv[]);
void usage(void);
int filladdr(char *host, struct sockaddr_in *addr);
void log_msg(const char *fmt, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));

// latency histogram
// 40 bins x 20 msec = 800 msec
#define HIST_BIN_COUNT 40
#define HIST_BIN_WIDTH 20
struct hist {
  uint32_t bin[HIST_BIN_COUNT];
};

static void
hist_inc(struct hist *h, uint32_t val)
{
  uint32_t i = val / HIST_BIN_WIDTH;
  if (i >= HIST_BIN_COUNT)
    i = HIST_BIN_COUNT-1;
  h->bin[i]++;
}

uint64_t
getmsec(void)
{
  struct timeval tv;
  uint64_t msec;
  if (0 != gettimeofday(&tv, NULL)) {
    PERROR_DIE("gettimeofday");
  }
  msec = ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec/1000);
  return msec;
}

struct rr {
  int id;
  int so;
  int msg_len;
  int sleep;
  int server;
  uint32_t stat_count;
  struct hist hist;
  pthread_t tid;
  pthread_attr_t attr;
  char buf[8*1024];
};

#if CHECK_RTO_RETRANS
pthread_mutex_t checker_lock;
#define CHECK_MAX_SO 128
int checker_so_num;
int checker_so[CHECK_MAX_SO];


uint32_t max_rto = 0;
uint32_t min_rto = 0;
uint32_t max_retransmits = 0;
uint32_t max_backoff = 0;
uint32_t min_rtt = 0;
uint32_t max_rtt = 0;
uint32_t total_retrans = 0;

static void
checker_add_so(int so)
{
  pthread_mutex_lock(&checker_lock);
  checker_so[checker_so_num++] = so;
  pthread_mutex_unlock(&checker_lock);
}

static void
checker_remove_so(int so)
{
  int i;
  pthread_mutex_lock(&checker_lock);
  for (i = 0; i < checker_so_num; i++) {
    if (checker_so[i] == so) {
      checker_so[i] = -1;
      break;
    }
  }
  checker_so_num = 0;
  for (i = 0; i < CHECK_MAX_SO; i++) {
    if (checker_so[i] >= 0)
      checker_so[checker_so_num++] = checker_so[i];
  }
  pthread_mutex_unlock(&checker_lock);
}

static void *
check_thread(void *arg)
{
  /* Do nother with locking */
  while (1) {
    usleep(100);
    int i;
    for (i = 0; i < checker_so_num; i++) {
      int so = checker_so[i];
      struct tcp_info info;
      socklen_t size = sizeof(info);
      if (0 != getsockopt(so, IPPROTO_TCP, TCP_INFO, &info, &size))
        continue;

      if (max_rto == 0 || max_rto < info.tcpi_rto)
        max_rto = info.tcpi_rto;
      if (min_rto == 0 || min_rto > info.tcpi_rto)
        min_rto = info.tcpi_rto;
      if (min_rtt == 0 || min_rtt > info.tcpi_rtt)
        min_rtt = info.tcpi_rtt;
      if (max_rtt == 0 || max_rtt < info.tcpi_rtt)
        max_rtt = info.tcpi_rtt;
      if (max_retransmits == 0 || max_retransmits < info.tcpi_retransmits)
        max_retransmits = info.tcpi_retransmits;
      if (max_backoff == 0 || max_backoff < info.tcpi_backoff)
        max_backoff = info.tcpi_backoff;
    }
  }
  return NULL;
}

static void
print_checker(void)
{
  int i;
  for (i = 0; i < checker_so_num; i++) {
    int so = checker_so[i];
    struct tcp_info info;
    socklen_t size = sizeof(info);
    if (0 != getsockopt(so, IPPROTO_TCP, TCP_INFO, &info, &size))
      continue;

    total_retrans += info.tcpi_total_retrans;
  }

  printf("rto=%u:%u max_retrans=%u max_backoff=%u rtt=%u:%u tot_retrans=%u\n",
    max_rto, min_rto, max_retransmits, max_backoff, max_rtt, min_rtt,
    total_retrans);
  max_rto = min_rto = max_retransmits =  max_backoff = 0;
  max_rtt = min_rtt = total_retrans = 0;
}

static void *
stat_thread(void *arg)
{
  while (1) {
    sleep(1);
    print_checker();
  }
  return NULL;
}
#endif

static void *
rr_thread(void *arg)
{
  struct rr *r = arg;

#ifdef UNUSED_TEST_CODE
  if (r->server) {
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(2, &cpu);
    if (0 != pthread_setaffinity_np(pthread_self(), sizeof(cpu), &cpu)) {
      ERROR_DIE("pthread_setaffinity failed");
    }
  }
#endif
#if CHECK_RTO_RETRANS
  checker_add_so(r->so);
#endif
  if (r->server) {
    while (1) {
      int msg_len;
      char *buf;
      int s = read(r->so, r->buf, sizeof(r->buf));
      if (s < 0) {
        log_msg("%d: read returns an error. %s(%d). stopping\n", r->id,
          strerror(errno), errno);
        goto exit;
      }
      if (s == 0) {
        log_msg("%d: read returns eof. stopping\n", r->id);
        goto exit;
      }
      if (s < sizeof(msg_len)) {
        log_msg("%d: read only %d bytes. stopping\n", r->id, s);
        goto exit;
      }
      buf = r->buf;
      msg_len = *((int*)buf);
      if (msg_len > sizeof(r->buf) || msg_len <= 0) {
        log_msg("%d: invalid msg_len %d. stopping\n", r->id, msg_len);
        goto exit;
      }
      if (s > msg_len) {
        log_msg("%d: client sent too much. msg_len=%d s=%d. stopping\n",
          r->id, msg_len, s);
        goto exit;
      }
      if (s < msg_len) {
        int rem = msg_len - s;
        while (rem > 0) {
          s = read(r->so, r->buf+msg_len-rem, rem);
          if (s > 0)
            rem -= s;
          else if (s < 0) {
            log_msg("%d: read returns an error. %s(%d). stopping\n", r->id,
              strerror(errno), errno);
            goto exit;
          }
          else if (s == 0) {
            log_msg("%d: read returns eof. stopping\n", r->id);
            goto exit;
          }
        }
      }
      s = write(r->so, r->buf, msg_len);
      if (s != msg_len) {
        log_msg("%d: write failed. s=%d msg_len=%d error=%s(%d). stopping\n",
          r->id, s, msg_len, strerror(errno), errno);
        goto exit;
      }
    }
  }
  else {
    memset(r->buf, 0, sizeof(r->buf));
    while (1) {
      int s, rem;
      uint64_t msec;
      char *buf;

      // Sleep a little and then send a request and receive a response

      if (r->sleep > 0)
        usleep(r->sleep);

      msec = getmsec();
      buf = r->buf;
      *((int*)buf) = r->msg_len; // msg length in the first 4 bytes
      s = write(r->so, r->buf, r->msg_len);
      if (s != r->msg_len) {
        log_msg("%d: write failed. s=%d msg_len=%d error=%s(%d). stopping\n",
          r->id, s, r->msg_len, strerror(errno), errno);
        goto exit;
      }

      rem = r->msg_len;
      while (rem > 0) {
        s = read(r->so, r->buf+r->msg_len-rem, rem);
        if (s > 0)
          rem -= s;
        else if (s < 0) {
          log_msg("%d: read returns an error. %s(%d). stopping\n", r->id,
            strerror(errno), errno);
          goto exit;
        }
        else if (s == 0) {
          log_msg("%d: read returns eof. stopping\n", r->id);
          goto exit;
        }
      }
      r->stat_count++;
      msec = getmsec() - msec;
      hist_inc(&r->hist, (uint32_t)msec);
    }
  }

exit:

#if CHECK_RTO_RETRANS
  checker_remove_so(r->so);
#endif
  // For server, free the conn structure.  The main loop does not.
  if (r->server) {
    free(r);
  }
  return NULL;
}

int
main(int argc, char *argv[])
{
  struct sockaddr_in addr;
  int so;

  parse_args(argc, argv);

  if (args_addr == NULL) {
    ERROR_DIE("-addr is missing\n");
  }
  if (args_port <= 0) {
    ERROR_DIE("-port is missing\n");
  }
  if (args_m <= 0) {
    ERROR_DIE("-m must be > 0\n");
  }
  if (args_m > 8*1024) {
    ERROR_DIE("-m must be <= 8192\n");
  }
  if (args_sleep < 0) {
    ERROR_DIE("-sleep must be >= 0\n");
  }

  memset(&addr, 0, sizeof(addr));
  if (0 != filladdr(args_addr, &addr)) {
    ERROR_DIE("filladdr\n");
  }
  addr.sin_port = htons(args_port);

#if CHECK_RTO_RETRANS
  pthread_mutex_init(&checker_lock, NULL);
  checker_so_num = 0;
  memset(checker_so, -1, sizeof(checker_so));
  {
    pthread_attr_t attr;
    pthread_t tid;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_create(&tid, &attr, check_thread, NULL);
    if (args_s)
      pthread_create(&tid, &attr, stat_thread, NULL);
  }
#endif

  if (args_s) {
    int one = 1;
    int new_conn;
    int id = 0;
    struct rr *r;

    so = socket(AF_INET, SOCK_STREAM, 0);
    if (so < 0) {
      PERROR_DIE("socket");
    }
    if (0 != setsockopt(so, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
      PERROR_DIE("setsockopt(SO_REUSEADDR)");
    }
    if (0 != bind(so, (const struct sockaddr*)&addr, sizeof(addr))) {
      PERROR_DIE("bind");
    }
    if (0 != listen(so, args_backlog)) {
      PERROR_DIE("listen");
    }
    while ((new_conn = accept(so, NULL, NULL)) > 0) {
      log_msg("server accepted a conn. so=%d id=%d\n", new_conn, id);

      r = malloc(sizeof(*r));
      memset(r, 0, sizeof(*r));
      r->id = id++;
      r->so = new_conn;
      r->server = 1;
      pthread_attr_init(&r->attr);
      pthread_attr_setscope(&r->attr, PTHREAD_SCOPE_SYSTEM);
      pthread_create(&r->tid, &r->attr, rr_thread, r); // thread frees r
      // We want to use threads, not events, for this test
    }
  }
  else {
    struct rr *ra, *r;
    int i;
    struct hist prev_hist, cur_hist;
    uint32_t prev_count, cur_count;

    ra = malloc(sizeof(*ra) * args_t);
    memset(ra, 0, sizeof(*ra) * args_t);
    for (i = 0; i < args_t; i++) {
      so = socket(AF_INET, SOCK_STREAM, 0);
      if (so < 0) {
        PERROR_DIE("socket");
      }
      if (0 != connect(so, (struct sockaddr*)&addr, sizeof(addr))) {
        PERROR_DIE("connect");
      }

      log_msg("connected to server. so=%d id=%d\n", so, i);

      r = &ra[i];
      r->id = i;
      r->so = so;
      r->msg_len = args_m;
      r->sleep = args_sleep;
      pthread_attr_init(&r->attr);
      pthread_attr_setscope(&r->attr, PTHREAD_SCOPE_SYSTEM);
      pthread_create(&r->tid, &r->attr, rr_thread, r);
    }

    // Print stats forever
    memset(&prev_hist, 0, sizeof(prev_hist));
    prev_count = 0;
    while (1) {
      sleep(1);

      // Gather stats from threads
      memset(&cur_hist, 0, sizeof(cur_hist));
      cur_count = 0;
      for (i = 0; i < args_t; i++) {
        int j;
        r = &ra[i];
        cur_count += r->stat_count;
        for (j = 0; j < HIST_BIN_COUNT; j++)
          cur_hist.bin[j] += r->hist.bin[j];
      }

      printf("%u request-responses/s\n", cur_count - prev_count);
      for (i = 0; i < HIST_BIN_COUNT; i++) {
        /* if (cur_hist.bin[i] == 0) continue; */
        printf("%d: %u[%u]\n", ((i+1) * HIST_BIN_WIDTH),
          cur_hist.bin[i], cur_hist.bin[i] - prev_hist.bin[i]);
      }

      prev_count = cur_count; // struct copy
      prev_hist = cur_hist;
#if CHECK_RTO_RETRANS
      print_checker();
#endif
    }
  }

  return 0;
}

void
log_msg(const char *fmt, ...)
{
  time_t t;
  struct tm tm;
  va_list ap;

  t = time(NULL);
  localtime_r(&t, &tm);
  printf("[%04d/%02d/%02d-%02d:%02d:%02d] ",
    tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

int
filladdr(char *host, struct sockaddr_in *addr)
{
  struct hostent *h;
  h = gethostbyname(host);
  if (h != NULL) {
    if (AF_INET != h->h_addrtype) {
      ERROR_DIE("addrtype");
    }
    if (NULL == h->h_addr_list[0]) {
      ERROR_DIE("no address");
    }
    if (sizeof(struct in_addr) != h->h_length) {
      ERROR_DIE("h_length");
    }

    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = *((uint32_t*)h->h_addr_list[0]);
  }
  else {
    PERROR_DIE("gethostbyname");
  }
  return 0;
}

void
usage(void)
{
  printf(
    "mrr <options>\n"
    "-h             print mrr usage\n"
    "-s             server mode\n"
    "-addr ip       server address\n"
    "-port port     server port\n"
    "-backlog num   listen backlog\n"
    "-sleep usec    sleep <usec> microseconds between requests\n"
    "-m size        message size in bytes\n"
    "-t threads     request-response threads\n"
         );
  exit(1);
}

void
parse_args(int argc, char *argv[])
{
  int i;
  for (i = 1; i < argc; i++) {
    if (0 == strcmp(argv[i], "-addr")) {
      i++;
      if (i < argc) {
        args_addr = argv[i];
      }
      else {
        ERROR_DIE("-addr");
      }
    }
    else if (0 == strcmp(argv[i], "-port")) {
      i++;
      if (i < argc) {
        args_port = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-port");
      }
    }
    else if (0 == strcmp(argv[i], "-backlog")) {
      i++;
      if (i < argc) {
        args_backlog = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-backlog");
      }
    }
    else if (0 == strcmp(argv[i], "-sleep")) {
      i++;
      if (i < argc) {
        args_sleep = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-sleep");
      }
    }
    else if (0 == strcmp(argv[i], "-m")) {
      i++;
      if (i < argc) {
        args_m = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-m");
      }
    }
    else if (0 == strcmp(argv[i], "-t")) {
      i++;
      if (i < argc) {
        args_t = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-t");
      }
    }
    else if (0 == strcmp(argv[i], "-s")) {
      args_s = 1;
    }
    else if (0 == strcmp(argv[i], "-h")) {
      usage();
    }
    else {
      printf("Unknown option: %s\n", argv[i]);
      usage();
    }
  }
}
