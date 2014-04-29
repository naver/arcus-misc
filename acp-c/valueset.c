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
#include "valueset.h"

struct valueset {
  int min;
  int max;
  int next_size;
  int next_off;
  uint8_t *buf;
  pthread_mutex_t lock;
  int seq;
};

struct valueset *
valueset_init(int min, int max)
{
  struct valueset *vs;
  int i;
  
  vs = malloc(sizeof(*vs));
  memset(vs, 0, sizeof(*vs));
  pthread_mutex_init(&vs->lock, NULL);
  vs->min = min;
  vs->max = max;

  /* Make a one big buffer and fill the whole thing with simple patterns.
   * 0 1 2 ... 255 0 1 2 ...
   */
  vs->buf = malloc(max);
  for (i = 0; i < max; i++) {
    vs->buf[i] = (uint8_t)i;
  }
  
  valueset_reset(vs);
  return vs;
}

void
valueset_reset(struct valueset *vs)
{
  vs->next_size = vs->min;
  vs->next_off = 0;
  
  vs->seq = 0;
}

int
valueset_get_value(struct valueset *vs, uint8_t **ptr)
{
  int size, rem, seq;
  uint8_t *c;
  
  pthread_mutex_lock(&vs->lock);
  
  size = vs->next_size;
  vs->next_size += 10;
  if (vs->next_size >= vs->max)
    vs->next_size = vs->min;
  
  seq = vs->seq++;
  
  pthread_mutex_unlock(&vs->lock);

  c = malloc(size);
  *ptr = c;
  rem = size;
  
  while (rem >= 4) {
    *(int*)c = seq;
    c += 4;
    seq += 4;
    rem -= 4;
  }
  while (rem > 0) {
    *c = (uint8_t)seq;
    c++;
    seq++;
    rem--;
  }
  
  return size;
}

void
valueset_return_value(struct valueset *vs, uint8_t *ptr)
{
  /* FIXME.  Make a per-client cache or something... */
  free((void*)ptr);
}
