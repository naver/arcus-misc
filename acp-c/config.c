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
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "common.h"
#include "config.h"

struct config default_conf = {
  .zookeeper = "127.0.0.1:2181",
  .service_code = "test",
  .single_server = NULL,
  .client = 10,
  .rate = 0,
  .request = 10000,
  .time = 0,
  .pool = 1,
  .pool_size = 10,
  .keyset_size = 100000,
  .valueset_min_size = 10,
  .valueset_max_size = 4096,
  .client_profile = "standard_mix",
  .key_prefix = NULL,
  .quit_on_error = 0,
  .poll_timeout = 0,
  .scream = 500000,
  .quiet = 0,
  .client_simple_getset_get_count = 1,
};

void
config_default_init(struct config *conf)
{
  *conf = default_conf;
}

static int
config_key_val(struct config *o, int line_num, char *key, char *val)
{
  int err = 0;

  if (strcmp(key, "zookeeper") == 0) {
    o->zookeeper = strdup(val);
  }
  else if (strcmp(key, "service_code") == 0) {
    o->service_code = strdup(val);
  }
  else if (strcmp(key, "single_server") == 0) {
    o->single_server = strdup(val);
  }
  else if (strcmp(key, "client") == 0) {
    int n = atoi(val);
    o->client = n;
  }
  else if (strcmp(key, "rate") == 0) {
    int n = atoi(val);
    o->rate = n;
  }
  else if (strcmp(key, "request") == 0) {
    int n = atoi(val);
    o->request = n;
  }
  else if (strcmp(key, "time") == 0) {
    int n = atoi(val);
    o->time = n;
  }
  else if (strcmp(key, "pool") == 0) {
    int n = atoi(val);
    o->pool = n;
  }
  else if (strcmp(key, "pool_size") == 0) {
    int n = atoi(val);
    o->pool_size = n;
  }
  else if (strcmp(key, "keyset_size") == 0) {
    int n = atoi(val);
    o->keyset_size = n;
  }
  else if (strcmp(key, "valueset_min_size") == 0) {
    int n = atoi(val);
    o->valueset_min_size = n;
  }
  else if (strcmp(key, "valueset_max_size") == 0) {
    int n = atoi(val);
    o->valueset_max_size = n;
  }
  else if (strcmp(key, "client_profile") == 0) {
    o->client_profile = strdup(val);
  }
  else if (strcmp(key, "client_simple_getset_get_count") == 0) {
    int n = atoi(val);
    o->client_simple_getset_get_count = n;
  }
  else if (strcmp(key, "key_prefix") == 0) {
    o->key_prefix = strdup(val);
  }
  else {
    print_log("Unknown key. line=%d key=%s", line_num, key);
    err = -1;
  }
  
  return err;
}

static int
config_line(struct config *o, int line_num, char line[])
{
  char *c;
  int found_key = 0, empty = 1;
  
  //printf("config line %d: %s\n", line_num, line);
  
  // key=value
  // no spaces in key names, no spaces between key and =
  // everything that follows = is the value, including spaces
  
  c = line;
  if (*c == '#') {
    // comment line
    return 0;
  }
  while (*c != '\0') {
    if (*c != ' ' && *c != '\t')
      empty = 0;
    if (*c == '=') {
      char *key = line;
      char *val = c+1;
      *c = '\0';
      found_key = 1;
      if (0 != config_key_val(o, line_num, key, val)) {
        return -1;
      }
      break;
    }
    c++;
  }
  
  if (!empty && !found_key) {
    print_log("Invalid line in the configuration file. line=%d", line_num);
    return -1;
  }
  
  return 0;
}

int
config_read(struct config *o, const char *path)
{
  int fd, s, i, line_e, line_num;
  char buf[1024], line[1024];
  
  fd = open(path, O_RDONLY);
  if (fd < 0) {
    ERRLOG(errno, "Cannot open configuration. path=%s", path);
    return -1;
  }

  line_num = 0;
  line_e = 0;
  while ((s = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < s; i++) {
      if (line_e >= sizeof(line)) {
        print_log("A line in the configuration file is too long. line=%d",
          line_num);
        return -1;
      }
      line[line_e++] = buf[i];
      if (buf[i] == '\r') {
        line_e--;
      }
      else if (buf[i] == '\n') {
        line_e--;
        line[line_e] = '\0';
        
        // got a line
        line_num++;
        line_e = 0;
        if (0 != config_line(o, line_num, line)) {
          return -1;
        }
      }
    }
  }

  if (line_e > 0) {
    // last line without newline char
    line_num++;
    if (0 != config_line(o, line_num, line)) {
      return -1;
    }
  }
  return 0;
}
