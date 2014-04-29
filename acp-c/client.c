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
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include "libmemcached/memcached.h"
#include "libmemcached/util/pool.h"
#include "libmemcached/arcus.h"
#include "common.h"
#include "config.h"
#include "keyset.h"
#include "valueset.h"
#include "lat_vec.h"
#include "client_profile.h"
#include "client.h"

struct client *
client_init(struct config *conf, int id, struct keyset *ks,
  struct valueset *vs, struct client_profile *profile, memcached_pool_st *pool)
{
  struct client *cli;

  cli = malloc(sizeof(*cli));
  memset(cli, 0, sizeof(*cli));

  cli->conf = conf;
  cli->ks = ks;
  cli->vs = vs;
  cli->id = id;
  cli->prof = profile;
  cli->pool = pool;
  cli->running = 0;
  pthread_mutex_init(&cli->lock, NULL);
  pthread_cond_init(&cli->cond, NULL);
  pthread_attr_init(&cli->attr);
  pthread_attr_setscope(&cli->attr, PTHREAD_SCOPE_SYSTEM);
  return cli;
}

int
client_before_request(struct client *cli)
{
  struct config *conf = cli->conf;
  memcached_return rc;
  
  // Rate control
  if (conf->rate > 0) {
    // Send one request now or sleep 1ms?
    int send_now = 0;
    while (!send_now) {
      int runtime = (int)((getmsec() - cli->start_time)/1000);
      if (runtime > 0) {
        int rate = (int)(cli->stat_requests/runtime);
        if (rate < conf->rate)
          send_now = 1;
        else {
          usleep(1000);
        }
      }
    }
  }
  
  // Get the server
  cli->next_mc = memcached_pool_pop(cli->pool, true, &rc);
  if (rc != MEMCACHED_SUCCESS) {
    print_log("memcached_pool_pop failed. rc=%d", rc);
    return -1;
  }
  
  cli->stat_requests++;
  if (cli->rem_requests == 0) {
    // Run forever.
    // acp counts down the run time and tells this thread to stop
    // when the time runs out.
  }
  else {
    // Count down, and stop if there are no requests remaining.
    cli->rem_requests--;
    if (cli->rem_requests <= 0)
      cli->rem_requests = -1; // Hack
  }
  
  // Response time
  cli->request_start_usec = getusec();
  return 0;
}

struct lat_vec *
client_remove_latency_vector(struct client *cli)
{
  struct lat_vec *lvec;
  pthread_mutex_lock(&cli->lock);
  lvec = cli->lvec;
  cli->lvec = NULL;
  pthread_mutex_unlock(&cli->lock);
  return lvec;
}

static void
add_latency(struct client *cli, uint64_t lat)
{
  pthread_mutex_lock(&cli->lock);
  if (cli->lvec == NULL) {
    int limit = cli->conf->rate * 2;
    if (limit == 0)
      limit = 10000;
    cli->lvec = lat_vec_init(limit);
  }
  lat_vec_add(cli->lvec, lat);
  pthread_mutex_unlock(&cli->lock);
}

int
client_after_request(struct client *cli, int ok)
{
  // Return the server to the pool
  memcached_pool_push(cli->pool, cli->next_mc);
  cli->next_mc = NULL;
  
  if (ok) {
    // Response time.  Only count success responses.  FIXME
    cli->request_end_usec = getusec();
    if (cli->request_end_usec >= cli->request_start_usec) {
      uint64_t lat = cli->request_end_usec - cli->request_start_usec;
      add_latency(cli, lat);
    }
    else {
      // Ignore it.  Do not bother with wraparound.
    }
  }
  else {
    cli->stat_requests_error++;
    if (cli->conf->quit_on_error) {
      print_log("quit_on_error");
      exit(0);
    }
  }
  
  if (cli->rem_requests == -1)
    return -1; // Stop the test
  return 0;
}

int
client_after_request_with_rc(struct client *cli, memcached_return rc)
{
  if (rc == MEMCACHED_SUCCESS) {
    // Response time.  Only count success responses.  FIXME
    cli->request_end_usec = getusec();
    if (cli->request_end_usec >= cli->request_start_usec) {
      uint64_t lat = cli->request_end_usec - cli->request_start_usec;
      add_latency(cli, lat);
    }
    else {
      // Ignore it.  Do not bother with wraparound.
    }
  }
  else {
    cli->stat_requests_error++;
    if (rc == MEMCACHED_SERVER_TEMPORARILY_DISABLED)
      cli->stat_error_disabled++;
    else if (rc == MEMCACHED_NO_SERVERS)
      cli->stat_error_no_server++;
    else if (rc == MEMCACHED_CLIENT_ERROR)
      cli->stat_error_client++;
    else
      cli->stat_error_other++;
    /*
    if (rc == MEMCACHED_SERVER_TEMPORARILY_DISABLED) {
      print_log("Quit on DISABLED");
      exit(0);
    }
    */
    if (cli->conf->quit_on_error) {
      print_log("quit_on_error. rc=%d(%s)", rc, memcached_strerror(NULL, rc));
      exit(0);
    }
  }
  
  // Return the server to the pool
  memcached_pool_push(cli->pool, cli->next_mc);
  cli->next_mc = NULL;
  
  if (cli->rem_requests == -1)
    return -1; // Stop the test
  return 0;
}

static void*
client_thread(void *arg)
{
  struct client *cli = arg;
  print_log("Client is running. id=%d", cli->id);
  
  cli->rem_requests = cli->conf->request;
  cli->start_time = getmsec();
  
  while (cli->running) {
    int do_another = cli->prof->do_test(cli);
    if (0 != do_another) {
      cli->running = 0;
      /* Quit now */
    }
  }
  print_log("Client stopped. id=%d", cli->id);
  return NULL;
}

void
client_set_stop(struct client *cli)
{
  cli->running = 0;
}

void
client_run(struct client *cli)
{
  cli->running = 1;
  pthread_create(&cli->tid, &cli->attr, client_thread, cli);
}

void
client_join(struct client *cli)
{
  void *ret;
  pthread_join(cli->tid, &ret);
}
