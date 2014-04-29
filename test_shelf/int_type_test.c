/*
 * Copyright 2012-2014 NAVER Corp.
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
#include <stdio.h>
#include <inttypes.h>

static void print_uint64_value(const uint64_t value)
{
  int i;  
  unsigned char *ptr;
  printf("uint64_val = %lu (%lX)\n", value, value);
  for (i=0; i<sizeof(uint64_t); i++) {
    ptr = ((unsigned char*)&value)+i;
    printf("%x int64_val[%d] = (%X)\n", ptr, i, *ptr);
  }
}

main()
{
  int32_t  int32_val = 1024;
  int64_t  int64_val = 1024;
  uint32_t uint32_val = 1024;
  uint64_t uint64_val = 1024; 
  unsigned char data[8];

  int32_val  = 1024;
  int64_val  = 1024;
  uint32_val = 1024;
  uint64_val = 1024; 
  printf("int32_val  = %d (%X)\n", int32_val, int32_val);
  printf("int64_val  = %d (%X)\n", int64_val, int64_val);
  printf("uint32_val = %u (%X)\n", uint32_val, uint32_val);
  printf("uint64_val = %u (%X)\n", uint64_val, uint64_val);

  int32_val  = 0x1024FA3C;
  int64_val  = 0x000000001024FA3C;
  uint32_val = 0x1024FA3C;
  uint64_val = 0x000000001024FA3C;
  printf("int32_val  = %d (%X)\n", int32_val, int32_val);
  printf("int64_val  = %d (%X)\n", int64_val, int64_val);
  printf("uint32_val = %u (%X)\n", uint32_val, uint32_val);
  printf("uint64_val = %u (%X)\n", uint64_val, uint64_val);

  printf("\n");
  uint64_val = 0x123456781024FA3C;
  printf("uint64_val = %u (%X)\n", uint64_val, uint64_val);
  print_uint64_value(uint64_val); 

  printf("\n");
  data[0] = 0x12;
  data[1] = 0x34;
  data[2] = 0x56;
  data[3] = 0x78;
  data[4] = 0x10;
  data[5] = 0x24;
  data[6] = 0xFA;
  data[7] = 0x3C;
  printf("uint64_val = %u (%X)\n", *(uint64_t*)data, *(uint64_t*)data);
  print_uint64_value(*(uint64_t*)data);
}
