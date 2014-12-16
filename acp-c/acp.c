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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

#include "libmemcached/memcached.h"
#include "libmemcached/util/pool.h"
#include "libmemcached/arcus.h"
#include "common.h"
#include "config.h"
#include "valueset.h"
#include "keyset.h"
#include "lat_vec.h"
#include "client_profile.h"
#include "client.h"

struct config conf;
struct keyset *ks;
struct valueset *vs;
struct client **all_cli;
memcached_st **all_master_mc;
memcached_pool_st **all_pool;

static void parse_args(int argc, char *argv[], struct config *conf);

static void
connect_pool(memcached_pool_st *pool, char *host, int port)
{
  if (host != NULL) {
#if 0
    if (MEMCACHED_SUCCESS != memcached_pool_use_single_server(pool,
        host, port)) {
      ERROR_DIE("use_single_server failed.");
    }
#endif
    ERROR_DIE("use_single_server is not supported.");
  }
  else {
    arcus_return_t error = arcus_pool_connect(pool, conf.zookeeper,
      conf.service_code);
    if (error != ARCUS_SUCCESS) {
      ERROR_DIE("pool_connect failed");
    }
    print_log("Connected to arcus");
  }
}

static int
setup(void)
{
  int i;
  char *single_server_host = NULL;
  int single_server_port = 0;
  int clients_per_pool;
  int pool_idx;
  int num_clients;
  struct client_profile *prof;
  bool enable_cas = false;

  if (0 == strcmp(conf.client_profile, "standard_mix"))
    prof = standard_mix_init();
  else if (0 == strcmp(conf.client_profile, "simple_set"))
    prof = simple_set_init();
  else if (0 == strcmp(conf.client_profile, "simple_getset"))
    prof = simple_getset_init(conf.client_simple_getset_get_count);
  else if (0 == strcmp(conf.client_profile, "torture_simple_decinc"))
    prof = torture_simple_decinc_init();
  else if (0 == strcmp(conf.client_profile, "torture_simple_cas")) {
    prof = torture_simple_cas_init();
    enable_cas = true;
  }
  else if (0 == strcmp(conf.client_profile, "torture_simple_zero_exptime"))
    prof = torture_simple_zero_exptime_init();
  else if (0 == strcmp(conf.client_profile, "torture_simple_sticky"))
    prof = torture_simple_sticky_init();
  else if (0 == strcmp(conf.client_profile, "torture_set"))
    prof = torture_set_init();
  else if (0 == strcmp(conf.client_profile, "torture_set_ins_del"))
    prof = torture_set_ins_del_init();
  else if (0 == strcmp(conf.client_profile, "torture_list"))
    prof = torture_list_init();
  else if (0 == strcmp(conf.client_profile, "torture_list_ins_del"))
    prof = torture_list_ins_del_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree"))
    prof = torture_btree_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree_ins_del"))
    prof = torture_btree_ins_del_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree_replace"))
    prof = torture_btree_replace_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree_decinc"))
    prof = torture_btree_decinc_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree_exptime"))
    prof = torture_btree_exptime_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree_maxbkeyrange"))
    prof = torture_btree_maxbkeyrange_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree_bytebkey"))
    prof = torture_btree_bytebkey_init();
  else if (0 == strcmp(conf.client_profile, "torture_btree_bytemaxbkeyrange"))
    prof = torture_btree_bytemaxbkeyrange_init();
  else {
    ERROR_DIE("Unknown client profile. profile=%s", conf.client_profile);
  }
  
  ks = keyset_init(conf.keyset_size, conf.key_prefix);
  vs = valueset_init(conf.valueset_min_size, conf.valueset_max_size);

  if (conf.single_server != NULL) {
    struct sockaddr_in addr;
    if (0 != parse_hostport((char*)conf.single_server, &addr,
        &single_server_host)) {
      ERROR_DIE("Failed to parse conf.single_server. server=%s",
        conf.single_server);
    }
    single_server_port = ntohs(addr.sin_port);
  }
  
  /* Create pools */
  all_master_mc = malloc(sizeof(memcached_st *) * conf.pool);
  all_pool = malloc(sizeof(memcached_pool_st *) * conf.pool);
  for (i = 0; i < conf.pool; i++) {
    all_master_mc[i] = memcached_create(NULL);
    
    /* Override POLL_TIMEOUT */
    if (conf.poll_timeout > 0) {
      uint64_t old_val, new_val;
      memcached_st *mc = all_master_mc[i];
      old_val = memcached_behavior_get(mc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT);
      new_val = conf.poll_timeout;
      if (MEMCACHED_SUCCESS != memcached_behavior_set(mc,
          MEMCACHED_BEHAVIOR_POLL_TIMEOUT, new_val)) {
        ERROR_DIE("Failed to set POLL_TIMEOUT. poll_timeout=%d",
          conf.poll_timeout);
      }
      new_val = memcached_behavior_get(mc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT);
      print_log("Changed POLL_TIMEOUT. old=%llu new=%llu",
        (long long unsigned)old_val, (long long unsigned)new_val);
    }

    if (enable_cas) {
      memcached_st *mc = all_master_mc[i];
      if (MEMCACHED_SUCCESS != memcached_behavior_set(mc,
          MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1)) {
        ERROR_DIE("Failed to set SUPPORT_CAS=1");
      }
    }
    
    all_pool[i] = memcached_pool_create(all_master_mc[i],
      conf.pool_size /* initial */, conf.pool_size /* maximum */);
    connect_pool(all_pool[i], single_server_host, single_server_port);
  }
  /* Don't bother freeing host... */

  clients_per_pool = conf.client / conf.pool;
  while (clients_per_pool * conf.pool < conf.client)
    clients_per_pool++;

  /* Create clients and assign pools to the clients */
  pool_idx = 0;
  num_clients = 0;
  all_cli = malloc(sizeof(struct client *) * conf.client);
  for (i = 0; i < conf.client; i++) {
    if (num_clients > clients_per_pool) {
      pool_idx++;
      num_clients = 0;
    }
    all_cli[i] = client_init(&conf, i, ks, vs, prof, all_pool[pool_idx]);
    num_clients++;
  }
  return 0;
}

static int stats_stop = 0;

static void *
stats_thread(void *arg)
{
  uint64_t prev_time = getmsec();
  uint64_t start_time = prev_time;
  uint64_t prev_stat_requests = 0;
  uint64_t prev_stat_requests_error = 0;
  uint64_t prev_stat_error_disabled = 0;
  uint64_t prev_stat_error_no_server = 0;
  uint64_t prev_stat_error_client = 0;
  uint64_t prev_stat_error_other = 0;
  uint64_t avg_request_rate = 0;
  uint64_t cur_time;
  uint64_t diff_time;
  uint64_t stat_requests;
  uint64_t stat_requests_error;
  uint64_t stat_error_disabled;
  uint64_t stat_error_no_server;
  uint64_t stat_error_client;
  uint64_t stat_error_other;
  uint64_t request_rate;
  int num_latencies;
  int run_time = conf.time;
  int i;
  int max_lat = 0;
  int scream = 0;
  struct lat_vec **lat_vectors;
  
  lat_vectors = malloc(sizeof(struct lat_vec*) * conf.client);
  memset(lat_vectors, 0, sizeof(struct lat_vec*) * conf.client);
  
  while (!stats_stop) {
    usleep(1000000);
    
    // Count down the run time
    if (run_time > 0) {
      run_time--;
      if (run_time <= 0) {
        print_log("Ran tests long enough. Stopping clients...");
        // Tell clients to stop
        for (i = 0; i < conf.client; i++)
          client_set_stop(all_cli[i]);
      }
    }
    
    cur_time = getmsec();
    diff_time = (cur_time - prev_time) / 1000;
    prev_time = cur_time;
    
    // Gather stats from each client
    stat_requests = 0;
    stat_requests_error = 0;
    stat_error_disabled = 0;
    stat_error_no_server = 0;
    stat_error_client = 0;
    stat_error_other = 0;
    num_latencies = 0;
    for (i = 0; i < conf.client; i++) {
      struct client *cli = all_cli[i];
      struct lat_vec *lvec;
      
      stat_requests += cli->stat_requests;
      stat_requests_error += cli->stat_requests_error;
      stat_error_disabled += cli->stat_error_disabled;
      stat_error_no_server += cli->stat_error_no_server;
      stat_error_client += cli->stat_error_client;
      stat_error_other += cli->stat_error_other;
      
      lvec = client_remove_latency_vector(cli);
      lat_vectors[i] = lvec;
      if (lvec != NULL)
        num_latencies += lvec->num;
    }
    prev_stat_requests = stat_requests - prev_stat_requests;
    prev_stat_requests_error = stat_requests_error - prev_stat_requests_error;
    prev_stat_error_disabled = stat_error_disabled - prev_stat_error_disabled;
    prev_stat_error_no_server = stat_error_no_server -
      prev_stat_error_no_server;
    prev_stat_error_client = stat_error_client - prev_stat_error_client;
    prev_stat_error_other = stat_error_other - prev_stat_error_other;
    request_rate = prev_stat_requests/diff_time;
    avg_request_rate = (95 * avg_request_rate + 5 * request_rate) / 100;
    print_log("elapsed(s)=%d requests/s=%d [error=%d]"
      " cumulative requests/client=%d [error=%d]"
      " cumulative requests/s=%d",
      (int)((cur_time - start_time) / 1000), (int)request_rate,
      (int)(prev_stat_requests_error/diff_time),
      (int)(stat_requests / conf.client),
      (int)(stat_requests_error / conf.client),
      (int)avg_request_rate);
    if (prev_stat_requests_error > 0) {
      print_log("error types. disabled=%d no_server=%d client=%d other=%d",
        (int)prev_stat_error_disabled,
        (int)prev_stat_error_no_server,
        (int)prev_stat_error_client,
        (int)prev_stat_error_other);
    }
    prev_stat_requests = stat_requests;
    prev_stat_requests_error = stat_requests_error;
    prev_stat_error_disabled = stat_error_disabled;
    prev_stat_error_no_server = stat_error_no_server;
    prev_stat_error_client = stat_error_client;
    prev_stat_error_other = stat_error_other;

    if (num_latencies > 0) {
      struct lat_vec *all = lat_vec_init(num_latencies);
      for (i = 0; i < conf.client; i++) {
        struct lat_vec *lvec = lat_vectors[i];
        if (lvec != NULL) {
          int j;
          for (j = 0; j < lvec->num; j++)
            lat_vec_add(all, lvec->lat[j]);
        }
      }
      
      lat_vec_sort(all);
      int p = all->num / 100;
      if (all->num > 0) {
        int max = (int)(all->lat[all->num-1]);
        if (max > max_lat)
          max_lat = max;
        print_log("latency (usec). min=%d 10th=%d"
          " 50th=%d 80th=%d 90th=%d 99th=%d max=%d max_so_far=%d scream=%d",
          (int)(all->lat[0]),
          (int)(all->lat[p * 10]),
          (int)(all->lat[p * 50]),
          (int)(all->lat[p * 80]),
          (int)(all->lat[p * 90]),
          (int)(all->lat[p * 99]),
          max, max_lat, scream);
        if (conf.scream > 0 && max >= conf.scream) {
          print_log("SCREAM");
          scream++;
        }
      }
      
      lat_vec_free(all);
    }

    for (i = 0; i < conf.client; i++) {
      struct lat_vec *lvec = lat_vectors[i];
      if (lvec != NULL) {
        lat_vec_free(lvec);
        lat_vectors[i] = NULL;
      }
    }
  }
  return NULL;
}

static void
run_bench(void)
{
  int i;
  pthread_t tid;
  pthread_attr_t attr;
  void *ret;

  // Start stats thread
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_create(&tid, &attr, stats_thread, NULL);
  
  // Start all client threads
  for (i = 0; i < conf.client; i++)
    client_run(all_cli[i]);

  // Wait for all clients to terminate
  for (i = 0; i < conf.client; i++)
    client_join(all_cli[i]);
  print_log("All clients have stopped");

  // Wait for stats thread to terminate
  stats_stop = 1;
  pthread_join(tid, &ret);
  print_log("Stats thread has stopped");

  print_log("Tests finished");
}

int
main(int argc, char *argv[])
{
  config_default_init(&conf);
  parse_args(argc, argv, &conf);
  
  if (0 == setup()) {
    run_bench();
  }
  
  return 0;
}

static void
usage(void)
{
  printf(
    "acp options\n"
    "-config path\n"
    "    Use configuration file at <path>\n"
    "\n"
    "--------------------------------------\n"
    "Use the following to override settings\n"
    "--------------------------------------\n"
    "-zookeeper host:port\n"
    "    Zookeeper server address\n"
    "-service-code svc\n"
    "    Use <svc> to find the cluster in ZK\n"
    "-client num\n"
    "    Use <num> Arcus clients\n"
    "-rate rps\n"
    "    Each client does <rps> requests per second.\n"
    "    0=as fast as possible\n"
    "-request num\n"
    "    Each client does <num> requests and quits.\n"
    "    0=infinite\n"
    "-time sec\n"
    "    Run for <sec> seconds and quit.\n"
    "    0=forever\n"
    "-pool num\n"
    "    Use <num> ArcusClientPool's\n"
    "-pool-size num\n"
    "    Use <num> ArcusClient's per ArcusClientPool\n"
    "-quit-on-error\n"
    "    Terminate the process when a request returns an error.\n"
    "-poll-timeout msec\n"
    "    Use <msec> POLL_TIMEOUT\n"
    "-scream usec\n"
    "    Print eye catching lines when max response times are greater\n"
    "    than <usec>\n"
    "-quiet\n"
    "\n"
    "Example: acp -zookeeper 127.0.0.1:2181 -service-code test"
    " -clients 100 -rate 1000\n"
         );
  exit(0);
}

static void
parse_args(int argc, char *argv[], struct config *conf)
{
  int i;
  for (i = 1; i < argc; i++) {
    if (0 == strcmp(argv[i], "-config")) {
      i++;
      if (i < argc) {
        config_read(conf, argv[i]);
      }
      else {
        ERROR_DIE("-config");
      }
    }
    else if (0 == strcmp(argv[i], "-zookeeper")) {
      i++;
      if (i < argc) {
        conf->zookeeper = strdup(argv[i]);
      }
      else {
        ERROR_DIE("-zookeeper");
      }
    }
    else if (0 == strcmp(argv[i], "-service-code")) {
      i++;
      if (i < argc) {
        conf->service_code = strdup(argv[i]);
      }
      else {
        ERROR_DIE("-service-code");
      }
    }
    else if (0 == strcmp(argv[i], "-client")) {
      i++;
      if (i < argc) {
        conf->client = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-client");
      }
    }
    else if (0 == strcmp(argv[i], "-rate")) {
      i++;
      if (i < argc) {
        conf->rate = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-rate");
      }
    }
    else if (0 == strcmp(argv[i], "-request")) {
      i++;
      if (i < argc) {
        conf->request = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-request");
      }
    }
    else if (0 == strcmp(argv[i], "-time")) {
      i++;
      if (i < argc) {
        conf->time = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-time");
      }
    }
    else if (0 == strcmp(argv[i], "-pool")) {
      i++;
      if (i < argc) {
        conf->pool = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-pool");
      }
    }
    else if (0 == strcmp(argv[i], "-pool-size")) {
      i++;
      if (i < argc) {
        conf->pool_size = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-pool-size");
      }
    }
    else if (0 == strcmp(argv[i], "-client-profile")) {
      i++;
      if (i < argc) {
        conf->client_profile = strdup(argv[i]);
      }
      else {
        ERROR_DIE("-client-profile");
      }
    }
    else if (0 == strcmp(argv[i], "-poll-timeout")) {
      i++;
      if (i < argc) {
        conf->poll_timeout = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-poll-timeout");
      }
    }
    else if (0 == strcmp(argv[i], "-scream")) {
      i++;
      if (i < argc) {
        conf->scream = atoi(argv[i]);
      }
      else {
        ERROR_DIE("-scream");
      }
    }
    else if (0 == strcmp(argv[i], "-quit-on-error")) {
      conf->quit_on_error = 1;
    }
    else if (0 == strcmp(argv[i], "-quiet")) {
      conf->quiet = 1;
    }
    else {
      printf("Unknown option: %s\n", argv[i]);
      usage();
    }
  }
}
