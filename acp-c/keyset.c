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
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common.h"
#include "keyset.h"

struct keyset {
  const char **set;
  int set_size;
  int next_idx;
  pthread_mutex_t lock;
};

struct keyset *
keyset_init(int num, const char *prefix)
{
  struct keyset *ks;
  char buf[128];
  int i;

  ks = malloc(sizeof(*ks));
  memset(ks, 0, sizeof(*ks));
  pthread_mutex_init(&ks->lock, NULL);
  ks->set_size = num;
  ks->set = malloc(num * sizeof(char*));
  for (i = 0; i < num; i++) {
    if (prefix != NULL)
      sprintf(buf, "%stestkey-%d", prefix, i);
    else
      sprintf(buf, "testkey-%d", i);
    ks->set[i] = strdup(buf);
  }
  
  keyset_reset(ks);
  return ks;
}

void
keyset_reset(struct keyset *ks)
{
  ks->next_idx = 0;
}

const char *
keyset_get_key(struct keyset *ks, int *id)
{
  int idx;
  const char *key;
  
  pthread_mutex_lock(&ks->lock);
  idx = ks->next_idx;
  ks->next_idx++;
  if (ks->next_idx >= ks->set_size)
    ks->next_idx = 0;
  key = ks->set[idx];
  pthread_mutex_unlock(&ks->lock);
  if (id != NULL)
    *id = idx;
  return key;
}
