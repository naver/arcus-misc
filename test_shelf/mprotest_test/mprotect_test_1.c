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
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>

#include <limits.h>    /* for PAGESIZE */
//#ifndef PAGESIZE
//#define PAGESIZE 4096
//#endif

int
main(void)
{
    char *p;
    char c;

    /* Allocate a buffer; it will have the default
       protection of PROT_READ|PROT_WRITE. */
    p = malloc(1024+PAGESIZE-1);
    if (!p) {
        perror("Couldn¡¯t malloc(1024)");
        exit(errno);
    }

    /* Align to a multiple of PAGESIZE, assumed to be a power of two */
    fprintf(stderr, "before align: p=%x (PAGESIZE=%u)\n", p, PAGESIZE);
    //p = (char *)(((int) p + PAGESIZE-1) & ~(PAGESIZE-1));
    p = (char *)(((long) p + PAGESIZE-1) & ~(PAGESIZE-1));
    fprintf(stderr, "after align: p=%x (PAGESIZE=%u)\n", p, PAGESIZE);

    c = p[666];         /* Read; ok */
    p[666] = 42;        /* Write; ok */

    /* Mark the buffer read-only. */
    if (mprotect(p, 1024, PROT_READ)) {
        perror("Couldn¡¯t mprotect");
        exit(errno);
    }

    c = p[666];         /* Read; ok */
    p[666] = 42;        /* Write; program dies on SIGSEGV */

    exit(0);
}

