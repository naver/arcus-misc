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
#include <assert.h>

#include "libmemcached/memcached.h"
#include "common.h"
#include "keyset.h"
#include "valueset.h"
#include "client_profile.h"
#include "client.h"

// create a btree with maxcount=10000, maxrange=4000
// then insert 10000 elements

static int
do_btree_test(struct client *cli)
{
  memcached_coll_create_attrs_st create_attr;
  memcached_coll_attrs_st coll_attr;
  memcached_return rc;
  int ok, keylen, base;
  const char *key;
  uint64_t bkey;
  uint8_t *val_ptr;
  int val_len;

  // Pick a key
  key = keyset_get_key(cli->ks, &base);
  keylen = strlen(key);

  // Create a btree item
  if (0 != client_before_request(cli))
    return -1;
  
  memcached_coll_create_attrs_init(&create_attr, 20 /* flags */,
    100 /* exptime */, 10000 /* maxcount */);  
  memcached_coll_create_attrs_set_overflowaction(&create_attr,
    OVERFLOWACTION_SMALLEST_TRIM);
  rc = memcached_bop_create(cli->next_mc, key, keylen, &create_attr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    print_log("bop create failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request(cli, ok))
    return -1;

  // Set maxbkeyrange.
  // As we insert 10000 elements, only 4000+1 should remain in the tree
  if (0 != client_before_request(cli))
    return -1;

  rc = memcached_coll_attrs_init(&coll_attr);
  assert(rc == MEMCACHED_SUCCESS);
  rc = memcached_coll_attrs_set_maxbkeyrange(&coll_attr, 4000);
  assert(rc == MEMCACHED_SUCCESS);
  rc = memcached_set_attrs(cli->next_mc, key, keylen, &coll_attr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    print_log("bop set attr failed. id=%d key=%s rc=%d(%s)",
      cli->id, key, rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request(cli, ok))
    return -1;
  
  // Insert elements
  for (bkey = base; bkey < base + 10000; bkey++) {
    if (0 != client_before_request(cli))
      return -1;

    val_ptr = NULL;
    val_len = valueset_get_value(cli->vs, &val_ptr);
    assert(val_ptr != NULL && val_len > 0 && val_len <= 4096);
    
    rc = memcached_bop_insert(cli->next_mc, key, keylen,
      bkey,
      NULL /* eflag */, 0 /* eflag length */,
      (const char*)val_ptr, (size_t)val_len,
      NULL /* Do not create automatically */);
    valueset_return_value(cli->vs, val_ptr);
    ok = (rc == MEMCACHED_SUCCESS);
    if (!ok) {
      print_log("bop insert failed. id=%d key=%s bkey=%llu rc=%d(%s)",
        cli->id, key, (long long unsigned)bkey,
        rc, memcached_strerror(NULL, rc));
    }
    if (0 != client_after_request(cli, ok))
      return -1;
  }

  return 0;
}

static int
do_test(struct client *cli)
{
  if (0 != do_btree_test(cli))
    return -1; // Stop the test
  
  return 0; // Do another test
}

static struct client_profile default_profile = {
  .do_test = do_test,
};

struct client_profile *
torture_btree_maxbkeyrange_init(void)
{
  return &default_profile;
}
