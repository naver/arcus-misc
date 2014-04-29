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
class valueset_default implements valueset {
  int min;
  int max;
  int next_size;
  byte start_byte;

  public valueset_default(int min, int max) {
    this.min = min;
    this.max = max;    
    reset();
  }

  public void reset() {
    next_size = min;
    start_byte = 0;
  }

  public byte[] get_value() {
    int size;
    byte b;
    synchronized(this) {
      size = next_size;
      // FIXME
      next_size += 10;
      if (next_size >= max)
        next_size = min;
      b = start_byte++;
    }

    // Don't worry about performance here.  Allocate, copy, fill would
    // take many cycles.  But then we only need to achieve 1Gbps.
    byte[] val = new byte[size];
    for (int i = 0; i < val.length; i++)
      val[i] = b++;
    
    // FIXME.  Is there any way to share the underly storage?
    return val;
  }
}
