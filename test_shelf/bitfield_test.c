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
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

//#define FIRST_TYPE 1

#ifdef FIRST_TYPE
typedef struct _pageid {
    uint64_t pfxid : 14;
    uint64_t fltyp : 2;
    uint64_t flnum : 16;
    uint64_t pgnum : 20;
    uint64_t dummy : 12;
} PageID;

typedef struct _itemid {
    uint64_t pfxid : 14;
    uint64_t fltyp : 2;
    uint64_t flnum : 16;
    uint64_t pgnum : 20;
    uint64_t sltid : 12;
} ItemID;

static inline uint32_t do_buf_pgid_hash(const PageID pgid)
{
    return (((uint32_t)pgid.pfxid << 18 | (uint32_t)pgid.fltyp << 16 | (uint32_t)pgid.flnum | (uint32_t)pgid.pgnum << 12 | (uint32_t)pgid.dummy) % 65536);
}
#else
/* page id structure */
typedef struct pageid {
    uint16_t    pfxid;   /* prefix id */
    uint16_t    flnum;   /* file number : file type (2bits) + file number (14bits) */
    uint32_t    pgnum;   /* page number : (0, 1, ...) */
} PageID;

/* item id structure */
typedef struct itemid {
    uint16_t    pfxid;   /* prefix id */
    uint16_t    flnum;   /* file number : file type (2bits) + file number (14bits) */
    uint32_t    itnum;   /* item number : page number(upper 20bits) + slot number(lower 12bits) */
} ItemID;

static inline uint32_t do_buf_pgid_hash(const PageID pgid)
{
    return (((uint32_t)pgid.pfxid << 16 | (uint32_t)pgid.flnum | pgid.pgnum) % 65536);
}

#define FLTYPE_FROM_PAGEID(pid) (((pid)->flnum & 0xC000) >> 14)
#define FLNUMB_FROM_PAGEID(pid) ((pid)->flnum & 0x3FFF)
#define PGNUMB_FROM_PAGEID(pid) (((pid)->pgnum & 0xFFFFF000) >> 12)
#define FLTYPE_FROM_ITEMID(iid) (((iid)->flnum & 0xC000) >> 14)
#define FLNUMB_FROM_ITEMID(iid) ((iid)->flnum & 0x3FFF)
#define PGNUMB_FROM_ITEMID(iid) (((iid)->itnum & 0xFFFFF000) >> 12)
#define SLOTID_FROM_ITEMID(iid) ((iid)->itnum & 0x00000FFF)

#if 0
#define PAGEID_FROM_ITEMID(iid, pid) { \
   (pid)->pfxid = (iid)->pfxid;\
   (pid)->flnum = (iid)->flnum;\
   (pid)->pgnum = (iid)->itnum & 0xFFFFF000; \
}
#else
#define SQUALL_SLOT_NUM_BITS 12
#define PAGEID_FROM_ITEMID(iid, pid) \
        do { (pid)->pfxid = (iid)->pfxid; (pid)->flnum = (iid)->flnum; \
             (pid)->pgnum = ((iid)->itnum >> SQUALL_SLOT_NUM_BITS); } while(0)
#endif
#endif

int main(void)
{
    PageID   pgid;
    ItemID   itmid;
    uint32_t hval;

#ifdef FIRST_TYPE
    pgid.pfxid = 2;
    pgid.fltyp = 0;
    pgid.flnum = 1;
    pgid.pgnum = 32;
    pgid.dummy = 0;

    itmid.pfxid = 2;
    itmid.fltyp = 0;
    itmid.flnum = 1;
    itmid.pgnum = 32;
    itmid.sltid = 4;

    fprintf(stderr, "sizeof(PageID) = %d\n", sizeof(PageID));
    fprintf(stderr, "pgid.pfxid=%d\n", pgid.pfxid);
    fprintf(stderr, "pgid.fltyp=%d\n", pgid.fltyp);
    fprintf(stderr, "pgid.flnum=%d\n", pgid.flnum);
    fprintf(stderr, "pgid.pgnum=%d\n", pgid.pgnum);
    char *ptr = (char*)&pgid;
    for (int i = 0; i < sizeof(PageID); i++) {
        fprintf(stderr, "pgid=%x\n", *ptr);
        ptr++;
    }
    hval = do_buf_pgid_hash(pgid);
    fprintf(stderr, "hval=%d\n", hval);

    fprintf(stderr, "sizeof(ItemID) = %d\n", sizeof(ItemID));
    fprintf(stderr, "itmid=%x\n", *(uint64_t*)&itmid);
    fprintf(stderr, "itmid.pfxid=%d\n", itmid.pfxid);
    fprintf(stderr, "itmid.fltyp=%d\n", itmid.fltyp);
    fprintf(stderr, "itmid.flnum=%d\n", itmid.flnum);
    fprintf(stderr, "itmid.pgnum=%d\n", itmid.pgnum);
    fprintf(stderr, "itmid.sltid=%d\n", itmid.sltid);
#else
    pgid.pfxid = 1;
    pgid.flnum = (uint16_t) 1 << 14 | (uint16_t)10;
    pgid.pgnum = (uint32_t)100;

    itmid.pfxid = 2;
    itmid.flnum = (uint16_t) 2 << 14 | (uint16_t)20;
    itmid.itnum = (uint32_t)200 << 12 | (uint32_t)20;

    fprintf(stderr, "sizeof(PageID) = %d\n", sizeof(PageID));
    fprintf(stderr, "pgid.pfxid=%d\n", pgid.pfxid);
    fprintf(stderr, "pgid.fltyp=%d\n", FLTYPE_FROM_PAGEID(&pgid));
    fprintf(stderr, "pgid.flnum=%d\n", FLNUMB_FROM_PAGEID(&pgid));
    fprintf(stderr, "pgid.pgnum=%d\n", pgid.pgnum);

    hval = do_buf_pgid_hash(pgid);
    fprintf(stderr, "hval=%d\n", hval);

    fprintf(stderr, "sizeof(ItemID) = %d\n", sizeof(ItemID));
    fprintf(stderr, "itmid.pfxid=%d\n", itmid.pfxid);
    fprintf(stderr, "itmid.fltyp=%d\n", FLTYPE_FROM_ITEMID(&itmid));
    fprintf(stderr, "itmid.flnum=%d\n", FLNUMB_FROM_ITEMID(&itmid));
    fprintf(stderr, "itmid.pgnum=%d\n", PGNUMB_FROM_ITEMID(&itmid));
    fprintf(stderr, "itmid.sltid=%d\n", SLOTID_FROM_ITEMID(&itmid));

    PAGEID_FROM_ITEMID(&itmid, &pgid);

    fprintf(stderr, "sizeof(PageID) = %d\n", sizeof(PageID));
    fprintf(stderr, "pgid.pfxid=%d\n", pgid.pfxid);
    fprintf(stderr, "pgid.fltyp=%d\n", FLTYPE_FROM_PAGEID(&pgid));
    fprintf(stderr, "pgid.flnum=%d\n", FLNUMB_FROM_PAGEID(&pgid));
    fprintf(stderr, "pgid.pgnum=%d\n", pgid.pgnum);

    hval = do_buf_pgid_hash(pgid);
    fprintf(stderr, "hval=%d\n", hval);
#endif

    exit(0);
}

