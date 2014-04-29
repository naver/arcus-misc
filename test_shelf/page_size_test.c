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

int
main(void)
{
    long page_size_tmp = sysconf(_SC_PAGESIZE);
    int  page_size = getpagesize();
    char *pgptr1;
    char *pgptr2;
    long limit;

    fprintf(stderr, "page_size (by sysconf(_SC_PAGESIZE)) = %ld\n", page_size_tmp);
    fprintf(stderr, "page_size (by getpagesize) = %ld\n", page_size);

    pgptr1 = malloc(10 * page_size); 
    if (pgptr1 == NULL) {
        fprintf(stderr, "malloc fail\n");
    }
    if (posix_memalign((void**)&pgptr2, (size_t)page_size, (10*page_size)) != 0) {
        fprintf(stderr, "posix_memalign fail\n");
    }

    fprintf(stderr, "pgptr (by malloc) = %x\n", pgptr1);
    fprintf(stderr, "pgptr (by posiz_memalsign) = %x\n", pgptr2);

    free(pgptr1);
    free(pgptr2);

    limit = pathconf("./redis-1.2.2.tar", _PC_REC_XFER_ALIGN);
    fprintf(stderr, "limit by pathconf(path, _PC_REC_XFER_ALIGN) = %ld\n", limit);

    exit(0);
}

