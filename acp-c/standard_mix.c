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

static int
do_btree_test(struct client *cli)
{
  memcached_coll_create_attrs_st attr;
  memcached_return rc;
  int i, ok, keylen;
  uint64_t bkey;
  
  // Pick a key
  const char *key = keyset_get_key(cli->ks, NULL);
  keylen = strlen(key);
  
  // Create a btree item
  if (0 != client_before_request(cli))
    return -1;
  
  memcached_coll_create_attrs_init(&attr, 20 /* flags */, 100 /* exptime */,
    4000 /* maxcount */);
  memcached_coll_create_attrs_set_overflowaction(&attr,
    OVERFLOWACTION_SMALLEST_TRIM);
  rc = memcached_bop_create(cli->next_mc, key, keylen, &attr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    print_log("bop create failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request(cli, ok))
    return -1;
  
  // Insert a number of btree element
  for (i = 0; i < 100; i++) {
    uint8_t *val_ptr;
    int val_len;
    
    if (0 != client_before_request(cli))
      return -1;
    
    bkey = 1234 + i;

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
  
  // Update
  // Get/delete
  // Upsert
  // Incr/decr
  // Expire time
  // Smget
  // Getattr
  // PipedInsert
  return 0;
}

static int
do_set_test(struct client *cli)
{
  memcached_return rc;
  memcached_coll_create_attrs_st attr;
  int i, ok, keylen;
  const char *key;
  
  // Pick a key
  key = keyset_get_key(cli->ks, NULL);
  keylen = strlen(key);
  
  // Create a set item
  if (0 != client_before_request(cli))
    return -1;

  memcached_coll_create_attrs_init(&attr, 10 /* flags */, 100 /* exptime */,
    4000 /* maxcount */);
  memcached_coll_create_attrs_set_overflowaction(&attr, OVERFLOWACTION_ERROR);
  rc = memcached_sop_create(cli->next_mc, key, keylen, &attr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    print_log("sop create failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request(cli, ok))
    return -1;
  
  // Insert a number of elements.  Set has no element keys.
  for (i = 0; i < 100; i++) {
    uint8_t *val_ptr;
    int val_len;
    
    if (0 != client_before_request(cli))
      return -1;
    
    val_ptr = NULL;
    val_len = valueset_get_value(cli->vs, &val_ptr);
    assert(val_ptr != NULL && val_len > 0 && val_len <= 4096);

    rc = memcached_sop_insert(cli->next_mc, key, keylen,
      (const char*)val_ptr, (size_t)val_len,
      NULL /* Do not create automatically */);
    valueset_return_value(cli->vs, val_ptr);
    ok = (rc == MEMCACHED_SUCCESS);
    if (!ok) {
      print_log("sop insert failed. id=%d key=%s val_len=%d rc=%d(%s)",
        cli->id, key, val_len, rc, memcached_strerror(NULL, rc));
    }
    if (0 != client_after_request(cli, ok))
      return -1;
  }
  
  // Get/delete
  // Delete
  // PipedInsertBulk
  // Expire time
  // Getattr
  return 0;
}

static int
do_list_test(struct client *cli)
{
  memcached_return rc;
  memcached_coll_create_attrs_st attr;
  int i, ok, keylen;
  const char *key;
  
  // Pick a key
  key = keyset_get_key(cli->ks, NULL);
  keylen = strlen(key);
    
  // Create a list item
  if (0 != client_before_request(cli))
    return -1;
  
  memcached_coll_create_attrs_init(&attr, 10 /* flags */, 100 /* exptime */,
    4000 /* maxcount */);
  memcached_coll_create_attrs_set_overflowaction(&attr,
    OVERFLOWACTION_TAIL_TRIM);
  rc = memcached_lop_create(cli->next_mc, key, keylen, &attr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    print_log("lop create failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request(cli, ok))
    return -1;
  
  // Insert a number of elements.  Push at the head.
  for (i = 0; i < 100; i++) {
    uint8_t *val_ptr;
    int val_len;
    
    if (0 != client_before_request(cli))
      return -1;
    
    val_ptr = NULL;
    val_len = valueset_get_value(cli->vs, &val_ptr);
    assert(val_ptr != NULL && val_len > 0 && val_len <= 4096);

    rc = memcached_lop_insert(cli->next_mc, key, keylen,
      0 /* 0=head */,
      (const char*)val_ptr, (size_t)val_len,
      NULL /* Do not create automatically */);
    valueset_return_value(cli->vs, val_ptr);
    ok = (rc == MEMCACHED_SUCCESS);
    if (!ok) {
      print_log("lop insert failed. id=%d key=%s rc=%d(%s)", cli->id, key,
        rc, memcached_strerror(NULL, rc));
    }
    if (0 != client_after_request(cli, ok))
      return -1;
  }
  
  // Get/delete
  // Delete
  // PipedInsert
  // Expire time
  // Getattr
  return 0;
}

static int
do_simple_test(struct client *cli)
{
  memcached_return rc;
  int i, ok, keylen;
  const char *key;
  uint8_t *val_ptr;
  int val_len;
  
  // Set a number of items
  for (i = 0; i < 100; i++) {
    if (0 != client_before_request(cli))
      return -1;
    
    // Pick a key
    key = keyset_get_key(cli->ks, NULL);
    keylen = strlen(key);

    // Pick a value
    val_ptr = NULL;
    val_len = valueset_get_value(cli->vs, &val_ptr);
    assert(val_ptr != NULL && val_len > 0);
    
    rc = memcached_set(cli->next_mc, key, keylen, (const char*)val_ptr,
      (size_t)val_len, 100 /* exptime */, 0 /* flags */);
    valueset_return_value(cli->vs, val_ptr);
    ok = (rc == MEMCACHED_SUCCESS);
    if (!ok) {
      print_log("set failed. id=%d key=%s rc=%d(%s)", cli->id, key,
        rc, memcached_strerror(NULL, rc));
    }
    if (0 != client_after_request(cli, ok))
      return -1;
  }
  
  // Incr/decr
  // Get/delete
  // Update
  // Multiget
  // Expire time
  // Getattr
  // Cas
  // Add
  // Replace
  return 0;
}

static int
do_test(struct client *cli)
{
  if (0 != do_btree_test(cli))
    return -1;
  
  if (0 != do_set_test(cli))
    return -1;
  
  if (0 != do_list_test(cli))
    return -1;
  
  if (0 != do_simple_test(cli))
    return -1; // Stop the test
  
  return 0; // Do another test
}

static struct client_profile default_profile = {
  .do_test = do_test,
};

struct client_profile *
standard_mix_init(void)
{
  return &default_profile;
}
