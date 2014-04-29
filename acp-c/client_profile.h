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
#ifndef _CLIENT_PROFILE_H_
#define _CLIENT_PROFILE_H_

struct client;

struct client_profile {
  int (*do_test)(struct client *);
};

struct client_profile *standard_mix_init(void);
struct client_profile *simple_set_init(void);
struct client_profile *simple_getset_init(int get_count);
struct client_profile *torture_simple_decinc_init(void);
struct client_profile *torture_simple_cas_init(void);
struct client_profile *torture_simple_zero_exptime_init(void);
struct client_profile *torture_simple_sticky_init(void);
struct client_profile *torture_btree_init(void);
struct client_profile *torture_btree_ins_del_init(void);
struct client_profile *torture_btree_replace_init(void);
struct client_profile *torture_btree_decinc_init(void);
struct client_profile *torture_btree_exptime_init(void);
struct client_profile *torture_btree_maxbkeyrange_init(void);
struct client_profile *torture_btree_bytebkey_init(void);
struct client_profile *torture_btree_bytemaxbkeyrange_init(void);
struct client_profile *torture_set_init(void);
struct client_profile *torture_set_ins_del_init(void);
struct client_profile *torture_list_init(void);
struct client_profile *torture_list_ins_del_init(void);

#endif /* !defined(_CLIENT_PROFILE_H_) */
