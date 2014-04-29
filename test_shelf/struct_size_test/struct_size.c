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

#define MAX_BKEY_LENG  31
#define MAX_EFLAG_LENG 31

    typedef struct {
        unsigned char from_bkey[MAX_BKEY_LENG];
        unsigned char to_bkey[MAX_BKEY_LENG];
        uint8_t from_nbkey;
        uint8_t to_nbkey;
    } bkey_range;

    typedef union {
        uint64_t      ival;
        unsigned char cval[MAX_BKEY_LENG];
    } bkey1_t;

    typedef struct {
        bkey1_t from_bkey;
        bkey1_t to_bkey;
        uint8_t from_nbkey;
        uint8_t to_nbkey;
    } bkey1_range;

    typedef struct {
        union {
            uint64_t      ui;
            unsigned char uc[MAX_BKEY_LENG];
        } val;
        uint8_t len;
    } bkey2_t;

    typedef struct {
        bkey2_t from_bkey;
        bkey2_t to_bkey;
    } bkey2_range;

    typedef union {
       uint64_t u64;
       struct { 
          unsigned char val[MAX_BKEY_LENG];
          uint8_t       len;
       } u8;
    } bkey3_t;

    typedef struct {
        bkey3_t from_bkey;
        bkey3_t to_bkey;
    } bkey3_range;

    typedef struct {
        unsigned char bitwval[MAX_EFLAG_LENG];
        unsigned char compval[MAX_EFLAG_LENG];
        uint8_t nbitwval;
        uint8_t ncompval;
        uint8_t fwhere;
        uint8_t bitwise_op;
        uint8_t compare_op;
        uint8_t reserved[5];
    } elem_filter;

    typedef struct {
        elem_filter efilter;
        uint8_t reserved;
        bkey1_range bkrange;
    } msg_body1;

    typedef struct {
        elem_filter efilter;
        uint32_t reserved1;
        uint32_t reserved2;
        bkey1_range bkrange;
    } msg_body2;

    typedef struct {
        elem_filter efilter;
        uint8_t reserved[8];
        bkey1_range bkrange;
    } msg_body3;

main()
{
    char dummy[2];
    char a1;
    bkey1_t bkey1;
    char b1;
    bkey1_range bkey1range;
    char a2;
    bkey2_t bkey2;
    char b2; 
    bkey2_range bkey2range;
    char a3;
    bkey3_t bkey3;
    char b3; 
    bkey3_range bkey3range;
     
    fprintf(stderr, "bkey_range=%d, elem_filter=%d\n",
            sizeof(bkey_range), sizeof(elem_filter));
    fprintf(stderr, "a1=(%x) bkey1_t=(%x,%d), b1=(%x) bkey1_range=(%x,%d)\n",
            &a1, &bkey1, sizeof(bkey1), &b1, &bkey1range, sizeof(bkey1range));
    fprintf(stderr, "a2=(%x) bkey2_t=(%x,%d), b2=(%x) bkey2_range=(%x,%d)\n",
            &a2, &bkey2, sizeof(bkey2), &b2, &bkey2range, sizeof(bkey2range));
    fprintf(stderr, "a3=(%x) bkey3_t=(%x,%d), b3=(%x) bkey3_range=(%x,%d)\n",
            &a3, &bkey3, sizeof(bkey3), &b3, &bkey3range, sizeof(bkey3range));
    fprintf(stderr, "msg_body1=%d, msg_body2=%d, msg_body3=%d\n",
            sizeof(msg_body1), sizeof(msg_body2), sizeof(msg_body3));
}
