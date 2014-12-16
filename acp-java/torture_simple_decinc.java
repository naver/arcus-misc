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

public class torture_simple_decinc implements client_profile {
  public boolean do_test(client cli) {
    try {
      if (!do_simple_test(cli))
        return false;
    } catch (Exception e) {
      System.out.printf("client_profile exception. id=%d exception=%s\n", 
                        cli.id, e.toString());
      e.printStackTrace();
    }
    return true;
  }

  public boolean do_simple_test(client cli) throws Exception {
    // Pick a key
    String key = cli.ks.get_key();

    // Insert an item
    if (!cli.before_request())
      return false;
    // Base-10 digits
    String str = String.format("%d", cli.id+1); // +1 to avoid 0
    byte[] val = str.getBytes();
    Future<Boolean> fb = 
      cli.next_ac.set(key, cli.conf.client_exptime, val, raw_transcoder.raw_tc);
    boolean ok = fb.get(1000L, TimeUnit.MILLISECONDS);
    if (!ok) {
      System.out.printf("set failed. id=%d key=%s\n", cli.id, key);
    }
    if (!cli.after_request(ok))
      return false;

    // Decrement to 0
    if (!cli.before_request())
      return false;
    Future<Long> fl = cli.next_ac.asyncDecr(key, cli.id+1);
    Long lv = fl.get(1000L, TimeUnit.MILLISECONDS);
    // The returned value is the result of decrement.
    ok = true;
    if (lv.longValue() != 0) {
      ok = false;
      System.out.println("Unexpected value from decrement. result=" +
                         lv.longValue());
    }
    if (!cli.after_request(ok))
      return false;

    // Increment to cli.id+2
    if (!cli.before_request())
      return false;
    fl = cli.next_ac.asyncIncr(key, cli.id+2);
    lv = fl.get(1000L, TimeUnit.MILLISECONDS);
    // The returned value is the result of decrement.
    ok = true;
    if (lv.longValue() != cli.id+2) {
      ok = false;
      System.out.println("Unexpected value from increment. result=" +
                         lv.longValue() + " expected=" + (cli.id+2));
    }
    if (!cli.after_request(ok))
      return false;

    // Replace the value with a pattern
    if (!cli.before_request())
      return false;
    val = cli.vset.get_value();
    fb = cli.next_ac.replace(key, cli.conf.client_exptime, val, raw_transcoder.raw_tc);
    ok = fb.get(1000L, TimeUnit.MILLISECONDS);
    if (!ok) {
      System.out.printf("replace failed. id=%d key=%s\n", cli.id, key);
    }
    if (!cli.after_request(ok))
      return false;

    return true;
  }
}
