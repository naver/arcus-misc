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
import java.util.Vector;

import net.spy.memcached.ArcusClient;
import net.spy.memcached.ArcusClientPool;

public class client implements Runnable {
  config conf;
  ArcusClientPool pool;
  ArcusClient fixed_ac = null;
  int id;
  keyset ks;
  bkey_set bks;
  valueset vset;
  client_profile profile;

  // Bookkeeping vars used when running the test
  boolean stop = false;
  long start_time;
  ArcusClient next_ac;
  int rem_requests;
  long request_start_usec;
  long request_end_usec;

  // acp.stats_thread polls these counters
  public long stat_requests = 0;
  public long stat_requests_error = 0;
  Vector<Long> latency_vector = null;

  public client(config conf, int id, ArcusClientPool pool, keyset ks,
                bkey_set bks, valueset vset,
                client_profile profile) {
    this.conf = conf;
    this.id = id;
    this.pool = pool;
    this.ks = ks;
    this.bks = bks;
    this.vset = vset;
    this.profile = profile;
  }
  
  public void set_fixed_arcus_client(ArcusClient ac) {
    fixed_ac = ac;
  }

  public void set_stop(boolean b) {
    stop = b;
  }

  public boolean before_request() {
      // Rate control
      if (conf.rate > 0) {
        // Send one request now or sleep 1ms?
        boolean send_now = false;
        while (!send_now) {
          int runtime = (int)((System.currentTimeMillis() - start_time)/1000);
          if (runtime > 0) {
            int rate = (int)(stat_requests/runtime);
            if (rate < conf.rate)
              send_now = true;
            else {
              try {
                Thread.sleep(1);
              } catch (Exception e) {
              }
            }
          }
        }
      }
      else if (conf.irg > 0) { // inter-request gap (msec)
        try {
          Thread.sleep(conf.irg);
        } catch (Exception e) {
        }
      }

      // Pick the ArcusClient
      ArcusClient ac = fixed_ac;
      if (ac == null) {
        ac = pool.getClient();
      }
      next_ac = ac;

      stat_requests++;
      if (rem_requests == 0) {
        // Run forever.
        // acp counts down the run time and tells this thread to stop
        // when the time runs out.
      }
      else {
        // Count down, and stop if there are no requests remaining.
        rem_requests--;
        if (rem_requests <= 0)
          rem_requests = -1; // Hack
      }
      
      // Response time
      request_start_usec = System.nanoTime() / 1000;
      return true;
  }

  public synchronized Vector<Long> remove_latency_vector() {
    Vector<Long> v = latency_vector;
    latency_vector = null;
    return v;
  }

  public synchronized void add_latency(long lat) {
    // Do not bother adding too many, in case the stats thread does not
    // consume the vector
    int limit = conf.rate * 2;
    if (limit == 0)
      limit = 10000;

    if (latency_vector == null) {
      latency_vector = new Vector<Long>(limit);
    }
    if (latency_vector.size() < limit)
      latency_vector.add(lat);
  }

  public boolean after_request(boolean ok) {
    if (ok) {
      // Response time.  Only count success responses.  FIXME
      request_end_usec = System.nanoTime() / 1000;
      if (request_end_usec >= request_start_usec) {
        long lat = request_end_usec - request_start_usec;
        add_latency(lat);
      }
      else {
        // Ignore it.  Do not bother with wraparound.
      }
    }
    else
      stat_requests_error++;

    if (rem_requests == -1)
      return false; // Stop the test
    return true;
  }

  public void run() {
    rem_requests = conf.request;
    start_time = System.currentTimeMillis();
    
    System.out.printf("Client is running. id=%d\n", id);    
    while (!stop) { 
      /* Keep running */   
      boolean do_another = profile.do_test(this);
      if (!do_another) {
        stop = true;
        continue;
      }
    }
    System.out.printf("Client stopped. id=%d\n", id);
  }
}
