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
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <time.h>
#if 0
#include <execinfo.h>
#include <limits.h>
#endif

#define arrsizeof(x) (sizeof(x)/sizeof(x[0]))
//#define HANDLE_VMM 

#ifdef HANDLE_VMM
#else
#if 0
char *getProcessName(void)
{
    char buffer[128];
    FILE *fp;
    char *s;

    sprintf (buffer, "/proc/%d/stat", getpid());
    if ((fp = fopen (buffer, "r"))) {
        if (fgets (buffer, sizeof(buffer), fp)) {
            if ((s = index(buffer, ')'))) {
                *s = '\0';
                if ((s = index(buffer, '('))) return ++s;
            }
        }
    }
    return NULL;
}
#endif

static void sighandlerPrint(FILE *fp, int signo, int code, ucontext_t *context, void *bt [], int bt_size)
{
  char *processName;
  time_t ttime = time(NULL);

  fprintf(fp, "%s", ctime (&ttime));
  //fprintf(fp, "PID=%d (%s)\n", getpid(), (processName = getProcessName()) ? processName : "unknown");
  fprintf(fp, "signo=%d/%s\n", signo, strsignal(signo));
  fprintf(fp, "code=%d (not always applicable)\n", code);
  fprintf(fp, "\nContext: 0x%08lx\n", (unsigned long) context);
#if 1
  fprintf(fp, "    gs: 0x%08x   fs: 0x%08x   es: 0x%08x   ds: 0x%08x\n"
               "   edi: 0x%08x  esi: 0x%08x  ebp: 0x%08x  esp: 0x%08x\n"
               "   ebx: 0x%08x  edx: 0x%08x  ecx: 0x%08x  eax: 0x%08x\n"
               "  trap:   %8u  err: 0x%08x  eip: 0x%08x   cs: 0x%08x\n"
               "  flag: 0x%08x   sp: 0x%08x   ss: 0x%08x\n",
               context->uc_mcontext.gregs[0], context->uc_mcontext.gregs[1],
               context->uc_mcontext.gregs[2], context->uc_mcontext.gregs[3],
               context->uc_mcontext.gregs[4], context->uc_mcontext.gregs[5],
               context->uc_mcontext.gregs[6], context->uc_mcontext.gregs[7],
               context->uc_mcontext.gregs[8], context->uc_mcontext.gregs[9],
               context->uc_mcontext.gregs[10], context->uc_mcontext.gregs[11],
               context->uc_mcontext.gregs[12], context->uc_mcontext.gregs[13],
               context->uc_mcontext.gregs[14], context->uc_mcontext.gregs[15],
               context->uc_mcontext.gregs[16], context->uc_mcontext.gregs[17],
               context->uc_mcontext.gregs[18]
         );
#else
  fprintf(fp, "    gs: 0x%08x   fs: 0x%08x   es: 0x%08x   ds: 0x%08x\n"
               "   edi: 0x%08x  esi: 0x%08x  ebp: 0x%08x  esp: 0x%08x\n"
               "   ebx: 0x%08x  edx: 0x%08x  ecx: 0x%08x  eax: 0x%08x\n"
               "  trap:   %8u  err: 0x%08x  eip: 0x%08x   cs: 0x%08x\n"
               "  flag: 0x%08x   sp: 0x%08x   ss: 0x%08x  cr2: 0x%08lx\n",
               context->uc_mcontext.gregs[REG_GS], context->uc_mcontext.gregs [REG_FS], context->uc_mcontext.gregs [REG_ES],  context->uc_mcontext.gregs [REG_DS],
               context->uc_mcontext.gregs [REG_EDI],    context->uc_mcontext.gregs [REG_ESI],  context->uc_mcontext.gregs [REG_EBP], context->uc_mcontext.gregs [REG_ESP],
               context->uc_mcontext.gregs [REG_EBX],    context->uc_mcontext.gregs [REG_EDX],  context->uc_mcontext.gregs [REG_ECX], context->uc_mcontext.gregs [REG_EAX],
               context->uc_mcontext.gregs [REG_TRAPNO], context->uc_mcontext.gregs [REG_ERR],  context->uc_mcontext.gregs [REG_EIP], context->uc_mcontext.gregs [REG_CS],
               context->uc_mcontext.gregs [REG_EFL],    context->uc_mcontext.gregs [REG_UESP], context->uc_mcontext.gregs [REG_SS],  context->uc_mcontext.cr2
  );
#endif

  fprintf(fp, "\n%d elements in backtrace\n", bt_size);
  fflush(fp);

  backtrace_symbols_fd(bt, bt_size, fileno(fp));
}
#endif


static void sighandlerSEGV(int signo, struct siginfo *si, void *ctx)
{
#ifdef HANDLE_VMM
    mprotect((char*)(((ucontext_t*)ctx)->uc_mcontext.gregs[8]), 4096, PROT_READ|PROT_WRITE);
#else
    void *bt[128];
    int bt_size;

    bt_size = backtrace(bt, arrsizeof(bt));
    sighandlerPrint(stderr, signo, si->si_code, (ucontext_t *)ctx, bt, bt_size);
#endif
}


int installSignalHandlers(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);

    sa.sa_flags = SA_ONESHOT | SA_SIGINFO;
    //sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sighandlerSEGV;

    if (sigaction(SIGSEGV, &sa, NULL)) {
        fprintf (stderr, "sigaction failed, line %d, %d/%s\n", __LINE__, errno, strerror (errno));
        exit(1);
    }
    return 1;
}

int
main(void)
{
    const char str[] = "string 1";
    int   size = (4096*10);
    char *anon;
 
    installSignalHandlers();

    anon = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    if (anon == MAP_FAILED) {
        fprintf(stderr, "mmap fails\n");
        exit(errno);
    }
 
    strcpy(anon, str);
 
    printf("anonymous page(%x): %s\n", anon, anon);

    if (mprotect(anon, 4096, PROT_NONE)) {
        perror("Couldn¡¯t mprotect");
        exit(errno);
    }

    printf("anonymous page(%x): %s\n", anon, anon);
    strcpy(anon, str);

    munmap(anon, size);
    exit(0);
}

