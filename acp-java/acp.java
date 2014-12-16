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
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Vector;

import net.spy.memcached.ArcusClient;
import net.spy.memcached.ArcusClientPool;
import net.spy.memcached.ConnectionFactoryBuilder;
import net.spy.memcached.DefaultConnectionFactory;

class acp {
  config conf;
  ArcusClientPool[] pool;
  client[] client;
  Thread[] client_thread;

  public acp(config conf) {
    this.conf = conf;
  }

  // Used to create ArcusClient that uses the single server specified by
  // the user.  It does not use ZK at all.
  class myfactory extends DefaultConnectionFactory {
  }
  class myclient extends ArcusClient {
    myclient(List<InetSocketAddress> addrs) throws Exception {
      super(new myfactory(), addrs);
    }
  }

  public void setup() throws Exception {
    // Key profile
    keyset kset = null;
    if (conf.keyset_profile.equals("default")) {
      kset = new keyset_default(conf.keyset_size, conf.key_prefix);
    }
    else if (conf.keyset_profile.equals("numeric")) {
      kset = new keyset_numeric(conf.keyset_size, conf.keyset_length,
                                conf.key_prefix);
    }
    if (kset == null) {
      System.out.println("Cannot find keyset profile=" + conf.keyset_profile);
      System.exit(0);
    }

    // Client profile
    client_profile profile = null;
    if (conf.client_profile.equals("standard_mix")) {
      profile = new standard_mix();
    }
    else if (conf.client_profile.equals("create_keys")) {
      profile = new create_keys();
    }
    else if (conf.client_profile.equals("btree_elem_upsert")) {
      profile = new btree_elem_upsert();
    }
    else if (conf.client_profile.equals("mget")) {
      profile = new mget(conf, kset);
    }
    else if (conf.client_profile.equals("get")) {
      profile = new get();
    }
    else if (conf.client_profile.equals("simple_set")) {
      profile = new simple_set();
    }
    else if (conf.client_profile.equals("simple_getset")) {
      profile = new simple_getset();
    }
    else if (conf.client_profile.equals("torture_cas")) {
      profile = new torture_cas();
    }
    else if (conf.client_profile.equals("torture_simple_decinc")) {
      profile = new torture_simple_decinc();
    }
    else if (conf.client_profile.equals("torture_simple_sticky")) {
      profile = new torture_simple_sticky();
    }
    else if (conf.client_profile.equals("torture_btree")) {
      profile = new torture_btree();
    }
    else if (conf.client_profile.equals("torture_btree_decinc")) {
      profile = new torture_btree_decinc();
    }
    else if (conf.client_profile.equals("torture_btree_ins_del")) {
      profile = new torture_btree_ins_del();
    }
    else if (conf.client_profile.equals("torture_btree_bytebkey")) {
      profile = new torture_btree_bytebkey();
    }
    else if (conf.client_profile.equals("torture_btree_maxbkeyrange")) {
      profile = new torture_btree_maxbkeyrange();
    }
    else if (conf.client_profile.equals("torture_btree_bytemaxbkeyrange")) {
      profile = new torture_btree_bytemaxbkeyrange();
    }
    else if (conf.client_profile.equals("torture_btree_exptime")) {
      profile = new torture_btree_exptime();
    }
    else if (conf.client_profile.equals("torture_set")) {
      profile = new torture_set();
    }
    else if (conf.client_profile.equals("torture_set_ins_del")) {
      profile = new torture_set_ins_del();
    }
    else if (conf.client_profile.equals("torture_list")) {
      profile = new torture_list();
    }
    else if (conf.client_profile.equals("torture_list_ins_del")) {
      profile = new torture_list_ins_del();
    }
    else if (conf.client_profile.equals("tiny_btree")) {
      profile = new tiny_btree();
    }
    else if (conf.client_profile.equals("torture_btree_replace")) {
      profile = new torture_btree_replace();
    }
    else if (conf.client_profile.equals("torture_arcus_integration")) {
      profile = new torture_arcus_integration();
    }
    if (profile == null) {
      System.out.println("Cannot find client profile=" + conf.client_profile);
      System.exit(0);
    }

    // Value profile
    valueset vset = null;
    if (conf.valueset_profile.equals("default")) {
      vset = new valueset_default(conf.valueset_min_size, 
                                  conf.valueset_max_size);
    }
    if (vset == null) {
      System.out.println("Cannot find valueset profile=" + 
                         conf.valueset_profile);
      System.exit(0);
    }

    // Make Arcus pools
    pool = new ArcusClientPool[conf.pool];
    if (conf.single_server != null) {
      String[] temp = conf.single_server.split(":");
      InetSocketAddress inet =
        new InetSocketAddress(temp[0], Integer.parseInt(temp[1]));
      List<InetSocketAddress> inet_list = new ArrayList<InetSocketAddress>();
      inet_list.add(inet);

      for (int i = 0; i < conf.pool; i++) {
        ArcusClient[] clients = new ArcusClient[conf.pool_size];
        for (int j = 0; j < conf.pool_size; j++) {
          clients[j] = new myclient(inet_list);
        }
        ArcusClientPool p = new ArcusClientPool(conf.pool_size, clients);
        pool[i] = p;
      }
    }
    else {
      // Use ZK
      for (int i = 0; i < conf.pool; i++) {
        System.out.printf("Creating ArcusClientPool. id=%d\n", i);
        ArcusClientPool p = 
          ArcusClient.createArcusClientPool(conf.zookeeper, conf.service_code,
                                            new ConnectionFactoryBuilder(),
                                            conf.pool_size);
        pool[i] = p;
      }
    }

    // Make acp clients
    client = new client[conf.client];

    // First, compute the number of clients per pool
    int clients_per_pool = conf.client / conf.pool;
    while (clients_per_pool * conf.pool < conf.client) {
      // Wow, this is stupid...
      clients_per_pool++;
    }

    // Then create each client and pick the pool and the ArcusClient.
    int pool_idx = 0;
    int num_clients = 0;
    int p_client_idx = 0;
    ArcusClientPool p = pool[pool_idx];
    ArcusClient[] p_clients = p.getAllClients();
    for (int i = 0; i < conf.client; i++) {
      // Move to the next pool if the current one is full
      if (num_clients >= clients_per_pool) {
        pool_idx++;
        p = pool[pool_idx];
        p_clients = p.getAllClients();
        num_clients = 0;
        p_client_idx = 0;
      }

      // Each client has its own bkey set.  FIXME
      client cli = new client(conf, i, p, kset, new bkey_set(100), vset,
                              profile);

      // Pick ArcusClient in a round robin fashion, if we are not picking
      // random clients for each request.
      if (!conf.pool_use_random) {
        if (p_client_idx >= p_clients.length) {
          p_client_idx = 0;
        }
        cli.set_fixed_arcus_client(p_clients[p_client_idx]);
        System.out.printf("New client. id=%d pool=%d arcus_client=%d\n",
                          i, pool_idx, p_client_idx);
        p_client_idx++;
      }
      else {
        System.out.printf("New client. id=%d pool=%d arcus_client=random\n",
                          i, pool_idx);
      }
      client[i] = cli;
    }

    // Create threads
    client_thread = new Thread[conf.client];
    for (int i = 0; i < conf.client; i++) {
      client_thread[i] = new Thread(client[i]);
    }
  }

  public void run_bench() {
    System.out.println("Starting clients...");
    // Start client threads
    for (int i = 0; i < client_thread.length; i++) {
      client_thread[i].start();
    }

    // Start the stats thread
    stats_display stats = new stats_display(conf);
    Thread stats_thread = new Thread(stats);
    stats_thread.start();

    // Wait for all client threads to terminate
    int join_count = 0;
    while (join_count < client_thread.length) {
      join_count = 0;
      for (Thread t : client_thread) {
        try {
          t.join();
          join_count++;
        } catch (Exception e) {
        }
      }
    }
    System.out.println("All clients have stopped.");

    // Wait for the stats thread to terminate
    stats.stop = true;
    try {
      stats_thread.join();
    } catch (Exception e) {
    }
    
    System.out.println("Tests finished.");
  }

  class stats_display implements Runnable {
    config conf;
    boolean stop = false;

    public stats_display(config conf) {
      this.conf = conf;
    }

    public void run() {
      long prev_time = System.currentTimeMillis();
      long start_time = prev_time;
      long prev_stat_requests_sum = 0;
      long prev_stat_requests_error = 0;
      long avg_request_rate = 0;
      int run_time = conf.time;
      Vector<Vector<Long>> lat_vectors = new Vector<Vector<Long>>(conf.client);
      long[] prev_stat_requests = new long[client.length];
      long[] cur_stat_requests = new long[client.length];
      int line = 0;

      while (!stop) {
        try {
          Thread.sleep(1000);
        } catch (Exception e) {
        }

        // Count down the run time
        if (run_time > 0) {
          run_time--;
          if (run_time <= 0) {
            System.out.println("Ran tests long enough. Stopping clients...");
            // Tell clients to stop
            for (client cli : client) {
              cli.set_stop(true);
            }
          }
        }
        
        if (conf.pretty_stat && (line % 30) == 0) {
          System.out.printf("Requests/s\t\tLatency (msec)\n" +
                            "Okay\tError\t\tmin\t50th\t90th\n" +
                            "---------------------------------------------\n");
        }
        line++;

        long cur_time = System.currentTimeMillis();
        long diff_time = (cur_time - prev_time) / 1000;
        prev_time = cur_time;

        // Gather stats from each client
        long stat_requests_sum = 0;
        long stat_requests_error = 0;
        int num_latencies = 0;
        for (int i = 0; i < client.length; i++) {
          client cli = client[i];

          cur_stat_requests[i] = cli.stat_requests;
          stat_requests_sum += cur_stat_requests[i];
          stat_requests_error += cli.stat_requests_error;
          Vector<Long> v = cli.remove_latency_vector();
          lat_vectors.add(v);
          if (v != null)
            num_latencies += v.size();
        }
        long delta_requests = stat_requests_sum - prev_stat_requests_sum;
        prev_stat_requests_error = stat_requests_error -
          prev_stat_requests_error;
        long request_rate = delta_requests/diff_time;
        avg_request_rate = (95 * avg_request_rate + 5 * request_rate) / 100;

        long delta_error = prev_stat_requests_error;
        long error_rate = prev_stat_requests_error/diff_time;

        if (!conf.pretty_stat) {
          System.out.printf
            ("elapsed(s)=%d requests/s=%d [error=%d]" +
             " cumulative requests/client=%d [error=%d]" +
             " cumulative requests/s=%d\n",
             (cur_time - start_time) / 1000, request_rate,
             prev_stat_requests_error/diff_time,
             stat_requests_sum / conf.client, stat_requests_error / conf.client,
             avg_request_rate);
        }
        prev_stat_requests_sum = stat_requests_sum;
        prev_stat_requests_error = stat_requests_error;

        long sum = 0;
        for (int i = 0; i < client.length; i++) {
          prev_stat_requests[i] = cur_stat_requests[i] - prev_stat_requests[i];
          sum += prev_stat_requests[i];
        }
        Arrays.sort(prev_stat_requests);
        if (!conf.pretty_stat) {
          System.out.printf("per-client requests. min=%d median=%d avg=%d max=%d\n",
                            prev_stat_requests[0], prev_stat_requests[client.length/2],
                            sum / client.length, prev_stat_requests[client.length-1]);
        }

        for (int i = 0; i < client.length; i++) {
          prev_stat_requests[i] = cur_stat_requests[i];
        }

        long[] all_latencies = new long[num_latencies];
        int idx = 0;
        for (int i = 0; i < lat_vectors.size(); i++) {
          Vector<Long> v = lat_vectors.get(i);
          if (v != null) {
            for (long lat : v) {
              all_latencies[idx] = lat;
              idx++;
            }
            v.clear();
          }
        }
        lat_vectors.clear();
        Arrays.sort(all_latencies);
        int p = all_latencies.length / 100;
        if (!conf.pretty_stat && all_latencies.length > 0) {
          System.out.printf("latency (usec). min=%d 10th=%d" +
                            " 50th=%d 80th=%d 90th=%d 99th=%d max=%d\n",
                            all_latencies[0], 
                            all_latencies[p * 10],
                            all_latencies[p * 50],
                            all_latencies[p * 80],
                            all_latencies[p * 90],
                            all_latencies[p * 99],
                            all_latencies[all_latencies.length-1]);
        }

        //request_rate -= error_rate;
        //if (request_rate < 0) request_rate = 0;
        delta_requests -= delta_error;
        if (delta_requests < 0) delta_requests = 0;
        if (conf.pretty_stat && all_latencies.length > 0) {
          /*
          System.out.printf("%d\t%d\t\t%d\t%d\t%d\n", delta_requests, delta_error, //request_rate, error_rate,
                            all_latencies[0], all_latencies[p * 50], all_latencies[p * 90]);
          */
          System.out.printf("%d\t%d\t\t%.02f\t%.02f\t%.02f\n", delta_requests, delta_error, //request_rate, error_rate,
                            all_latencies[0]/1000.0, all_latencies[p * 50]/1000.0, 
                            all_latencies[p * 90]/1000.0);
        }
        else {
          System.out.printf("%d\t%d\t\tX\tX\tX\n", delta_requests, delta_error); //request_rate, error_rate);
        }
      }
    }
  }

  public static void main(String[] args) throws Exception {
    config conf = new config();
    String config_path = null;
    for (int i = 0; i < args.length; i++) {
	    if (args[i].equals("-config")) {
        i++;
        if (i >= args.length) {
          System.out.println("-config requires path");
          System.exit(0);
        }
        config_path = args[i];
      }
    }
    if (config_path != null) {
      conf.load_file(config_path);
    }

    parse_args(args, conf);

    acp acp = new acp(conf);
    acp.setup();
    acp.run_bench();
    System.exit(0);
  }

  public static void usage() {
    String txt =
      "acp options\n" +
      "-config path\n" + 
      "    Use configuration file at <path>\n" +
      "-pretty-stat\n" + 
      "\n" +
      "--------------------------------------\n" +
      "Use the following to override settings\n" +
      "--------------------------------------\n" +
      "-zookeeper host:port\n" +
      "    Zookeeper server address\n" +
      "-service-code svc\n" +
      "    Use <svc> to find the cluster in ZK\n" +
      "-client num\n" +
      "    Use <num> Arcus clients\n" +
      "-rate rps\n" +
      "    Each client does <rps> requests per second.\n" +
      "    0=as fast as possible\n" +
      "-request num\n" +
      "    Each client does <num> requests and quits.\n" +
      "    0=infinite\n" +
      "-time sec\n" +
      "    Run for <sec> seconds and quit.\n" +
      "    0=forever\n" +
      "-pool num\n" +
      "    Use <num> ArcusClientPool's\n" +
      "-pool-size num\n" +
      "    Use <num> ArcusClient's per ArcusClientPool\n" +
      "\n" +
      "Example: acp -zookeeper 127.0.0.1:2181 -service-code test" +
      " -clients 100 -rate 1000\n";
    System.out.println(txt);
    System.exit(0);
  }

  public static void parse_args(String[] args, config conf) {
    for (int i = 0; i < args.length; i++) {
	    if (args[i].equals("-config")) {
        i++;
        if (i >= args.length) {
          System.out.println("-config requires path");
          System.exit(0);
        }
        // Ignore
      }
	    else if (args[i].equals("-pretty-stat")) {
        conf.pretty_stat = true;
      }
	    else if (args[i].equals("-zookeeper")) {
        i++;
        if (i >= args.length) {
          System.out.println("-zookeeper requires host:port");
          System.exit(0);
        }
        conf.zookeeper = args[i];
	    }
	    else if (args[i].equals("-service-code")) {
        i++;
        if (i >= args.length) {
          System.out.println("-service-code requires service string");
          System.exit(0);
        }
        conf.service_code = args[i];
	    }
	    else if (args[i].equals("-client")) {
        i++;
        if (i >= args.length) {
          System.out.println("-client requires the number of client");
          System.exit(0);
        }
        conf.client = Integer.parseInt(args[i]);
	    }
	    else if (args[i].equals("-rate")) {
        i++;
        if (i >= args.length) {
          System.out.println("-rate requires the number of requests" +
                             " per second");
          System.exit(0);
        }
        conf.rate = Integer.parseInt(args[i]);
	    }
	    else if (args[i].equals("-request")) {
        i++;
        if (i >= args.length) {
          System.out.println("-request requires the number of requests" +
                             " per client");
          System.exit(0);
        }
        conf.request = Integer.parseInt(args[i]);
	    }
	    else if (args[i].equals("-time")) {
        i++;
        if (i >= args.length) {
          System.out.println("-time requires the number of seconds");
          System.exit(0);
        }
        conf.time = Integer.parseInt(args[i]);
	    }
	    else if (args[i].equals("-pool")) {
        i++;
        if (i >= args.length) {
          System.out.println("-pool requires the number of pools");
          System.exit(0);
        }
        conf.pool = Integer.parseInt(args[i]);
	    }
	    else if (args[i].equals("-pool-size")) {
        i++;
        if (i >= args.length) {
          System.out.println("-pool-size requires the number of pools");
          System.exit(0);
        }
        conf.pool_size = Integer.parseInt(args[i]);
      }
      else if (args[i].equals("-client-profile")) {
        i++;
        if (i >= args.length) {
          System.out.println("-client-profile requires the profile name");
          System.exit(0);
        }
        conf.client_profile = args[i];
      }
      else {
        System.out.println("Unknown argument: " + args[i]);
        usage();
      }
    }
  }
}
