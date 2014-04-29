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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    char *prefix_name;
    int   prefix_leng, i;
    int   valid = 1;
 
    if (argc != 2) {
        fprintf(stderr, "usage) prefixname_test <prefixname>\n");
        exit(-1);
    }
    
    prefix_name = argv[1];
    prefix_leng = strlen(prefix_name);

    fprintf(stderr, "prefix_name = %s (leng=%d)\n", prefix_name, prefix_leng);

    for (i = 0; i < prefix_leng; i++) {
        if (! isalnum(prefix_name[i])) {
            valid = 0; break; 
        }
    }

    fprintf(stderr, "prefix_name = %s: %s\n", prefix_name, (valid ? "valid" : "invalid"));
}

