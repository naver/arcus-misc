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
#include <sys/wait.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CHILD_COUNT 10
int main(int argc, char *argv[])
{
    int i;
    int pfd[CHILD_COUNT][2];
    pid_t cpid[CHILD_COUNT];
    char buf;

    assert(argc == 2);

    for (i = 0; i < CHILD_COUNT; i++) {
        if (pipe(pfd[i]) == -1) { perror("pipe"); exit(EXIT_FAILURE); }
        cpid[i] = fork();
        if (cpid[i] == -1) { perror("fork"); exit(EXIT_FAILURE); }

        if (cpid[i] == 0) {    /* Child reads from pipe */
            close(pfd[i][1]);          /* Close unused write end */
            while (read(pfd[i][0], &buf, 1) > 0)
                write(STDOUT_FILENO, &buf, 1);
            write(STDOUT_FILENO, "\n", 1);
            close(pfd[i][0]);
            _exit(EXIT_SUCCESS);
        }
    else {            /* Parent writes argv[1] to pipe */
        close(pfd[i][0]);          /* Close unused read end */
#if 0
        write(pfd[1], argv[1], strlen(argv[1]));
        close(pfd[1]);          /* Reader will see EOF */
        wait(NULL);             /* Wait for child */
        exit(EXIT_SUCCESS);
#endif
    }
    }

    for (i = 0; i < CHILD_COUNT; i++)  {
        write(pfd[i][1], argv[1], strlen(argv[1]));
        close(pfd[i][1]);          /* Reader will see EOF */
    }
    for (i = 0; i < CHILD_COUNT; i++) {
        waitpid(cpid[i], NULL, 0);
    }
    exit(EXIT_SUCCESS);
}


