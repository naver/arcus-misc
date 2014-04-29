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
    (size_t)val_len, 0 /* exptime */, 0 /* flags */);

  valueset_return_value(cli->vs, val_ptr);

  ok = (rc == MEMCACHED_SUCCESS);
  if (!ok && !cli->conf->quiet) {
    print_log("set failed. id=%d key=%s rc=%d(%s)", cli->id, key,
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
torture_simple_zero_exptime_init(void)
{
  return &default_profile;
}
