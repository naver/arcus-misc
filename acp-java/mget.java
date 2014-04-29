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
import java.util.Collection;
import java.util.LinkedList;
import java.util.Map;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import net.spy.memcached.internal.BulkFuture;

public class mget implements client_profile {  
  Collection<String> keys;
  int keys_size;

  public mget(config conf, keyset ks) {
    Collection<String> key_list = new LinkedList<String>();
    for (int i = 0; i < conf.client_mget_keys; i++) {
      String key = ks.get_key();
      key_list.add(key);
    }
    keys = key_list;
    keys_size = key_list.size();
  }

  public boolean do_test(client cli) {
    try {
      if (!do_mget(cli))
        return false;
    } catch (Exception e) {
      System.out.printf("client_profile exception. id=%d exception=%s\n", 
                        cli.id, e.toString());
      //e.printStackTrace();
      //System.exit(0);
    }
    return true;
  }

  public boolean do_mget(client cli) throws Exception {
    if (!cli.before_request())
      return false;
    BulkFuture<Map<String, byte[]>> f =
      cli.next_ac.asyncGetBulk(keys, raw_transcoder.raw_tc);
    Map<String, byte[]> val = f.get(1000L, TimeUnit.MILLISECONDS);
    boolean ok = true;
    if (val == null || val.size() != keys_size) {
      System.out.printf("GetBulk failed. id=%d\n", cli.id);
    }
    if (!cli.after_request(ok))
      return false;
    return true;
  }
}
