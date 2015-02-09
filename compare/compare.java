/* -*- Mode: Java; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
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
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.Vector;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.zip.Adler32;

import net.spy.memcached.ArcusClient;
import net.spy.memcached.CachedData;
import net.spy.memcached.CASValue;
import net.spy.memcached.DefaultConnectionFactory;
import net.spy.memcached.collection.ByteArrayBKey;
import net.spy.memcached.collection.CollectionAttributes;
import net.spy.memcached.collection.Element;
import net.spy.memcached.internal.CollectionFuture;
import net.spy.memcached.transcoders.Transcoder;

public class compare {

  static final long op_timeout = 4000L; // 4 seconds
  static final long expDiffLimit = 3; // (unit: seconds)

  // Receive raw bytes
  class mytc implements Transcoder<byte[]> {
    public boolean asyncDecode(CachedData d) {
      return false;
    }

    public CachedData encode(byte[] o) {
      // Not used
      return null;
    }

    public byte[] decode(CachedData d) {
      return d.getData();
    }

    public int getMaxSize() {
      return 1000000000;
    }
  }

  // Force the client to use exactly one server
  // 1.7 or 1.6 does not matter
  class myfactory extends DefaultConnectionFactory {
  }
  class myclient extends ArcusClient {
    String name;
    myclient(String name, List<InetSocketAddress> addrs) throws Exception {
      super(new myfactory(), addrs);
      this.name = name;
    }
  }

  List<String> args_keydump = new ArrayList<String>();
  List<String> args_server = new ArrayList<String>();
  String args_key = null;
  int args_verbose = 0;
  boolean args_quiet = false;
  boolean args_cas = false;

  class Key {
    String str;
    byte[] bytes;
    int type;
    static final int SIMPLE = 0;
    static final int BTREE  = 1;
    static final int LIST   = 2;
    static final int SET    = 3;
    int hash_val;

    Key(byte[] b, int t, int hv) {
      bytes = b;
      type = t;
      hash_val = hv;
      str = new String(bytes);

      hash_val = (int)JenkinsHash.hash64(bytes, 0);
      /*
      String[] temp = str.split("-");
      try {
        hash_val = (int)Long.parseLong(temp[1]);
      } catch (Exception e) {
        hash_val = 0;
      }
      */
    }

    public int hashCode() {
      //System.out.println("hash_val=" + hash_val);
      return hash_val;
    }

    public boolean equals(Object obj) {
      Key k = (Key)obj;
      if (k.bytes != null && bytes != null &&
          k.bytes.length == bytes.length) {
        for (int i = 0; i < bytes.length; i++)
          if (k.bytes[i] != bytes[i])
            return false;
        return true;
      }
      return false;
    }
  }

  HashMap<Key, Key> keymap = new HashMap<Key, Key>(10000000);
  List<myclient> server_list = new ArrayList<myclient>();
  mytc tc = new mytc();

  public compare() {
    // Nothing to do
  }

  void read_keydump(String path) throws Exception {
    System.out.println("Read key dump file=" + path);
    File file = new File(path);
    FileInputStream fin = new FileInputStream(file);
    BufferedInputStream bin = new BufferedInputStream(fin);
    byte[] buf = new byte[1024];
    boolean last = false;
    int count = 0;
    Adler32 sum = new Adler32();
    while (!last) {
      if (4 != bin.read(buf, 0, 4))
        throw new Exception("Cannot read key length");
      // Little endian
      int len = (((int)buf[0]) & 0xff) +
        ((((int)buf[1]) & 0xff) << 8) +
        ((((int)buf[2]) & 0xff) << 16) +
        ((((int)buf[3]) & 0xff) << 24);
      //hexdump(buf, 4);
      //System.out.println("key length=" + len);
      if (len == 0) {
        last = true;
      }
      else {
        if (1 != bin.read(buf, 0, 1))
          throw new Exception("Cannot read iflag.");
        int type;
        if (buf[0] == 0)
          type = Key.SIMPLE;
        else if (buf[0] == 2)
          type = Key.LIST;
        else if (buf[0] == 4)
          type = Key.SET;
        else if (buf[0] == 8)
          type = Key.BTREE;
        else
          throw new Exception("Unexpected iflag. byte=" + buf[0]);

        if (len != bin.read(buf, 0, len))
          throw new Exception("Cannot read key. length=" + len);
        byte[] key = new byte[len];
        for (int i = 0; i < len; i++)
          key[i] = buf[i];
        //sum.reset();
        //sum.update(key);
        //System.out.println("key=" + new String(key) + " hash=" + sum.getValue());
        Key k = new Key(key, type, /* (int)sum.getValue() */ 0);
        //System.out.println("key length=" + len + " key=" + printable_key(key));
        Key old = keymap.put(k, k);
        if (old != null && old.type != k.type) {
          System.out.println("Key type are different on different servers." +
                             " key=" + printable_key(key));
          System.out.println("type=" + old.type + " type=" + k.type);
        }
        count++;
      }
    }
    bin.close();
    System.out.println("Keys read=" + count);
    System.out.println("Key map size= " + keymap.size());
  }

  void prep() throws Exception {
    // Make a map of all keys
    for (String path : args_keydump) {
      read_keydump(path);
    }
    if (args_key != null) {
      //Adler32 sum = new Adler32();
      //sum.reset();
      //sum.update(args_key.getBytes());
      Key k = new Key(args_key.getBytes(), Key.BTREE,
                      /* (int)sum.getValue() */ 0);
      keymap.put(k, k);
    }
    //if (keymap.size() == 0)
    //  throw new Exception("There are no keys.");

    // Make one client per server
    if (args_server.size() < 2)
      throw new Exception("There are fewere than 2 servers.");
    for (String addr : args_server) {
      String[] temp = addr.split(":");
      InetSocketAddress inet =
        new InetSocketAddress(temp[0], Integer.parseInt(temp[1]));
      List<InetSocketAddress> inet_list = new ArrayList<InetSocketAddress>();
      inet_list.add(inet);
      myclient cli = new myclient(addr, inet_list);
      server_list.add(cli);
    }
    // just wait for memcached connection to do some preparation.
    Thread.sleep(100L);
    // print server list 
    System.out.println("Server list");
    for (myclient cli : server_list) {
      System.out.println(cli.name);
    }
  }

  void cleanup() throws Exception {
    for (myclient cli : server_list) {
      cli.shutdown();
    }
  }

  enum comp_result {
    EQUAL, EXIST_DIFF, ATTR_DIFF, VAL_DIFF, EFLAG_DIFF, VAL_NULL, FETCH_ERROR,
      MISSING_0, MISSING_1, MISSING_0_EXP, MISSING_1_EXP, CAS_DIFF,
  }

  comp_result compareKeyExist(CollectionAttributes a0, CollectionAttributes a1) {
    if (a0 == null || a1 == null) {
      if (a0 != null) {
        int exp0 = a0.getExpireTime();
        if (exp0 == -1 || exp0 == 0 || exp0 > expDiffLimit)
          return comp_result.EXIST_DIFF;
        else
          return comp_result.MISSING_0_EXP;
      }
      if (a1 != null) {
        int exp1 = a1.getExpireTime();
        if (exp1 == -1 || exp1 == 0 || exp1 > expDiffLimit)
          return comp_result.EXIST_DIFF;
        else
          return comp_result.MISSING_1_EXP;
      }
    }
    return comp_result.EQUAL;
  }

  comp_result compareKeyAttrs(CollectionAttributes a0, CollectionAttributes a1, Key key) {
    assert(a0 != null && a1 != null);
    if (a0.getType() != a1.getType()) {
      System.out.println("type is different");
      return comp_result.ATTR_DIFF;
    }
    if (!a0.getFlags().equals(a1.getFlags())) {
      System.out.println("flags is different");
      return comp_result.ATTR_DIFF;
    }
    if (true) {
      int exp0 = a0.getExpireTime();
      int exp1 = a1.getExpireTime();
      if (exp0 != exp1) {
        //  0 has a special meaning: no expiration.
        // -1 has a special meaning: no expiration & no eviction
        // So, both must be the same.
        if (exp0 == 0 || exp0 == -1 || exp1 == 0 || exp1 == -1) {
          System.out.println("exptime (0 or -1) is different");
          return comp_result.ATTR_DIFF;
        }
        // both have positive integers
        int expDiff = ((exp0 > exp1) ? (exp0-exp1) : (exp1-exp0));
        if (expDiff > expDiffLimit) {
          System.out.println("exptime is different");
          return comp_result.ATTR_DIFF;
        }
      }
    }

    if (key.type != Key.SIMPLE) {
      // compare common collection attributes
      Long l0 = a0.getCount();
      Long l1 = a1.getCount();
      if (l0 != null || l1 != null) {
        if (l0 == null || l1 == null || !l0.equals(l1)) {
          System.out.println("count is different");
          return comp_result.ATTR_DIFF;
        }
      }
      if (!a0.getMaxCount().equals(a1.getMaxCount())) {
        System.out.println("maxcount is different");
        return comp_result.ATTR_DIFF;
      }
      if (!a0.getOverflowAction().equals(a1.getOverflowAction())) {
        System.out.println("overflowaction is different");
        return comp_result.ATTR_DIFF;
      }
      if (!a0.getReadable().equals(a1.getReadable())) {
        System.out.println("readable is different");
        return comp_result.ATTR_DIFF;
      }
      // compare btree specific attributes
      if (key.type == Key.BTREE) {
        l0 = a0.getMaxBkeyRange();
        l1 = a1.getMaxBkeyRange();
        if (l0 != null || l1 != null) {
          if (l0 == null || l1 == null || !l0.equals(l1)) {
            System.out.println("maxbkeyrange is different");
            return comp_result.ATTR_DIFF;
          }
        }
        if (!Arrays.equals(a0.getMaxBkeyRangeByBytes(),
                           a1.getMaxBkeyRangeByBytes())) {
          System.out.println("maxbkeyrange (bytes) is different");
          return comp_result.ATTR_DIFF;
        }
      }
    }
    return comp_result.EQUAL;
  }

  comp_result compareItemAttributes(Key key, Vector<CollectionAttributes> attrs) {
    int server_count = server_list.size();
    comp_result totResult = comp_result.EQUAL;
    comp_result curResult;
    CollectionAttributes v0_attr;
    CollectionAttributes vi_attr;

    v0_attr = attrs.get(0);
    for (int i = 1; i < server_count; i++) {
      vi_attr = attrs.get(i);
      curResult = compareKeyExist(v0_attr, vi_attr);
      if (curResult != comp_result.EQUAL) {
        if (totResult != comp_result.EXIST_DIFF)
          totResult = curResult;
      }
    }
    if (totResult == comp_result.EXIST_DIFF) {
      System.out.println("Attr exists on some servers but not others." + " key=" + key.str);
      for (int i = 0; i < server_count; i++) {
        if (attrs.get(i) == null) {
          System.out.println(server_list.get(i).name + " not found");
        } else {
          System.out.println(server_list.get(i).name + " found (exptime=" +
                             attrs.get(i).getExpireTime() + ")");
        }
      }
      return totResult;
    }
    if (totResult != comp_result.EQUAL) {
      // comp_result.MISSING_0_EXP or comp_result.MISSING_1_EXP
      totResult = comp_result.EQUAL;
    } else {
      v0_attr = attrs.get(0);
      for (int i = 1; i < server_count; i++) {
        vi_attr = attrs.get(i);
        curResult = compareKeyAttrs(v0_attr, vi_attr, key);
        if (curResult != comp_result.EQUAL) {
          System.out.println("Attributes are different." +
                             " server0=" + server_list.get(0).name + " attr0=" + v0_attr +
                             " serveri=" + server_list.get(i).name + " attri=" + vi_attr);
          totResult = curResult;
        }
      }
    }
    return totResult;
  }

  comp_result compare_simple_key(Key key) throws Exception {
    int server_count = server_list.size();
    Vector<byte[]> values = new Vector<byte[]>(server_count);
    Vector<Long> cas_numbers = new Vector<Long>(server_count);
    Vector<CollectionAttributes> attrs = new Vector<CollectionAttributes>(server_count);

    // get values
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      byte[] val;
      long cas_num;

      if (args_cas) {
        // We are checking cas numbers.  Use "gets" to get the value and
        // the cas number.
        Future<CASValue<byte[]>> fu = cli.asyncGets(key.str, tc);
        CASValue<byte[]> casv = fu.get(op_timeout, TimeUnit.MILLISECONDS);
        if (casv != null) {
          val = casv.getValue();
          cas_num = casv.getCas();
        } else {
          val = null;
          cas_num = 0;
        }
      } else {
        Future<byte[]> fu = cli.asyncGet(key.str, tc);
        val = fu.get(op_timeout, TimeUnit.MILLISECONDS);
        cas_num = 0; // pretend it is zero
      }
      values.add(i, val);
      cas_numbers.add(i, new Long(cas_num));
    }

    // get attributes
    int null_count = 0;
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      CollectionFuture<CollectionAttributes> f = cli.asyncGetAttr(key.str);
      CollectionAttributes attr = f.get(op_timeout, TimeUnit.MILLISECONDS);
      attrs.add(i, attr);
      if (attr == null) null_count++;
    }

    // compare attributes
    if (null_count == server_count) {
      return comp_result.EQUAL; // all null, okay
    }
    comp_result res = compareItemAttributes(key, attrs);
    if (res != comp_result.EQUAL) {
      return res; // comp_result.EXIST_DIFF
    }
    if (null_count > 0) {
      return comp_result.EQUAL; // some will be expired
    }

    // compare value
    byte[] v0 = values.get(0);
    for (int i = 1; i < server_count; i++) {
      byte[] vi = values.get(i);
      if (!Arrays.equals(v0, vi)) {
        System.out.println("Values are different." +
                           " server0=" + server_list.get(0).name +
                           " serveri=" + server_list.get(i).name);
        res = comp_result.VAL_DIFF;
      }
    }
    if (res != comp_result.EQUAL)
      return res;

    // compare cas
    long cas0 = cas_numbers.get(0);
    for (int i = 1; i < server_count; i++) {
      long casi = cas_numbers.get(i);
      if (cas0 != casi) {
        System.out.println("CAS numbers are different." +
                           " server0=" + server_list.get(0).name + " cas0=" + cas0 +
                           " serveri=" + server_list.get(i).name + " casi=" + casi);
        res = comp_result.CAS_DIFF;
      }
    }
    if (res != comp_result.EQUAL)
      return res;

    return res; // comp_result.EQUAL
  }

  comp_result compare_btree_key(Key key) throws Exception {
    int server_count = server_list.size();
    Vector<CollectionAttributes> attrs = new Vector<CollectionAttributes>(server_count);

    // First, get attributes and compare them
    int null_count = 0;
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      CollectionFuture<CollectionAttributes> f = cli.asyncGetAttr(key.str);
      CollectionAttributes attr = f.get(op_timeout, TimeUnit.MILLISECONDS);
      attrs.add(i, attr);
      if (attr == null) null_count++;
    }

    if (null_count == server_count) {
      return comp_result.EQUAL; // all null, okay
    }
    comp_result res = compareItemAttributes(key, attrs);
    if (res != comp_result.EQUAL) {
      return res; // comp_result.EXIST_DIFF
    }
    if (null_count > 0) {
      return comp_result.EQUAL; // some will be expired
    }

    // compare attributes
    if (attrs.get(0).getCount() == 0) { // Empty collection
      stats_btree_empty++;
      return comp_result.EQUAL;
    }

    // We cannot reliably tell the bkey type from collection attributes.
    // When type is BKEY_NULL, it is ambiguous.
    // Do a get from the first server with both types and see if which one
    // works.
    boolean binkey;
    do {
      // check bkey type while getting ony 1 element.
      myclient cli = server_list.get(0);

      CollectionFuture<Map<Long, Element<byte[]>>> f1 =
        cli.asyncBopGet(key.str, 0, Long.MAX_VALUE, null, 0, 1, false, false, tc);
      Map<Long, Element<byte[]>> val1 = f1.get(op_timeout, TimeUnit.MILLISECONDS);
      if (val1 != null) {
        binkey = false; break;
      }

      CollectionFuture<Map<ByteArrayBKey, Element<byte[]>>> f2 =
        cli.asyncBopGet(key.str, ByteArrayBKey.MIN, ByteArrayBKey.MAX, null, 0, 1, false, false, tc);
      Map<ByteArrayBKey, Element<byte[]>> val2 = f2.get(op_timeout, TimeUnit.MILLISECONDS);
      if (val2 != null) {
        binkey = true; break;
      }

      System.out.println("Attributes show a non-zero count, but cannot fetch elements.");
      if (attrs.get(0).getExpireTime() <= expDiffLimit) {
        System.out.println("The item is about to expire. Ignore the error. " +
                           "exptime=" + attrs.get(0).getExpireTime());
        return comp_result.EQUAL;
      }
      return comp_result.FETCH_ERROR;
    } while(false);


    Vector<Map<Long, Element<byte[]>>> longkey_values = null;
    Vector<Map<ByteArrayBKey, Element<byte[]>>> binkey_values = null;

    if (binkey) {
      binkey_values = new Vector<Map<ByteArrayBKey, Element<byte[]>>>(server_count);
      stats_btree_bytearraybkey++;
    } else {
      longkey_values = new Vector<Map<Long, Element<byte[]>>>(server_count);
    }

    null_count = 0;
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      boolean null_value = false;
      if (binkey) {
        CollectionFuture<Map<ByteArrayBKey, Element<byte[]>>> f =
          cli.asyncBopGet(key.str, ByteArrayBKey.MIN, ByteArrayBKey.MAX,
                          null, 0, 0xffffffff, false, false, tc);
        Map<ByteArrayBKey, Element<byte[]>> val = f.get(op_timeout, TimeUnit.MILLISECONDS);
        binkey_values.add(i, val);
        if (val == null)
          null_count++;
      } else {
        CollectionFuture<Map<Long, Element<byte[]>>> f =
          cli.asyncBopGet(key.str, 0, Long.MAX_VALUE,
                          null, 0, 0xffffffff, false, false, tc);
        Map<Long, Element<byte[]>> val = f.get(op_timeout, TimeUnit.MILLISECONDS);
        longkey_values.add(i, val);
        if (val == null)
          null_count++;
      }
    }
    if (null_count > 0) { // We have non-null attributes, but null values?
      boolean is_same = true;
      for (int i = 0; i < server_count; i++) {
        if (binkey) {
          if (binkey_values.get(i) == null && attrs.get(i).getExpireTime() > 1) {
            is_same = false; break;
          }
        } else {
          if (longkey_values.get(i) == null && attrs.get(i).getExpireTime() > 1) {
            is_same = false; break;
          }
        }
      }
      if (is_same == true) return comp_result.EQUAL;

      System.out.println("Key exists on some servers but not others." + " key=" + key.str);
      for (int i = 0; i < server_count; i++) {
        System.out.println("attr=" + attrs.get(i));
        if (binkey) {
          System.out.println(server_list.get(i).name + " " +
                     (binkey_values.get(i) == null ?  "not found" : "found"));
        } else {
          System.out.println(server_list.get(i).name + " " +
                     (longkey_values.get(i) == null ?  "not found" : "found"));
        }
      }
      return comp_result.VAL_NULL;
    }

    res = comp_result.EQUAL;
    if (binkey == false) {
      Map<Long, Element<byte[]>> v0 = longkey_values.get(0);
      Set<Long> v0_bkey = v0.keySet();

      for (int i = 1; i < server_count; i++) {
        Map<Long, Element<byte[]>> vi = longkey_values.get(i);
        Set<Long> vi_bkey = vi.keySet();

        boolean equal = true;
        do {
          if (v0_bkey.size() != vi_bkey.size()) {
            System.out.println("Element count are different. key=" + key.str +
                               "\nserver0=" + server_list.get(0).name +
                               "  ecount0=" + v0_bkey.size() + " attr0=" + attrs.get(0) +
                               "\nserveri=" + server_list.get(i).name +
                               "  ecounti=" + vi_bkey.size() + " attri=" + attrs.get(i));
            equal = false; break;
          }
          if (v0_bkey.size() == 0) {
            break; // nothing to compare
          }
          Iterator<Long> iter = v0_bkey.iterator();
          while (iter.hasNext()) {
            Long bk = iter.next();
            Element<byte[]> v0_elem = v0.get(bk);
            Element<byte[]> vi_elem = vi.get(bk);
            if (vi_elem == null) {
              System.out.println("bkey exists on some servers but not others." +
                                 " key=" + key.str + " bkey=" + bk +
                                 "\nserver0=" + server_list.get(0).name + " attr0=" + attrs.get(0) +
                                 "\nserveri=" + server_list.get(i).name + " attri=" + attrs.get(i));
              equal = false; break;
            }
            equal = Arrays.equals(v0_elem.getValue(), vi_elem.getValue());
            if (!equal) {
              System.out.println("Values are different." + " key=" + key.str + " bkey=" + bk +
                                 "\nserver0=" + server_list.get(0).name + " attr0=" + attrs.get(0) +
                                 "\nserveri=" + server_list.get(i).name + " attri=" + attrs.get(i));
              byte[] val;
              val = v0_elem.getValue();
              System.out.println("\nv0.length=" + val.length);
              hexdump(val, val.length);
              val = vi_elem.getValue();
              System.out.println("\nvi.length=" + val.length);
              hexdump(val, val.length);
              break;
            }
            equal = Arrays.equals(v0_elem.getFlag(), vi_elem.getFlag());
            if (!equal) {
              System.out.println("Eflags are different." + " key=" + key.str + " bkey=" + bk +
                                 "\nserver0=" + server_list.get(0).name + " attr0=" + attrs.get(0) +
                                 "\nserveri=" + server_list.get(i).name + " attri=" + attrs.get(i));
              break;
            }
          }
        } while(false);

        if (equal == false)
          res = comp_result.VAL_DIFF;
      }
    } else { // binkey == true
      Map<ByteArrayBKey, Element<byte[]>> v0 = binkey_values.get(0);
      Set<ByteArrayBKey> v0_bkey = v0.keySet();

      for (int i = 1; i < server_count; i++) {
        Map<ByteArrayBKey, Element<byte[]>> vi = binkey_values.get(i);
        Set<ByteArrayBKey> vi_bkey = vi.keySet();

        boolean equal = true;
        do {
          if (v0_bkey.size() != vi_bkey.size()) {
            System.out.println("Element count are different. key=" + key.str +
                               "\nserver0=" + server_list.get(0).name +
                               "  ecount0=" + v0_bkey.size() + " attr0=" + attrs.get(0) +
                               "\nserveri=" + server_list.get(i).name +
                               "  ecounti=" + vi_bkey.size() + " attri=" + attrs.get(i));
            equal = false; break;
          }
          if (v0_bkey.size() == 0) {
            break; // nothing to compare
          }
          Iterator<ByteArrayBKey> iter = v0_bkey.iterator();
          while (iter.hasNext()) {
            ByteArrayBKey bk = iter.next();
            Element<byte[]> v0_elem = v0.get(bk);
            Element<byte[]> vi_elem = vi.get(bk);
            if (vi_elem == null) {
              System.out.println("bkey exists on some servers but not others." +
                                 " key=" + key.str + " bkey=" + bk +
                                 "\nserver0=" + server_list.get(0).name + " attr0=" + attrs.get(0) +
                                 "\nserveri=" + server_list.get(i).name + " attri=" + attrs.get(i));
              equal = false; break;
            }
            equal = Arrays.equals(v0_elem.getValue(), vi_elem.getValue());
            if (!equal) {
              System.out.println("Values are different." + " key=" + key.str + " bkey=" + bk +
                                 "\nserver0=" + server_list.get(0).name + " attr0=" + attrs.get(0) +
                                 "\nserveri=" + server_list.get(i).name + " attri=" + attrs.get(i));
              byte[] val;
              val = v0_elem.getValue();
              System.out.println("\nv0.length=" + val.length);
              hexdump(val, val.length);
              val = vi_elem.getValue();
              System.out.println("\nvi.length=" + val.length);
              hexdump(val, val.length);
              break;
            }
            equal = Arrays.equals(v0_elem.getFlag(), vi_elem.getFlag());
            if (!equal) {
              System.out.println("Eflags are different." + " key=" + key.str + " bkey=" + bk +
                                 "\nserver0=" + server_list.get(0).name + " attr0=" + attrs.get(0) +
                                 "\nserveri=" + server_list.get(i).name + " attri=" + attrs.get(i));
              break;
            }
          }
        } while(false);

        if (equal == false)
          res = comp_result.VAL_DIFF;
      }
    }
    return res;
  }

  comp_result compare_set_key(Key key) throws Exception {
    int server_count = server_list.size();
    Vector<Set<byte[]>> values = new Vector<Set<byte[]>>(server_count);
    Vector<CollectionAttributes> attrs = new Vector<CollectionAttributes>(server_count);

    // get values
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      CollectionFuture<Set<byte[]>> f1 = cli.asyncSopGet(key.str, 1000000, false, false, tc);
      Set<byte[]> val = f1.get(op_timeout, TimeUnit.MILLISECONDS);
      values.add(i, val);
    }
    // get attributes
    int null_count = 0;
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      CollectionFuture<CollectionAttributes> f2 = cli.asyncGetAttr(key.str);
      CollectionAttributes attr = f2.get(op_timeout, TimeUnit.MILLISECONDS);
      attrs.add(i, attr);
      if (attr == null) null_count++;
    }

    // compare attributes
    if (null_count == server_count) {
      return comp_result.EQUAL; // all null, okay
    }
    comp_result res = compareItemAttributes(key, attrs);
    if (res != comp_result.EQUAL) {
      return res; // comp_result.EXIST_DIFF
    }
    if (null_count > 0) {
      return comp_result.EQUAL; // some will be expired
    }

    // compare values
    boolean equal = true;
    Set<byte[]> v0 = values.get(0);
    TreeSet<byte[]>  v0_tset = new TreeSet<byte[]>();
    Iterator<byte[]> v0_iter = v0.iterator();
    while (v0_iter.hasNext()) {
      byte[] v0_elem = v0_iter.next();
      assert(v0_tset.add(v0_elem) == true);
    }
    for (int i = 1; i < server_count; i++) {
      Set<byte[]> vi = values.get(i);
      do {
        if (v0.size() != vi.size()) {
          equal = false; break;
        }
        TreeSet<byte[]>  vi_tset = new TreeSet<byte[]>();
        Iterator<byte[]> vi_iter = vi.iterator();
        while (vi_iter.hasNext()) {
          byte[] vi_elem = vi_iter.next();
          assert(vi_tset.add(vi_elem) == true);
        }
        v0_iter = v0_tset.iterator();
        vi_iter = vi_tset.iterator();
        while (v0_iter.hasNext()) {
          byte[] v0_elem = v0_iter.next();
          byte[] vi_elem = vi_iter.next();
          equal = Arrays.equals(v0_elem, vi_elem);
          if (equal == false)
            break;
        }
      } while(false);
      if (!equal) {
        System.out.println("Set sizes are different." +
                           " server0=" + server_list.get(0).name + " count=" + v0.size() +
                           " serveri=" + server_list.get(i).name + " count=" + vi.size());
        res = comp_result.VAL_DIFF;
      }
    }
    return res;
  }

  comp_result compare_list_key(Key key) throws Exception {
    int server_count = server_list.size();
    Vector<List<byte[]>> values = new Vector<List<byte[]>>(server_count);
    Vector<CollectionAttributes> attrs = new Vector<CollectionAttributes>(server_count);

    // get values
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      CollectionFuture<List<byte[]>> f1 =
        cli.asyncLopGet(key.str, 0, 1000000, false, false, tc);
      List<byte[]> val = f1.get(op_timeout, TimeUnit.MILLISECONDS);
      values.add(i, val);
    }
    // get attributes
    int null_count = 0;
    for (int i = 0; i < server_count; i++) {
      myclient cli = server_list.get(i);
      CollectionFuture<CollectionAttributes> f2 = cli.asyncGetAttr(key.str);
      CollectionAttributes attr = f2.get(op_timeout, TimeUnit.MILLISECONDS);
      attrs.add(i, attr);
      if (attr == null) null_count++;
    }

    // compare attributes
    if (null_count == server_count) {
      return comp_result.EQUAL; // all null, okay
    }
    comp_result res = compareItemAttributes(key, attrs);
    if (res != comp_result.EQUAL) {
      return res; // comp_result.EXIST_DIFF
    }
    if (null_count > 0) {
      return comp_result.EQUAL; // some will be expired
    }

    // compare values
    boolean equal = true;
    List<byte[]> v0 = values.get(0);
    for (int i = 1; i < server_count; i++) {
      List<byte[]> vi = values.get(i);
      do {
        if (v0.size() != vi.size()) {
          equal = false; break;
        }
        Iterator<byte[]> v0_iter = v0.iterator();
        Iterator<byte[]> vi_iter = vi.iterator();
        while (v0_iter.hasNext()) {
          byte[] v0_elem = v0_iter.next();
          byte[] vi_elem = vi_iter.next();
          equal = Arrays.equals(v0_elem, vi_elem);
          if (equal == false)
            break;
        }
      } while(false);
      if (equal == false) {
        System.out.println("Values are different." +
                           " server0=" + server_list.get(0).name +
                           " serveri=" + server_list.get(i).name);
        res = comp_result.VAL_DIFF;
      }
    }
    return res;
  }

  // FIXME
  int stats_btree_bytearraybkey = 0;
  int stats_btree_expired_on_all = 0;
  int stats_btree_empty = 0;

  void do_compare() throws Exception {
    int simple_equal = 0;
    int simple_exist = 0;
    int simple_missing_0 = 0;
    int simple_missing_0_exp = 0;
    int simple_missing_1 = 0;
    int simple_missing_1_exp = 0;
    int simple_attr = 0;
    int simple_val = 0;
    int simple_bad = 0;
    int simple_cas = 0;
    int btree_equal = 0;
    int btree_exist = 0;
    int btree_attr = 0;
    int btree_eflag = 0;
    int btree_val = 0;
    int btree_val_null = 0;
    int btree_fetch_error = 0;
    int btree_bad = 0;
    int list_equal = 0;
    int list_exist = 0;
    int list_attr = 0;
    int list_val = 0;
    int list_bad = 0;
    int set_equal = 0;
    int set_exist = 0;
    int set_attr = 0;
    int set_val = 0;
    int set_bad = 0;
    int count = 0;

    System.out.println("Start comparing keys...");
    Set<Key> keys = keymap.keySet();
    Iterator<Key> iter = keys.iterator();
    int total_items = keymap.size();
    int progress_tick = total_items / 20;
    int next_progress_tick = progress_tick;
    while (iter.hasNext()) {
      Key key = iter.next();

      if (key.type == Key.SIMPLE) {
        comp_result res = compare_simple_key(key);
        switch (res) {
        case EQUAL:
          simple_equal++;
          break;
        case EXIST_DIFF:
          simple_exist++;
          break;
        case ATTR_DIFF:
          simple_attr++;
          break;
        case VAL_DIFF:
          simple_val++;
          break;
        case MISSING_0:
          simple_missing_0++;
          break;
        case MISSING_0_EXP:
          simple_missing_0_exp++;
          break;
        case MISSING_1:
          simple_missing_1++;
          break;
        case MISSING_1_EXP:
          simple_missing_1_exp++;
          break;
        case CAS_DIFF:
          simple_cas++;
          break;
        }
        if (res != comp_result.EQUAL) {
          simple_bad++;
          if (!args_quiet)
            System.out.println("BAD SIMPLE key=" + printable_key(key.bytes));
        }
      }
      else if (key.type == Key.BTREE) {
        comp_result res = compare_btree_key(key);
        switch (res) {
        case EQUAL:
          btree_equal++;
          break;
        case EXIST_DIFF:
          btree_exist++;
          break;
        case ATTR_DIFF:
          btree_attr++;
          break;
        case EFLAG_DIFF:
          btree_eflag++;
          break;
        case VAL_DIFF:
          btree_val++;
          break;
        case VAL_NULL:
          btree_val_null++;
          break;
        case FETCH_ERROR:
          btree_fetch_error++;
          break;
        }
        if (res != comp_result.EQUAL) {
          btree_bad++;
          //System.out.println("BAD BTREE key=" + printable_key(key.bytes));
          if (args_verbose > 0) {
            for (myclient cli : server_list) {
              dump_btree(key, cli);
            }
          }
        }
      }
      else if (key.type == Key.LIST) {
        comp_result res = compare_list_key(key);
        switch (res) {
        case EQUAL:
          list_equal++;
          break;
        case EXIST_DIFF:
          list_exist++;
          break;
        case ATTR_DIFF:
          list_attr++;
          break;
        case VAL_DIFF:
          list_val++;
          break;
        }
        if (res != comp_result.EQUAL) {
          list_bad++;
          System.out.println("BAD LIST key=" + printable_key(key.bytes));
        }
      }
      else if (key.type == Key.SET) {
        comp_result res = compare_set_key(key);
        switch (res) {
        case EQUAL:
          set_equal++;
          break;
        case EXIST_DIFF:
          set_exist++;
          break;
        case ATTR_DIFF:
          set_attr++;
          break;
        case VAL_DIFF:
          set_val++;
          break;
        }
        if (res != comp_result.EQUAL) {
          set_bad++;
          System.out.println("BAD SET key=" + printable_key(key.bytes));
        }
      }
      else {
        throw new Exception("Unknow key type=" + key.type);
      }
      count++;
      if (count >= next_progress_tick) {
        next_progress_tick += progress_tick;
        System.out.println("Checked items so far=" + count +
                           " / " + total_items);
      }
    }

    boolean isSame = true;
    do {
      if (simple_bad > 0 || simple_exist > 0 ||
          simple_attr > 0 || simple_val > 0 || simple_cas > 0 ||
          simple_missing_0 > 0 || simple_missing_0_exp > 0 ||
          simple_missing_1 > 0 || simple_missing_1_exp > 0) {
        isSame = false; break;
      }
      if (btree_bad > 0 || btree_exist > 0 || btree_attr > 0 ||
          btree_eflag > 0 || btree_val > 0 ||
          btree_val_null > 0 || btree_fetch_error > 0) {
        isSame = false; break;
      }
      if (list_bad > 0 || list_exist > 0 || list_attr > 0 || list_val > 0) {
        isSame = false; break;
      }
      if (set_bad > 0 || set_exist > 0 || set_attr > 0 || set_val > 0) {
        isSame = false; break;
      }
    } while(false);

    if (isSame == true) {
      System.out.println("Finished comparison: SAME");
    } else {
      System.out.println("Finished comparison: DIFFERENT");
    }
    System.out.printf("SIMPLE ok=%d bad=%d missing=%d attr=%d value=%d cas=%d\n" +
                      "       missing_0=%d missing_0_expired=%d missing_1=%d missing_1_expired=%d\n",
                      simple_equal, simple_bad, simple_exist, simple_attr, simple_val, simple_cas,
                      simple_missing_0, simple_missing_0_exp, simple_missing_1, simple_missing_1_exp);
    System.out.printf("BTREE  ok=%d bad=%d missing=%d attr=%d eflag=%d " +
                              "value=%d value_null=%d fetch_error=%d\n",
                      btree_equal, btree_bad, btree_exist, btree_attr,
                      btree_eflag, btree_val, btree_val_null, btree_fetch_error);
    System.out.printf("       (bytearraybkey=%d expired_on_all=%d empty=%d)\n",
                      stats_btree_bytearraybkey, stats_btree_expired_on_all,
                      stats_btree_empty);
    System.out.printf("LIST   ok=%d bad=%d missing=%d attr=%d value=%d\n",
                      list_equal, list_bad, list_exist, list_attr, list_val);
    System.out.printf("SET    ok=%d bad=%d missing=%d attr=%d value=%d\n",
                      set_equal, set_bad, set_exist, set_attr, set_val);
  }

  public static void main(String[] args) throws Exception {
    compare c = new compare();
    c.parse_args(args);
    c.prep();
    c.do_compare();
    c.cleanup();
  }

  public static void usage() {
    String txt =
      "compare options\n" +
      "-keydump dumpfile\n" +
      "-server memcached {ip}:{port}\n" +
      "\n" +
      "Example: compare -keydump dump1 -keydump dump2" +
      " -server localhost:11211 -server localhost:11212\n";
    System.out.println(txt);
    System.exit(0);
  }

  public void parse_args(String[] args) {
    for (int i = 0; i < args.length; i++) {
      if (args[i].equals("-keydump")) {
        i++;
        if (i >= args.length) {
          System.out.println("-keydump requires dumpfile");
          System.exit(0);
        }
        args_keydump.add(args[i]);
        // More file names may follow.
        // These should not start with "-"
        int j = i+1;
        while (j < args.length) {
          if (!args[j].startsWith("-")) {
            args_keydump.add(args[j]);
            i = j;
            j++;
          }
          else
            break;
        }
      }
      else if (args[i].equals("-server")) {
        i++;
        if (i >= args.length) {
          System.out.println("-server requires ip:port");
          System.exit(0);
        }
        args_server.add(args[i]);
      }
      else if (args[i].equals("-key")) {
        i++;
        if (i >= args.length) {
          System.out.println("-key requires a key string");
          System.exit(0);
        }
        args_key = args[i];
      }
      else if (args[i].equals("-cas")) {
        args_cas = true;
      }
      else if (args[i].equals("-v")) {
        args_verbose++;
      }
      else if (args[i].equals("-q")) {
        args_quiet = true;
      }
      else {
        System.out.println("Unknown argument: " + args[i]);
        usage();
      }
    }
  }

  String printable_key(byte[] b) {
    StringBuilder sb = new StringBuilder(b.length);
    for (int i = 0; i < b.length; i++) {
      if (b[i] >= 33 && b[i] <= 126)
        sb.append((char)b[i]);
      else
        sb.append('.');
    }
    return sb.toString();
  }

  static public String hex(int num, int num_digits) {
    String s = "";
    int r, mask;
    char c;

    mask = 0;
    for (int i = 0; i < num_digits; i++) {
      mask = mask << 4;
      mask = mask | 0xf;
    }
    num = num & mask;
    for (int i = 0; i < num_digits; i++) {
      r = (num % 16);
      if (r < 10)
        c = (char)('0' + r);
      else
        c = (char)('A' + r - 10);
      s = c + s;
      num = num / 16;
    }
    return s;
  }

  static public void hexdump(byte[] data, int len) {
    int d;
    int off, i;

    // offset  00 11 22 33 44 55 66 77  88 99 aa bb cc dd ee ff  ascii
    d = 0;
    off = 0;
    while (len-off >= 16) {
      // This is horrendous, I know
      String s = hex(off, 4);
      s = s + "  " + hex(data[d+0], 2);
      for (int j = 1; j < 16; j++) {
        if (j == 8)
          s = s + " ";
        s = s + " " + hex(data[d+j], 2);
      }
      s = s + " |";
      System.out.print(s);
      for (i = 0; i < 16; i++) {
        if (data[d+i] >= 33 && data[d+i] <= 126)
          System.out.print((char)data[d+i]);
        else
          System.out.print(".");
      }
      d += 16;
      off += 16;
      System.out.print("|\n");
    }
    if (len-off > 0) {
      System.out.print(hex(off, 4) + " ");
      for (i = 0; i < 16; i++) {
        if (i < len-off)
          System.out.print(" " + hex(data[d+i], 2));
        else
          System.out.print("   ");
        if (i == 7)
          System.out.print(" ");
      }
      System.out.print(" |");
      for (i = 0; i < 16; i++) {
        if (i < len-off) {
          if (data[d+i] >= 33 && data[d+i] <= 126)
            System.out.print((char)data[d+i]);
          else
            System.out.print(".");
        }
        else
          System.out.print(" ");
      }
      System.out.print("|\n");
    }
  }

  void dump_btree(Key key, myclient cli) throws Exception {
    System.out.println("dump_btree. server=" + cli.name + " key=" + key.str);

    CollectionAttributes attr = null;
    {
      CollectionFuture<CollectionAttributes> f = cli.asyncGetAttr(key.str);
      attr = f.get(op_timeout, TimeUnit.MILLISECONDS);
      if (attr == null) {
        System.out.println("Null attributes");
        return;
      }
    }
    System.out.println("attr=" + attr);

    boolean binkey;
    {
      CollectionFuture<Map<Long, Element<byte[]>>> f =
        cli.asyncBopGet(key.str, 0, Long.MAX_VALUE, null, 0,
                        0xffffffff, false, false, tc);
      Map<Long, Element<byte[]>> val = f.get(op_timeout, TimeUnit.MILLISECONDS);
      if (val != null) {
        binkey = false;
      }
      else {
        CollectionFuture<Map<ByteArrayBKey, Element<byte[]>>> f2 =
          cli.asyncBopGet(key.str, ByteArrayBKey.MIN, ByteArrayBKey.MAX,
                          null, 0, 0xffffffff, false, false, tc);
        Map<ByteArrayBKey, Element<byte[]>> val2 =
          f2.get(op_timeout, TimeUnit.MILLISECONDS);
        if (val2 != null)
          binkey = true;
        else {
          System.out.println("Cannot fetch elements.");
          return;
        }
      }
    }

    Map<Long, Element<byte[]>> longkey_value = null;
    Map<ByteArrayBKey, Element<byte[]>> binkey_value = null;

    if (binkey) {
      CollectionFuture<Map<ByteArrayBKey, Element<byte[]>>> f =
        cli.asyncBopGet(key.str, ByteArrayBKey.MIN, ByteArrayBKey.MAX,
                        null, 0, 0xffffffff, false, false, tc);
      Map<ByteArrayBKey, Element<byte[]>> val =
        f.get(op_timeout, TimeUnit.MILLISECONDS);
      binkey_value = val;
      if (val == null) {
        System.out.println("Null value");
        return;
      }
    }
    else {
      CollectionFuture<Map<Long, Element<byte[]>>> f =
        cli.asyncBopGet(key.str, 0, Long.MAX_VALUE, null, 0,
                        0xffffffff, false, false, tc);
      Map<Long, Element<byte[]>> val = f.get(op_timeout, TimeUnit.MILLISECONDS);
      longkey_value = val;
      if (val == null) {
        System.out.println("Null value");
        return;
      }
    }

    if (binkey == false) {
      Map<Long, Element<byte[]>> v0 = longkey_value;
      Set<Long> v0_bkey = v0.keySet();
      System.out.println("Elements=" + v0.size());
      if (v0_bkey.size() > 0) {
        Iterator<Long> iter = v0_bkey.iterator();
        while (iter.hasNext()) {
          Long bk = iter.next();
          Element<byte[]> v0_elem = v0.get(bk);
          System.out.println("bkey=" + bk + " length=" +
                             v0_elem.getValue().length);
        }
      }
    }
    else if (binkey == true) {
      System.out.println("UNIMPL");
    }
  }
}
