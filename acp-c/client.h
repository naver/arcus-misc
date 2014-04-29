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
#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "libmemcached/memcached.h"
#include "libmemcached/util/pool.h"

struct client {
  struct config *conf;
  struct keyset *ks;
  struct valueset *vs;
  struct client_profile *prof;
  int id;
  memcached_pool_st *pool;

  /* Vars to track requests */
  int rem_requests;
  memcached_st *next_mc;
  uint64_t stat_requests;
  uint64_t stat_requests_error;
  uint64_t stat_error_disabled;
  uint64_t stat_error_no_server;
  uint64_t stat_error_client;
  uint64_t stat_error_other;
  uint64_t start_time;
  uint64_t request_start_usec;
  uint64_t request_end_usec;
  struct lat_vec *lvec;

  /* Thread */
  int running;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  pthread_attr_t attr;
  pthread_t tid;
};

struct client *client_init(struct config *conf, int id, struct keyset *ks,
  struct valueset *vs, struct client_profile *profile, memcached_pool_st *pool);
int client_before_request(struct client *cli);
int client_after_request(struct client *cli, int ok);
int client_after_request_with_rc(struct client *cli, memcached_return rc);
void client_run(struct client *cli);
struct lat_vec *client_remove_latency_vector(struct client *cli);
void client_set_stop(struct client *cli);
void client_join(struct client *cli);

/* Do not support fixed memcached_st.  The client always pops a server from
 * the pool and pushes it back to the pool at the end of each request.
 */

#endif /* !defined(_CLIENT_H_) */
