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

public class get implements client_profile {  
  public boolean do_test(client cli) {
    try {
      if (!do_get(cli))
        return false;
    } catch (Exception e) {
      System.out.printf("client_profile exception. id=%d exception=%s\n", 
                        cli.id, e.toString());
      //e.printStackTrace();
      //System.exit(0);
    }
    return true;
  }

  public boolean do_get(client cli) throws Exception {
    if (!cli.before_request())
      return false;
    String key = cli.ks.get_key();
    Future<byte[]> f = cli.next_ac.asyncGet(key, raw_transcoder.raw_tc);
    byte[] val = f.get(1000L, TimeUnit.MILLISECONDS);
    boolean ok = true;
    if (val == null) {
      System.out.printf("get failed. id=%d\n", cli.id);
      ok = false;
    }
    if (!cli.after_request(ok))
      return false;
    return true;
  }
}
