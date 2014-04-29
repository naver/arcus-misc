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
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>

int page_size;

static void sig_segv_handler(int signo, struct siginfo *si, void *ctx)
{
#if 0
    int i;
    for (i=0; i < 18; i++) {
        fprintf (stderr, "segfault : page=%x\n", (char*)(((ucontext_t*)ctx)->uc_mcontext.gregs[i]));
    } 
#endif
    mprotect((char*)(((ucontext_t*)ctx)->uc_mcontext.gregs[14]), page_size, PROT_READ|PROT_WRITE);
}

int installSignalHandlers(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);

    //sa.sa_flags = SA_ONESHOT | SA_SIGINFO;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sig_segv_handler;

    if (sigaction(SIGSEGV, &sa, NULL)) {
        fprintf (stderr, "sigaction failed, line %d, %d/%s\n", __LINE__, errno, strerror (errno));
        exit(1);
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int use_mprotect;
    const char str1[] = "string 1";
    const char str2[] = "string 2";
    size_t mmap_size;
    char  *anon[1024];
    char  *page;
    int i, j, k;
    int main_loop;
    int mmap_loop;
    int page_loop;
 
    if (argc != 3) {
        fprintf(stderr, "usage) mprotect_test mem_size(GB) use_mprotect(0 or 1)\n");
        exit(-1);
    }
    
    /* preparation */
    page_size = getpagesize();
    fprintf(stderr, "page_size=%d\n", page_size);

    mmap_size    = atoi(argv[1]); /* unit: GB */
    use_mprotect = atoi(argv[2]); /* 0 or 1 */
    fprintf(stderr, "command: mprotect_test mem_size=%d(GB) use_mprotect=%d\n", mmap_size, use_mprotect);
    
    mmap_size = mmap_size * 1024 * 1024; /* unit: MB */
    mmap_loop = 1024;
    page_loop = mmap_size / page_size;
    main_loop = (use_mprotect ? 3 : 100);

    fprintf(stderr, "main_loop=%d mmap_loop=%d, page_loop=%d\n", main_loop, mmap_loop, page_loop);

    /* register signal handler */
    installSignalHandlers();

    for (i = 0; i < mmap_loop; i++) {
        anon[i] = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
        if (anon[i] == MAP_FAILED) {
            fprintf(stderr, "mmap fails: errno=(%d|%s)\n", errno, strerror(errno));
            exit(errno);
        }
        if (use_mprotect) { 
            for (j = 0; j < page_loop; j++) {
                page = anon[i] + (j*page_size);
                if (mprotect(page, page_size, PROT_NONE)) {
                    perror("Couldn¡¯t mprotect"); exit(errno);
                }
            } 
        }
    }

    /* main loop */
    fprintf(stderr, "##### BEGIN MAIN LOOP\n");
    for (k = 0; k < main_loop; k++) {
      for (i = 0; i < mmap_loop; i++) {
        for (j = 0; j < page_loop; j++) {
            page = anon[i] + (j*page_size);
            //fprintf(stderr, "page=%x\n", page);

            strcpy(page, str1);
            if (strcmp(page, str1) != 0) {
                perror("strcmp error"); exit(errno);
            }
             
            strcpy(page, str2);
            if (strcmp(page, str2) != 0) {
                perror("strcmp error"); exit(errno);
            }

            if (use_mprotect) {
                if (madvise(page, page_size, MADV_REMOVE) != 0) {
                    perror("madvise error"); exit(errno);
                }
                /*****
                if (madvise(page, page_size, MADV_DONTNEED) != 0) {
                    perror("madvise error"); exit(errno);
                }
                *****/
                if (mprotect(page, page_size, PROT_NONE)) {
                    perror("Couldn¡¯t mprotect"); exit(errno);
                }
            }
        }
        if ((i % 50) == (50-1))
            fprintf(stderr, "\tmmap_loop = %d\n", i+1);
      }
      fprintf(stderr, "main_loop = %d\n", k+1);
    }
    fprintf(stderr, "##### END MAIN LOOP\n");

    /* finalize */
    for (i = 0; i < mmap_loop; i++) {
        munmap(anon[i], mmap_size);
    } 
    exit(0);
}

