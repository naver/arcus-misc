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
import net.spy.memcached.CachedData;
import net.spy.memcached.transcoders.Transcoder;

// Receive raw bytes
class raw_transcoder implements Transcoder<byte[]> {
  public static raw_transcoder raw_tc = new raw_transcoder();

  public boolean asyncDecode(CachedData d) {
    return false;
  }
  
  public CachedData encode(byte[] o) {
    return new CachedData(0, o, getMaxSize());
  }
  
  public byte[] decode(CachedData d) {
    return d.getData();
  }
  
  public int getMaxSize() {
    return 1024*1024;
  }
}
