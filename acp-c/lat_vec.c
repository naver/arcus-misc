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
#include <netinet/in.h>

#include "common.h"
#include "config.h"
#include "lat_vec.h"

struct lat_vec *
lat_vec_init(int max_elem)
{
  int size = sizeof(struct lat_vec) + sizeof(uint64_t) * (max_elem-1);
  struct lat_vec *v = malloc(size);
  v->max = max_elem;
  v->num = 0;
  return v;
}

void
lat_vec_free(struct lat_vec *v)
{
  free(v);
}

void
lat_vec_add(struct lat_vec *v, uint64_t lat)
{
  if (v->num < v->max) {
    v->lat[v->num++] = lat;
  }
}

static int
uint64_compare(const void *a, const void *b)
{
  uint64_t ai = *(uint64_t*)a;
  uint64_t bi = *(uint64_t*)b;
  if (ai < bi)
    return -1;
  else if (ai > bi)
    return 1;
  else
    return 0;
}

void
lat_vec_sort(struct lat_vec *v)
{
  qsort(v->lat, v->num, sizeof(uint64_t), uint64_compare);
}
