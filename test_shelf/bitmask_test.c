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
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

int
main(void)
{
    int i;
    uint32_t key_hash_mask[32];
    uint8_t  set_used_mask[8];
    uint8_t  set_free_mask[8];

    /* key hash mask */
    key_hash_mask[0] = 0x00000000;
    key_hash_mask[1] = 0x00000001;
    for (i = 2; i < 32; i++) {
        key_hash_mask[i] = ((0x00000001 << i) | key_hash_mask[i-1]);
    }
    for (i = 0; i < 32; i++) {
        fprintf(stderr, "key_hash_mask[%d]=%X\n", i, key_hash_mask[i]);
    }

    /* set_used_mask */
    set_used_mask[0] = 0x80;
    set_used_mask[1] = 0x40;
    set_used_mask[2] = 0x20;
    set_used_mask[3] = 0x10;
    set_used_mask[4] = 0x08;
    set_used_mask[5] = 0x04;
    set_used_mask[6] = 0x02;
    set_used_mask[7] = 0x01;
    for (i = 0; i < 8; i++) {
        fprintf(stderr, "set_used_mask[%d]=%X\n", i, (uint8_t)set_used_mask[i]);
    }

    for (i = 1; i < 8; i++) {
        set_used_mask[i] = (0x80 >> i);
    }
    for (i = 0; i < 8; i++) {
        fprintf(stderr, "set_used_mask[%d]=%X\n", i, (uint8_t)set_used_mask[i]);
    }

    /* set_free_mask */
    set_free_mask[0] = 0x7F;
    set_free_mask[1] = 0xBF;
    set_free_mask[2] = 0xDF;
    set_free_mask[3] = 0xEF;
    set_free_mask[4] = 0xF7;
    set_free_mask[5] = 0xFB;
    set_free_mask[6] = 0xFD;
    set_free_mask[7] = 0xFE;
    for (i = 0; i < 8; i++) {
        fprintf(stderr, "set_free_mask[%d]=%X\n", i, (uint8_t)set_free_mask[i]);
    }

    for (i = 1; i < 8; i++) {
        set_free_mask[i] = set_used_mask[i] ^ 0xFF;
    }
    for (i = 0; i < 8; i++) {
        fprintf(stderr, "set_free_mask[%d]=%X\n", i, (uint8_t)set_free_mask[i]);
    }

    exit(0);
}

