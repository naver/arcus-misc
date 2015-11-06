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
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Random;

import net.spy.memcached.collection.BTreeGetResult;
import net.spy.memcached.collection.ByteArrayBKey;
import net.spy.memcached.collection.CollectionAttributes;
import net.spy.memcached.collection.CollectionOverflowAction;
import net.spy.memcached.collection.CollectionResponse;
import net.spy.memcached.collection.Element;
import net.spy.memcached.collection.ElementFlagFilter;
import net.spy.memcached.collection.ElementFlagUpdate;
import net.spy.memcached.collection.ElementValueType;
import net.spy.memcached.collection.SMGetElement;
import net.spy.memcached.internal.CollectionFuture;
import net.spy.memcached.internal.CollectionGetBulkFuture;
import net.spy.memcached.internal.SMGetFuture;
import net.spy.memcached.ops.CollectionOperationStatus;

// Port of arcus1.6.2-integration.py

public class torture_arcus_integration implements client_profile {

  public torture_arcus_integration() {
    int next_val_idx = 0;
    chunk_values = new String[chunk_sizes.length+1];
    chunk_values[next_val_idx++] = "Not_a_slab_class";
    String lowercase = "abcdefghijlmnopqrstuvwxyz";
    for (int s : chunk_sizes) {
      int len = s*2/3;
      char[] raw = new char[len];
      for (int i = 0; i < len; i++) {
        raw[i] = lowercase.charAt(random.nextInt(lowercase.length()));
      }
      chunk_values[next_val_idx++] = new String(raw);
    }

    //Logger.getLogger("net.spy.memcached").setLevel(Level.DEBUG);
 }

  int KeyLen = 20;
  int ExpireTime = 600;
  String DEFAULT_PREFIX = "arcustest-";
  char[] dummystring = 
    ("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
     "abcdefghijlmnopqrstuvwxyz").toCharArray();
  Random random = new Random(); // repeatable is okay
  int[] chunk_sizes = {
    96, 120, 152, 192, 240, 304, 384, 480, 600, 752, 944, 1184, 1480, 1856,
    2320, 2904, 3632, 4544, 5680, 7104, 8880, 11104, 13880, 17352, 21696,
    27120, 33904, 42384, 52984, 66232, 82792, 103496, 129376, 161720, 202152,
    252696, 315872, 394840, 493552, 1048576
  };
  String[] chunk_values;

  String generateData(int length) {
    String ret = "";
    for (int loop = 0; loop < length; loop++) {
      int randomInt = random.nextInt(60);
      char tempchar = dummystring[randomInt];
      ret = ret + tempchar;
    }
    return ret;
  }

  // Generates a key with given name and postfix
  String gen_key(String name) {
    if (name == null)
      name = "unknown";
    String prefix = DEFAULT_PREFIX;
    String key = generateData(KeyLen);
    return prefix + name + ":" + key;
  }
  
  // Generates a string workload with specific size.
  String gen_workload(boolean is_collection) {
    if (is_collection) {
      // random.choice(chunk_values[0:17]);
      // Why 0 index?  chunk_values[0] is "Not_a_slab_class"?
      return chunk_values[random.nextInt(17+1)];
    }
    else {
      return chunk_values[random.nextInt(chunk_values.length)];
    }
  }

  public boolean do_test(client cli) {
    try {
      if (!do_KeyValue(cli))
        return false;
      
      if (!do_Collection_Btree(cli))
        return false;
    
      if (!do_Collection_Set(cli))
        return false;
      
      if (!do_Collection_List(cli))
        return false;
    } catch (Exception e) {
      System.out.printf("client_profile exception. id=%d exception=%s\n", 
                        cli.id, e.toString());
      e.printStackTrace();
    }
    return true;
  }

  // get:set:delete:incr:decr = 3:1:0.01:0.1:0.0001
  public boolean do_KeyValue(client cli) throws Exception {
    String key = gen_key("KeyValue");
    String workloads = chunk_values[24];
    
    // Set 
    for (int i = 0; i < 1; i++) {
      if (!cli.before_request())
        return false;
      Future<Boolean> fb = cli.next_ac.set(key, ExpireTime, workloads);
      boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("KeyValue: set failed. id=%d key=%s\n", cli.id, key);
      }
      if (!cli.after_request(ok))
        return false;
    }

    // Get
    for (int i = 0; i < 5; i++) {
      if (!cli.before_request())
        return false;
      Future<Object> fs = cli.next_ac.asyncGet(key);
      String s = (String)fs.get(1000L, TimeUnit.MILLISECONDS);
      boolean ok = true;
      if (s == null) {
        ok = false;
        System.out.printf("KeyValue: Get failed. id=%d key=%s\n", cli.id, key);
      }
      if (!cli.after_request(ok))
        return false;
    }

    // Delete
    if (random.nextInt(3) == 0) {
      if (!cli.before_request())
        return false;
      Future<Boolean> f = cli.next_ac.delete(key);
      boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("KeyValue: delete failed. id=%d key=%s\n", 
                          cli.id, key);
      }
      if (!cli.after_request(ok))
        return false;
    }

    // Incr
    if (random.nextInt(1) == 0) {
      if (!cli.before_request())
        return false;
      Future<Boolean> fb = cli.next_ac.set(key + "numeric", ExpireTime, "1");
      boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("KeyValue: set numeric failed. id=%d key=%s\n",
                          cli.id, key);
      }
      if (!cli.after_request(ok))
        return false;
      if (!cli.before_request())
        return false;
      Future<Long> fl = cli.next_ac.asyncIncr(key + "numeric", 1);
      Long lv = fl.get(1000L, TimeUnit.MILLISECONDS);
      // The returned value is the result of increment.
      ok = true;
      if (lv.longValue() != 2) {
        ok = false;
        System.out.println("KeyValue: Unexpected value from increment." +
                           " result=" +lv.longValue() +
                           " expected=" + 2);
      }
      if (!cli.after_request(ok))
        return false;
    }
    
    // Decr
    if (random.nextInt(1) == 0) {
      if (!cli.before_request())
        return false;
      Future<Boolean> fb = cli.next_ac.set(key + "numeric", ExpireTime, "1");
      boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("KeyValue: set numeric failed. id=%d key=%s\n",
                          cli.id, key);
      }
      if (!cli.after_request(ok))
        return false;
      if (!cli.before_request())
        return false;
      Future<Long> fl = cli.next_ac.asyncDecr(key + "numeric", 1);
      Long lv = fl.get(1000L, TimeUnit.MILLISECONDS);
      // The returned value is the result of decrement.
      ok = true;
      if (lv.longValue() != 0) {
        ok = false;
        System.out.println("KeyValue: Unexpected value from decrement." +
                           " result=" + lv.longValue());
      }
      if (!cli.after_request(ok))
        return false;
    }

    return true;
  }
  
  public boolean do_Collection_Btree(client cli) throws Exception {
    String key = gen_key("Collection_Btree");
    List<String> key_list = new LinkedList<String>();
    for (int i = 0; i < 4; i++)
      key_list.add(key + i);
    
    String bkeyBASE = "bkey_byteArry";

    byte[] eflag = ("EFLAG").getBytes();
    ElementFlagFilter filter = 
      new ElementFlagFilter(ElementFlagFilter.CompOperands.Equal,
                            ("EFLAG").getBytes());
    CollectionAttributes attr = new CollectionAttributes();
    attr.setExpireTime(ExpireTime);

    String[] workloads = { chunk_values[1], 
                           chunk_values[1], 
                           chunk_values[2], 
                           chunk_values[2], 
                           chunk_values[3] };

    // BopInsert + byte_array bkey
    for (int j = 0; j < 4; j++) {
      // Insert 50 bkey
      for (int i = 0; i < 50; i++) {
        if (!cli.before_request())
          return false;
        // Uniq bkey
        String bk = bkeyBASE + Integer.toString(j) + Integer.toString(i);
        byte[] bkey = bk.getBytes();
        CollectionFuture<Boolean> f = cli.next_ac.
          asyncBopInsert(key_list.get(j), bkey, eflag, 
                         workloads[random.nextInt(workloads.length)], attr);
        boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
        if (!ok) {
          System.out.printf("Collection_Btree: BopInsert failed." +
                            " id=%d key=%s bkey=%s: %s\n", cli.id,
                            key_list.get(j), bk,
                            f.getOperationStatus().getResponse());
        }
        if (!cli.after_request(ok))
          return false;
      }
    }
    
    // Bop Bulk Insert (Piped Insert)
    {
      List<Element<Object>> elements = new LinkedList<Element<Object>>();
      for (int i = 0; i < 50; i++) {
        String bk = bkeyBASE + "0" + Integer.toString(i) + "bulk";
        elements.add(new Element<Object>(bk.getBytes(), workloads[0], eflag));
      }
      if (!cli.before_request())
        return false;
      CollectionFuture<Map<Integer, CollectionOperationStatus>> f =
        cli.next_ac.asyncBopPipedInsertBulk(key_list.get(0), elements,
                                            new CollectionAttributes());
      Map<Integer, CollectionOperationStatus> status_map = 
        f.get(1000L, TimeUnit.MILLISECONDS);
      Iterator<CollectionOperationStatus> status_iter = 
        status_map.values().iterator();
      while (status_iter.hasNext()) {
        CollectionOperationStatus status = status_iter.next();
        CollectionResponse resp = status.getResponse();
        if (resp != CollectionResponse.STORED) {
          System.out.printf("Collection_Btree: BopPipedInsertBulk failed." +
                            " id=%d key=%s response=%s\n", cli.id,
                            key_list.get(0), resp);
        }
      }
      if (!cli.after_request(true))
        return false;
    }

    // BopGet Range + filter
    for (int j = 0; j < 4; j++) {
      if (!cli.before_request())
        return false;
      String bk = bkeyBASE + Integer.toString(j) + Integer.toString(0);
      String bk_to = bkeyBASE + Integer.toString(j) + Integer.toString(50);
      byte[] bkey = bk.getBytes();
      byte[] bkey_to = bk_to.getBytes();
      CollectionFuture<Map<ByteArrayBKey, Element<Object>>> f =
        cli.next_ac.asyncBopGet(key_list.get(j), bkey, bkey_to, filter,
                                0, random.nextInt(30) + 20,
                                /* random.randint(20, 50) */
                                false, false);
      Map<ByteArrayBKey, Element<Object>> val = 
        f.get(1000L, TimeUnit.MILLISECONDS);
      if (val == null || val.size() <= 0) {
        System.out.printf("Collection_Btree: BopGet failed." +
                          " id=%d key=%s val.size=%d\n", cli.id,
                          key_list.get(j), val == null ? -1 : 0);
      }
      if (!cli.after_request(true))
        return false;
    }

    // BopGetBulk  // 20120319 Ad
    {
      if (!cli.before_request())
        return false;
      String bk = bkeyBASE + "0" + "0";
      String bk_to = bkeyBASE + "4" + "50";
      byte[] bkey = bk.getBytes();
      byte[] bkey_to = bk_to.getBytes();
      CollectionGetBulkFuture
        <Map<String, BTreeGetResult<ByteArrayBKey, Object>>> f =
        cli.next_ac.asyncBopGetBulk(key_list, bkey, bkey_to, filter, 0,
                                    random.nextInt(30) + 20
                                    /* random.randint(20, 50) */);
      Map<String, BTreeGetResult<ByteArrayBKey, Object>> val = 
        f.get(1000L, TimeUnit.MILLISECONDS);
      if (val == null || val.size() <= 0) {
        System.out.printf("Collection_Btree: BopGetBulk failed." +
                          " id=%d val.size=%d\n", cli.id,
                          val == null ? -1 : 0);
      }
      else {
        // Should we check individual elements?  FIXME
      }
      if (!cli.after_request(true))
        return false;
    }
    
    // BopEmpty Create
    {
      if (!cli.before_request())
        return false;
      CollectionFuture<Boolean> f = 
        cli.next_ac.asyncBopCreate(key, ElementValueType.STRING, 
                                   new CollectionAttributes());
      boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("Collection_Btree: BopCreate failed." +
                          " id=%d key=%s: %s\n", cli.id, key,
                          f.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }

    // BopSMGet
    {
      if (!cli.before_request())
        return false;
      String bk = bkeyBASE + "0" + "0";
      String bk_to = bkeyBASE + "4" + "50";
      byte[] bkey = bk.getBytes();
      byte[] bkey_to = bk_to.getBytes();
      SMGetFuture<List<SMGetElement<Object>>> f =
        cli.next_ac.asyncBopSortMergeGet(key_list, bkey, bkey_to, 
                                         filter, 0, 
                                         random.nextInt(30) + 20
                                         /* random.randint(20, 50) */);
      List<SMGetElement<Object>> val = f.get(1000L, TimeUnit.MILLISECONDS);
      if (val == null || val.size() <= 0) {
        System.out.printf("Collection_Btree: BopSortMergeGet failed." +
                          " id=%d val.size=%d\n", cli.id,
                          val == null ? -1 : 0);
      }
      if (!cli.after_request(true))
        return false;
    }
    
    // BopUpdate  (eflag bitOP + value)
    {
      String key0 = key_list.get(0);
      int eflagOffset = 0;
      String value = "ThisIsChangeValue";
      ElementFlagUpdate bitop = 
        new ElementFlagUpdate(eflagOffset, 
                              ElementFlagFilter.BitWiseOperands.AND,
                              ("aflag").getBytes());
      for (int i = 0; i < 2; i++) {
        if (!cli.before_request())
          return false;
        String bk = bkeyBASE + "0" + Integer.toString(i);
        byte[] bkey = bk.getBytes();
        CollectionFuture<Boolean> f = 
          cli.next_ac.asyncBopUpdate(key0, bkey, bitop, value);
        boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
        if (!ok) {
          System.out.printf("Collection_Btree: BopUpdate failed." +
                            " id=%d key=%s: %s\n", cli.id, key0,
                            f.getOperationStatus().getResponse());
        }
        if (!cli.after_request(ok))
          return false;
      }
    }
    
    // SetAttr  (change Expire Time)
    {
      if (!cli.before_request())
        return false;
      attr.setExpireTime(100);
      CollectionFuture<Boolean> f = cli.next_ac.asyncSetAttr(key, attr);
      boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("Collection_Btree: SetAttr failed." +
                          " id=%d key=%s: %s\n", cli.id, key,
                          f.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }
    
    // BopDelete          (eflag filter delete)
    {
      for (int j = 0; j < 4; j++) {
        if (!cli.before_request())
          return false;
        String bk = bkeyBASE + Integer.toString(j) + "0";
        String bk_to = bkeyBASE + Integer.toString(j) + "10";
        byte[] bkey = bk.getBytes();
        byte[] bkey_to = bk_to.getBytes();
        CollectionFuture<Boolean> f = 
          cli.next_ac.asyncBopDelete(key_list.get(j), bkey, bkey_to, filter,
                                     0, false);
        boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
        if (!ok) {
          System.out.printf("Collection_Btree: BopDelete failed." +
                            " id=%d key=%s: %s\n", cli.id, key_list.get(j),
                            f.getOperationStatus().getResponse());
        }
        if (!cli.after_request(ok))
          return false;
      }
    }

    return true;
  }

  public boolean do_Collection_Set(client cli) throws Exception {
    String key = gen_key("Collection_Set");
    List<String> key_list = new LinkedList<String>();
    for (int i = 0; i < 4; i++)
      key_list.add(key + i);

    CollectionAttributes attr = new CollectionAttributes();
    attr.setExpireTime(ExpireTime);

    String[] workloads = { chunk_values[1], 
                           chunk_values[1], 
                           chunk_values[2], 
                           chunk_values[2], 
                           chunk_values[3] };

    // SopInsert
    {
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 19; j++) {
          if (!cli.before_request())
            return false;
          String set_value = workloads[i] + Integer.toString(j);
          CollectionFuture<Boolean> f = 
            cli.next_ac.asyncSopInsert(key_list.get(i), set_value, attr);
          boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
          if (!ok) {
            System.out.printf("Collection_Set: SopInsert failed." +
                              " id=%d key=%s: %s\n", cli.id, key_list.get(i),
                              f.getOperationStatus().getResponse());
          }
          if (!cli.after_request(ok))
            return false;
        }
      }
    }
    
    // SopInsert Bulk (Piped)
    {
      List<Object> elements = new LinkedList<Object>();
      for (int i = 0; i < 50; i++) {
        elements.add((Integer.toString(i) + "_" + workloads[0]));
      }
      if (!cli.before_request())
        return false;
      CollectionFuture<Map<Integer, CollectionOperationStatus>> f =
        cli.next_ac.asyncSopPipedInsertBulk(key_list.get(0), elements, 
                                            new CollectionAttributes());
      Map<Integer, CollectionOperationStatus> status_map = 
        f.get(1000L, TimeUnit.MILLISECONDS);
      Iterator<CollectionOperationStatus> status_iter = 
        status_map.values().iterator();
      while (status_iter.hasNext()) {
        CollectionOperationStatus status = status_iter.next();
        CollectionResponse resp = status.getResponse();
        if (resp != CollectionResponse.STORED) {
          System.out.printf("Collection_Set: SopPipedInsertBulk failed." +
                            " id=%d key=%s response=%s\n", cli.id,
                            key_list.get(0), resp);
        }
      }
      if (!cli.after_request(true))
        return false;
    }
    
    // SopEmpty Create
    {
      if (!cli.before_request())
        return false;
      CollectionFuture<Boolean> f = 
        cli.next_ac.asyncSopCreate(key, ElementValueType.STRING, 
                                   new CollectionAttributes());
      boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("Collection_Set: SopCreate failed." +
                          " id=%d key=%s: %s\n", cli.id, key,
                          f.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }
    
    // SopExist    (Piped exist)
    {
      for (int i = 0; i < 4; i++) {
        List<Object> list_value = new LinkedList<Object>();
        for (int j = 0; j < 9; j++) {
          if (!cli.before_request())
            return false;
          list_value.add(workloads[i] + Integer.toString(j));
          CollectionFuture<Map<Object, Boolean>> f = 
            cli.next_ac.asyncSopPipedExistBulk(key_list.get(i), list_value);
          Map<Object, Boolean> result_map =
            f.get(1000L, TimeUnit.MILLISECONDS);
          if (result_map == null || result_map.size() != list_value.size()) {
            System.out.printf("Collection_Set: SopPipedExistBulk failed." +
                              " id=%d key=%s result_map.size=%d" +
                              " list_value.size=%d\n",
                              cli.id, key_list.get(i), 
                              result_map == null ? -1 : result_map.size(),
                              list_value.size());
          }
          if (!cli.after_request(true))
            return false;
        }
      }
    }
    
    // SetAttr  (change Expire Time)
    {
      if (!cli.before_request())
        return false;
      attr.setExpireTime(100);
      CollectionFuture<Boolean> f = cli.next_ac.asyncSetAttr(key, attr);
      boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("Collection_Set: SetAttr failed." +
                          " id=%d key=%s: %s\n", cli.id, key,
                          f.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }
    
    // SopDelete
    {
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
          if (!cli.before_request())
            return false;
          String del_value = workloads[i] + Integer.toString(j);
          CollectionFuture<Boolean> f = 
            cli.next_ac.asyncSopDelete(key_list.get(i), del_value, true);
          boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
          if (!ok) {
            System.out.printf("Collection_Set: SopDelete failed." +
                              " id=%d key=%s: %s\n", cli.id, key_list.get(i),
                              f.getOperationStatus().getResponse());
          }
          if (!cli.after_request(ok))
            return false;
        }
      }
    }

    return true;
  }

  public boolean do_Collection_List(client cli) throws Exception {
    String key = gen_key("Collection_List");
    List<String> key_list = new LinkedList<String>();
    for (int i = 0; i < 4; i++)
      key_list.add(key + i);

    CollectionAttributes attr = new CollectionAttributes();
    attr.setExpireTime(ExpireTime);

    String[] workloads = { chunk_values[1], 
                           chunk_values[1], 
                           chunk_values[2], 
                           chunk_values[2], 
                           chunk_values[3] };

    // LopInsert
    {
      int index = -1; // tail insert
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 50; j++) {
          if (!cli.before_request())
            return false;
          CollectionFuture<Boolean> f = cli.next_ac
            .asyncLopInsert(key_list.get(i), index, 
                            workloads[random.nextInt(workloads.length)], attr);
          boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
          if (!ok) {
            System.out.printf("Collection_List: LopInsert failed." +
                              " id=%d key=%s: %s\n", cli.id, key_list.get(i),
                              f.getOperationStatus().getResponse());
          }
          if (!cli.after_request(ok))
            return false;
        }
      }
    }

    // LopInsert Bulk (Piped)
    {
      List<Object> elements = new LinkedList<Object>();
      for (int i = 0; i < 50; i++) {
        elements.add(Integer.toString(i) + "_" + workloads[0]);
      }
      if (!cli.before_request())
        return false;
      CollectionFuture<Map<Integer, CollectionOperationStatus>> f =
        cli.next_ac.asyncLopPipedInsertBulk(key_list.get(0), -1, elements, 
                                            new CollectionAttributes());
      Map<Integer, CollectionOperationStatus> status_map = 
        f.get(1000L, TimeUnit.MILLISECONDS);
      Iterator<CollectionOperationStatus> status_iter = 
        status_map.values().iterator();
      while (status_iter.hasNext()) {
        CollectionOperationStatus status = status_iter.next();
        CollectionResponse resp = status.getResponse();
        if (resp != CollectionResponse.STORED) {
          System.out.printf("Collection_List: LopPipedInsertBulk failed." +
                            " id=%d key=%s response=%s\n", cli.id,
                            key_list.get(0), resp);
        }
      }
      if (!cli.after_request(true))
        return false;
    }

    // LopGet
    {
      for (int i = 0; i < 4; i++) {
        if (!cli.before_request())
          return false;
        int index = 0;
        int index_to = index + 
          /* random.randint(20, 50) */ random.nextInt(30) + 20;
        CollectionFuture<List<Object>> f =
          cli.next_ac.asyncLopGet(key_list.get(i), index, index_to, 
                                  false, false);
        List<Object> val = f.get(1000L, TimeUnit.MILLISECONDS);
        if (val == null || val.size() <= 0) {
          System.out.printf("Collection_List: LopGet failed." +
                            " id=%d key=%s val.size=%d\n",
                            cli.id, key_list.get(i), 
                            val == null ? -1 : val.size());
        }
        if (!cli.after_request(true))
          return false;
      }
    }

    // LopAttr
    {
      if (!cli.before_request())
        return false;
      attr.setExpireTime(100);
      CollectionFuture<Boolean> f =
        cli.next_ac.asyncSetAttr(key_list.get(0), attr);
      boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
      if (!ok) {
        System.out.printf("Collection_List: SetAttr failed." +
                          " id=%d key=%s: %s\n", cli.id, key_list.get(0),
                          f.getOperationStatus().getResponse());
      }
      if (!cli.after_request(ok))
        return false;
    }

    // LopDelete
    {
      int index = 0;
      int index_to = 19;
      for (int i = 0; i < 1; i++) {
        if (!cli.before_request())
          return false;
        CollectionFuture<Boolean> f = 
          cli.next_ac.asyncLopDelete(key_list.get(i), index, index_to, true);
        boolean ok = f.get(1000L, TimeUnit.MILLISECONDS);
        if (!ok) {
          System.out.printf("Collection_List: LopDelete failed." +
                            " id=%d key=%s: %s\n", cli.id, key_list.get(i),
                            f.getOperationStatus().getResponse());
        }
        if (!cli.after_request(ok))
          return false;
      }
    }

    return true;
  }
}
