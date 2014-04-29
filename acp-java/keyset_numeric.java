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
// Default key set.

class keyset_numeric implements keyset {
  String[] set;
  int next_idx;

  // Each key is simply base-10 integer in ASCII.
  // Each has 'len' digits.

  public keyset_numeric(int num, int len, String prefix) {
    set = new String[num];
    char[] c = new char[len];
    for (int i = 0; i < num; i++) {
      int rem = i;
      for (int j = len-1; j >= 0; j--) {
        c[j] = (char)('0' + (rem % 10));
        rem = rem / 10;
      }
      set[i] = new String(c);
      if (prefix != null)
        set[i] = prefix + set[i];
    }
    reset();
  }

  public void reset() {
    next_idx = 0;
  }

  synchronized public String get_key() {
    int idx = next_idx++;
    if (next_idx >= set.length)
      next_idx = 0;
    return set[idx];
  }
}
