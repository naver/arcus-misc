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
#ifndef _CONFIG_H_
#define _CONFIG_H_

struct config {
  const char *zookeeper;
  const char *service_code;
  const char *single_server;
  int client;
  int rate;
  int request;
  int time;
  int pool;
  int pool_size;
  int keyset_size;
  int valueset_min_size;
  int valueset_max_size;
  const char *client_profile;
  const char *key_prefix;
  int quit_on_error;
  int poll_timeout;
  int scream;
  int quiet;
  int client_simple_getset_get_count;
};

void config_default_init(struct config *conf);
int config_read(struct config *conf, const char *path);

#endif /* !defined(_CONFIG_H_) */
