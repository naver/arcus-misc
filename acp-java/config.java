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
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;

// Every configuration variable we use is in this file.

public class config {

  public boolean pretty_stat = false;
  public String zookeeper = "127.0.0.1:2181";
  public String service_code = "test";
  public String single_server = null;
  public int client = 10;
  public int rate = 0;
  public int irg = 0;  // inter-request gap (msec)
  public int request = 100000;
  public int time = 0; // Run this many seconds.  0=forever
  public int pool = 1;
  public int pool_size = 10;
  public boolean pool_use_random = false;
  public String keyset_profile = "default";
  public int keyset_size = 100000;
  public int keyset_length = 0;
  public String key_prefix = null;
  public String valueset_profile = "default";
  public int valueset_min_size = 10;
  public int valueset_max_size = 4096;
  public String client_profile = "standard_mix";
  public int client_exptime = 100;
  public int client_mget_keys = 500;

  public config() {
  }

  // Read from the configuration file.
  public void load_file(String path) throws Exception {
    File file = new File(path);
    FileInputStream fin = new FileInputStream(file);
    BufferedInputStream bin = new BufferedInputStream(fin);
    int s;
    byte[] linebuf = new byte[1024];
    int off = 0;
    boolean first_char = true;
    boolean comment = false;
    boolean dq = false; // can handle exactly one pair of double quotes
    while ((s = bin.read()) > 0) {
      // grab a line
      
      // eat whitespaces
      if (dq == false && (s == ' ' || s == '\t'))
        continue;
      
      if (s == '"') {
        if (dq == true) {
          dq = false;
          continue;
        }
        else {
          dq = true;
          continue;
        }
      }
      
      // got one line 
      if (s == '\n') {
        if (off > 0 && comment == false) {
          String line = new String(linebuf, 0, off, "US-ASCII");
          process_line(line);
        }
        off = 0;
        comment = false;
        first_char = true;
      }
      else {
        if (first_char && s == '#') {
          first_char = false;
          comment = true;
        }
        linebuf[off++] = (byte)s;
      }
    }
  }

  public void process_line(String line) throws Exception {
    String key, val;
    int idx = line.indexOf('=');
    if (idx < 0)
      throw new Exception("invalid line=" + line);
    key = line.substring(0, idx);
    val = line.substring(idx+1);
    System.out.println(key + "=" + val);

    if (key.equals("zookeeper")) {
      zookeeper = val;
    }
    else if (key.equals("service_code")) {
      service_code = val;
    }
    else if (key.equals("single_server")) {
      single_server = val;
    }
    else if (key.equals("client")) {
      client = Integer.parseInt(val);
    }
    else if (key.equals("rate")) {
      rate = Integer.parseInt(val);
    }
    else if (key.equals("irg")) {
      irg = Integer.parseInt(val);
    }
    else if (key.equals("request")) {
      request = Integer.parseInt(val);
    }
    else if (key.equals("time")) {
      time = Integer.parseInt(val);
    }
    else if (key.equals("pool")) {
      pool = Integer.parseInt(val);
    }
    else if (key.equals("pool_size")) {
      pool_size = Integer.parseInt(val);
    }
    else if (key.equals("pool_use_random")) {
      pool_use_random = val.equals("true");
    }
    else if (key.equals("keyset_size")) {
      keyset_size = Integer.parseInt(val);
    }
    else if (key.equals("keyset_length")) {
      keyset_length = Integer.parseInt(val);
    }
    else if (key.equals("keyset_profile")) {
      keyset_profile = val;
    }
    else if (key.equals("valueset_min_size")) {
      valueset_min_size = Integer.parseInt(val);
    }
    else if (key.equals("valueset_max_size")) {
      valueset_max_size = Integer.parseInt(val);
    }
    else if (key.equals("valueset_profile")) {
      valueset_profile = val;
    }
    else if (key.equals("client_profile")) {
      client_profile = val;
    }
    else if (key.equals("client_exptime")) {
      client_exptime = Integer.parseInt(val);
    }
    else if (key.equals("client_mget_keys")) {
      client_mget_keys = Integer.parseInt(val);
    }
    else if (key.equals("key_prefix")) {
      key_prefix = val;
    }
    else {
      throw new Exception("Unknown configuration key/value. line="+ line);
    }
  }
}
