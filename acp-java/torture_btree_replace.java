/* -*- Mode: Java; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
 * acp-java : Arcus Java Client Performance benchmark program
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
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import net.spy.memcached.collection.CollectionAttributes;
import net.spy.memcached.collection.CollectionOverflowAction;
import net.spy.memcached.collection.CollectionResponse;
import net.spy.memcached.collection.ElementValueType;
import net.spy.memcached.internal.CollectionFuture;

public class torture_btree_replace implements client_profile {
  public boolean do_test(client cli) {
    try {
      if (!do_btree_test(cli))
        return false;
    } catch (Exception e) {
      System.out.printf("client_profile exception. id=%d exception=%s\n", 
                        cli.id, e.toString());
      e.printStackTrace();
    }
    return true;
  }
  
  public boolean do_btree_test(client cli) throws Exception {
    // Pick a key
    String key = cli.ks.get_key();

    String[] temp = key.split("-");
    long base = Long.parseLong(temp[1]);
    base = base * 64*1024;

    // Create a btree item
    if (!cli.before_request())
      return false;
    ElementValueType vtype = ElementValueType.BYTEARRAY;
    CollectionAttributes attr = 
      new CollectionAttributes(cli.conf.client_exptime,
                               new Long(4000),
                               CollectionOverflowAction.smallest_trim);
    CollectionFuture<Boolean> fb = cli.next_ac.asyncBopCreate(key, vtype, attr);
    boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
    if (!ok) {
      System.out.printf("bop create failed. id=%d key=%s: %s\n", cli.id,
                        key, fb.getOperationStatus().getResponse());
    }
    if (!cli.after_request(ok))
      return false;

    // Insert elements
    for (long bkey = base; bkey < base + 4000; bkey++) {
      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      assert(val.length <= 4096);
      fb = cli.next_ac.asyncBopInsert(key, bkey, null /* eflag */,
                                      val,
                                      null /* Do not auto-create item */);
      ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("bop insert failed. id=%d key=%s bkey=%d: %s\n",
                          cli.id, key, bkey,
                          fb.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }

    // Update elements
    for (long bkey = base; bkey < base + 4000; bkey++) {
      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      assert(val.length <= 4096);
      fb = cli.next_ac.asyncBopUpdate(key, bkey, null /* eflag */, val);
      ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        CollectionResponse res = fb.getOperationStatus().getResponse();
        System.out.printf("bop update failed. id=%d key=%s bkey=%d res=%s\n", 
                          cli.id, key, bkey, res.toString());
      }
      if (!cli.after_request(ok))
        return false;
    }

    // Upsert elements
    for (long bkey = base; bkey < base + 4000; bkey++) {
      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      assert(val.length <= 4096);
      fb = cli.next_ac.asyncBopUpsert(key, bkey, null /* eflag */, val, null);
      ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        CollectionResponse res = fb.getOperationStatus().getResponse();
        System.out.printf("bop upsert failed. id=%d key=%s bkey=%d res=%s\n",
                          cli.id, key, bkey, res.toString());
      }
      if (!cli.after_request(ok))
        return false;
    }

    if (true)
      return true;

    // Incr elements
    for (long bkey = base; bkey < base + 4000; bkey++) {
      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      assert(val.length <= 4096);
      CollectionFuture<Long> fl = 
        cli.next_ac.asyncBopIncr(key, bkey, (int)bkey);
      Long lv = fl.get(1000L, TimeUnit.MILLISECONDS);
      //if (!ok) {
      //  System.out.printf("bop incr failed. id=%d key=%s bkey=%d\n", cli.id,
      //                    key, bkey);
      //}
      if (!cli.after_request(true))
        return false;
    }

    // Decr elements
    for (long bkey = base; bkey < base + 4000; bkey++) {
      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      assert(val.length <= 4096);
      CollectionFuture<Long> fl = 
        cli.next_ac.asyncBopDecr(key, bkey, (int)bkey);
      Long lv = fl.get(1000L, TimeUnit.MILLISECONDS);
      //if (!ok) {
      //  System.out.printf("bop decr failed. id=%d key=%s bkey=%d\n", cli.id,
      //                    key, bkey);
      //}
      if (!cli.after_request(true))
        return false;
    }

    return true;
  }
}
