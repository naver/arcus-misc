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


static void sig_segv_handler(int signo, struct siginfo *si, void *ctx)
{
    mprotect((char*)(((ucontext_t*)ctx)->uc_mcontext.gregs[8]), 4096, PROT_READ|PROT_WRITE);
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

int main(void)
{
    const char str1[] = "string 1";
    const char str2[] = "string 2";
    int   size = (4096*10);
    char *anon;
 
    installSignalHandlers();

    anon = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if (anon == MAP_FAILED) {
        fprintf(stderr, "mmap fails\n");
        exit(errno);
    }

    strcpy(anon, str1);

    printf("anonymous page(%x): %s\n", anon, anon);

    if (mprotect(anon, 4096, PROT_NONE)) {
        perror("Couldn¡¯t mprotect");
        exit(errno);
    }

    strcpy(anon, str2);

    if (mprotect(anon, 4096, PROT_NONE)) {
        perror("Couldn¡¯t mprotect");
        exit(errno);
    }

    printf("anonymous page(%x): %s\n", anon, anon);

    munmap(anon, size);
    exit(0);
}

