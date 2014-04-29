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
#ifndef _LAT_VEC_H_
#define _LAT_VEC_H_

struct lat_vec {
  int max;
  int num;
  uint64_t lat[1];
};

struct lat_vec *lat_vec_init(int max_elem);
void lat_vec_free(struct lat_vec *v);
/* Not thread safe.  Locking is up to the caller. */
void lat_vec_add(struct lat_vec *v, uint64_t lat);
void lat_vec_sort(struct lat_vec *v);

#endif /* !defined(_LAT_VEC_H_) */
