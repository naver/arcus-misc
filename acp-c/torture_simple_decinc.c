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
  memcached_return rc;
  int ok, keylen, base;
  const char *key;
  uint8_t *val_ptr;
  int val_len;
  uint64_t new_val;
  char val_buf[64];

  // Pick a key
  key = keyset_get_key(cli->ks, &base);
  keylen = strlen(key);

  // Insert an item
  if (0 != client_before_request(cli))
    return -1;
  // Base-10 digits
  val_len = sprintf(val_buf, "%d", cli->id+1); // +1 to avoid 0
  val_ptr = (uint8_t*)val_buf;
  rc = memcached_set(cli->next_mc, key, keylen, (const char*)val_ptr,
    (size_t)val_len, 1000 /* exptime */, 0 /* flags */);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok && !cli->conf->quiet) {
    print_log("set failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;
  
  // Decrement to 0
  if (0 != client_before_request(cli))
    return -1;
  rc = memcached_decrement(cli->next_mc, key, keylen,
    cli->id+1, &new_val);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    if (!cli->conf->quiet)
      print_log("decrement failed. id=%d key=%src=%d(%s)",
        cli->id, key, rc, memcached_strerror(NULL, rc));
  }
  else if (new_val != 0) {
    print_log("Unexpected value from decrement. result=%llu",
      (long long unsigned)new_val);
    ok = 0;
  }
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;
  
  // Increment to cli.id+2
  if (0 != client_before_request(cli))
    return -1;
  rc = memcached_increment(cli->next_mc, key, keylen,
    cli->id+2, &new_val);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok) {
    if (!cli->conf->quiet)
      print_log("increment failed. id=%d key=%src=%d(%s)",
        cli->id, key, rc, memcached_strerror(NULL, rc));
  }
  else if (new_val != cli->id+2) {
    print_log("Unexpected value from increment. result=%llu expected=%d",
      (long long unsigned)new_val, cli->id+2);
    ok = 0;
  }
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;

#if 0
  // Replace with a pattern
  if (0 != client_before_request(cli))
    return -1;
  val_ptr = NULL;
  val_len = valueset_get_value(cli->vs, &val_ptr);
  assert(val_ptr != NULL && val_len > 0);
  rc = memcached_replace(cli->next_mc, key, keylen, (const char*)val_ptr,
    (size_t)val_len, 1000 /* exptime */, 0 /* flags */);
  valueset_return_value(cli->vs, val_ptr);
  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok && !cli->conf->quiet) {
    print_log("replace failed. id=%d key=%s rc=%d(%s)", cli->id, key,
      rc, memcached_strerror(NULL, rc));
  }
  if (0 != client_after_request_with_rc(cli, rc))
    return -1;
#endif
  
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
torture_simple_decinc_init(void)
{
  return &default_profile;
}
