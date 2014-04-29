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
#include "config.h"
#include "keyset.h"
#include "valueset.h"
#include "client_profile.h"
#include "client.h"

static int
do_simple_test(struct client *cli)
{
  memcached_return rc, error;
  int ok, keylen, base;
  const char *key;
  uint8_t *val_ptr;
  int val_len;
  uint64_t cas;
  memcached_result_st result, *result_ptr;

  // Pick a key
  key = keyset_get_key(cli->ks, &base);
  keylen = strlen(key);

  // Insert an item
  if (0 != client_before_request(cli))
    return -1;
  val_ptr = NULL;
  val_len = valueset_get_value(cli->vs, &val_ptr);
  assert(val_ptr != NULL && val_len > 0);
  rc = memcached_set(cli->next_mc, key, keylen, (const char*)val_ptr,
    (size_t)val_len, 100 /* exptime */, 0 /* flags */);
  valueset_return_value(cli->vs, val_ptr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok && !cli->conf->quiet) {
    print_log("set failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;
  
  // Gets (mget)
  if (0 != client_before_request(cli))
    return -1;
  {
    size_t len = keylen;
    rc = memcached_mget(cli->next_mc, &key, &len, 1);
  }
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    if (!cli->conf->quiet)
      print_log("mget failed. id=%d key=%s rc=%d(%s)",
        cli->id, key, rc, memcached_strerror(NULL, rc));
  }
  // Fetch the result from the server before returning it to the pool
  ok = false;
  (void)memcached_result_create(cli->next_mc, &result);
  result_ptr = memcached_fetch_result(cli->next_mc, &result, &error);
  if (result_ptr == NULL) {
    print_log("fetch_result returns null");
  }
  else if (result_ptr != &result) {
    print_log("fetch_result returns unexpected pointer. ptr=%p expected=%p",
      result_ptr, &result);
  }
  else if (error != MEMCACHED_SUCCESS) {
    print_log("fetched return code is not success. error=%d(%s)",
      error, memcached_strerror(NULL, error));
  }
  else {
    ok = true;
    cas = memcached_result_cas(result_ptr);
    //print_log("cas=%llu", (long long unsigned)cas);
  }
  memcached_result_free(&result);
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;

  if (!ok)
    return 0;
  
  // CAS.  Use cas+1 to see if it fails.
  if (0 != client_before_request(cli))
    return -1;
  val_ptr = NULL;
  val_len = valueset_get_value(cli->vs, &val_ptr);
  assert(val_ptr != NULL && val_len > 0);
  rc = memcached_cas(cli->next_mc, key, keylen, (const char*)val_ptr,
    (size_t)val_len, 100 /* exptime */, 0 /* flags */, cas+1);
  valueset_return_value(cli->vs, val_ptr);
  ok = (rc != MEMCACHED_SUCCESS);
  if (!ok) {
    if (!cli->conf->quiet) {
      print_log("CAS returns an unexpected SUCCESS. id=%d key=%s",
        cli->id, key);
    }
  }
  else
    rc = MEMCACHED_SUCCESS;
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;
  
  // CAS.  Use cas+0.  This time, CAS should succeed.
  if (0 != client_before_request(cli))
    return -1;
  val_ptr = NULL;
  val_len = valueset_get_value(cli->vs, &val_ptr);
  assert(val_ptr != NULL && val_len > 0);
  rc = memcached_cas(cli->next_mc, key, keylen, (const char*)val_ptr,
    (size_t)val_len, 100 /* exptime */, 0 /* flags */, cas);
  valueset_return_value(cli->vs, val_ptr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok && !cli->conf->quiet) {
    print_log("CAS returns an unexpected error. id=%d key=%s cas=%llu"
      " rc=%d(%s)", cli->id, key, (long long unsigned)cas,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;
  
  return 0;
}

static int
do_test(struct client *cli)
{
  if (0 != do_simple_test(cli))
    return -1; // Stop the test
  
  return 0; // Do another test
}

static struct client_profile default_profile = {
  .do_test = do_test,
};

struct client_profile *
torture_simple_cas_init(void)
{
  return &default_profile;
}
