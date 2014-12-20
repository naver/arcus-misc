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
import net.spy.memcached.collection.CollectionType;
import net.spy.memcached.collection.CollectionOverflowAction;
import net.spy.memcached.collection.ElementValueType;
import net.spy.memcached.internal.CollectionFuture;

// Should make subclasses of client instead of using the interface?
// FIXME

public class standard_mix implements client_profile {
  public boolean do_test(client cli) {
    try {
      if (!do_btree_test(cli))
        return false;
      
      if (!do_set_test(cli))
        return false;
    
      if (!do_list_test(cli))
        return false;
      
      if (!do_simple_test(cli))
        return false;
    } catch (Exception e) {
      System.out.printf("client_profile exception. id=%d exception=%s\n", 
                        cli.id, e.toString());
      e.printStackTrace();
    }
    return true;
  }
  
  private boolean do_delete_if_exist(client cli, String key, CollectionType type)
      throws Exception {
    // Get attributes
    if (!cli.before_request())
      return false;
    CollectionFuture<CollectionAttributes> f = cli.next_ac.asyncGetAttr(key);
    CollectionAttributes attr = f.get(1000L, TimeUnit.MILLISECONDS);
    if (!cli.after_request(true))
      return false;

    if (attr != null && (type != CollectionType.kv || type != attr.getType())) {
      if (!cli.before_request())
        return false;
      Future<Boolean> fb = cli.next_ac.delete(key);
      boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("KeyValue: delete failed. id=%d key=%s\n",
                          cli.id, key);
      }
      if (!cli.after_request(ok))
        return false;
    }
    return true; 
  }
 
  public boolean do_btree_test(client cli) throws Exception {
    // Pick a key
    String key = cli.ks.get_key();

    if (!do_delete_if_exist(cli, key, CollectionType.btree))
      return false;

    // Create a btree item
    if (!cli.before_request())
      return false;
    ElementValueType vtype = ElementValueType.BYTEARRAY;
    CollectionAttributes attr = 
      new CollectionAttributes(cli.conf.client_exptime,
                               CollectionAttributes.DEFAULT_MAXCOUNT,
                               CollectionOverflowAction.smallest_trim);
    CollectionFuture<Boolean> fb = cli.next_ac.asyncBopCreate(key, vtype, attr);
    boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
    if (!ok) {
      System.out.printf("bop create failed. id=%d key=%s: %s\n", cli.id,
                        key, fb.getOperationStatus().getResponse());
    }
    if (!cli.after_request(ok))
      return false;

    // Insert a number of btree element
    cli.bks.reset();
    for (int i = 0; i < cli.bks.get_size(); i++) {
      if (!cli.before_request())
        return false;
      long bkey = cli.bks.get_bkey();
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

    // Update
    // Get/delete
    // Upsert
    // Incr/decr
    // Expire time
    // Smget
    // Getattr
    // PipedInsert
    return true;
  }

  public boolean do_set_test(client cli) throws Exception {
    // Pick a key
    String key = cli.ks.get_key();
    
    if (!do_delete_if_exist(cli, key, CollectionType.set))
      return false;

    // Create a set item
    if (!cli.before_request())
      return false;
    ElementValueType vtype = ElementValueType.BYTEARRAY;
    CollectionAttributes attr = 
      new CollectionAttributes(cli.conf.client_exptime,
                               CollectionAttributes.DEFAULT_MAXCOUNT,
                               CollectionOverflowAction.error);
    // For set items, OverflowAction is always "error".

    CollectionFuture<Boolean> fb = cli.next_ac.asyncSopCreate(key, vtype, attr);
    boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
    if (!ok) {
      System.out.printf("sop create failed. id=%d key=%s: %s\n", cli.id,
                        key, fb.getOperationStatus().getResponse());
    }
    if (!cli.after_request(ok))
      return false;
    
    // Insert a number of elements.  Set has no element keys.
    for (int i = 0; i < 100; i++) {
      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      assert(val.length <= 4096);
      fb = cli.next_ac.asyncSopInsert(key, val,
                                      null /* Do not auto-create item */);
      ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("sop insert failed. id=%d key=%s: %s\n", cli.id,
                          key, fb.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }

    // Get/delete
    // Delete
    // PipedInsertBulk
    // Expire time
    // Getattr
    return true;
  }

  public boolean do_list_test(client cli) throws Exception {
    // Pick a key
    String key = cli.ks.get_key();
    
    if (!do_delete_if_exist(cli, key, CollectionType.list))
      return false;

    // Create a list item
    if (!cli.before_request())
      return false;
    ElementValueType vtype = ElementValueType.BYTEARRAY;
    CollectionAttributes attr = 
      new CollectionAttributes(cli.conf.client_exptime,
                               CollectionAttributes.DEFAULT_MAXCOUNT,
                               CollectionOverflowAction.tail_trim);
    // OverflowAction should be error, head_trim, or tail_trim
    CollectionFuture<Boolean> fb = cli.next_ac.asyncLopCreate(key, vtype, attr);
    boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
    if (!ok) {
      System.out.printf("lop create failed. id=%d key=%s: %s\n", cli.id,
                        key, fb.getOperationStatus().getResponse());
    }
    if (!cli.after_request(ok))
      return false;

    // Insert a number of elements.  Push at the head.
    for (int i = 0; i < 100; i++) {
      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      assert(val.length <= 4096);
      fb = cli.next_ac.asyncLopInsert(key, 0 /* head */, val,
                                  null /* Do not auto-create item */);
      ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("lop insert failed. id=%d key=%s: %s\n", cli.id,
                          key, fb.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }

    // Get/delete
    // Delete
    // PipedInsert
    // Expire time
    // Getattr
    return true;
  }

  public boolean do_simple_test(client cli) throws Exception {

    // Set a number of items
    for (int i = 0; i < 100; i++) {
      // Pick a key
      String key = cli.ks.get_key();
      if (!do_delete_if_exist(cli, key, CollectionType.btree))
        return false;

      if (!cli.before_request())
        return false;
      byte[] val = cli.vset.get_value();
      Future<Boolean> fb = 
        cli.next_ac.set(key, cli.conf.client_exptime, val, raw_transcoder.raw_tc);
      boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("set failed. id=%d key=%s\n", cli.id, key);
      }
      if (!cli.after_request(ok))
        return false;
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
    return true;
  }
}
