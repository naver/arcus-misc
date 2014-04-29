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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define MAX_BKEY_LENG 32
#define MAX_EFLAG_LENG 32

static int safe_strtoull(const char *str, uint64_t *out) {
    assert(out != NULL);
    errno = 0;
    *out = 0;
    char *endptr;
    unsigned long long ull = strtoull(str, &endptr, 10);
    if (errno == ERANGE)
        return 0;
    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long long) ull < 0) {
            /* only check for negative signs in the uncommon case when
             * the unsigned number is so big that it's negative as a
             * signed number. */
            if (strchr(str, '-') != NULL) {
                return 0;
            }
        }
        *out = ull;
        return 0;
    }
    return 1;
}

static int safe_strtohexa(const char *str, unsigned char *bin, const int size)
{
    assert(bin != NULL);
    int i, slen = strlen(str);
    char ch1, ch2;

    if (slen <= 0 || slen > (2*size) || (slen%2) != 0) {
        return 0;
    }
    for (i=0; i < (slen/2); i++) {
        ch1 = str[2*i]; ch2 = str[2*i+1];
        if      (ch1 >= '0' && ch1 <= '9') bin[i] = (ch1 - '0');
        else if (ch1 >= 'A' && ch1 <= 'F') bin[i] = (ch1 - 'A' + 10);
        else if (ch1 >= 'a' && ch1 <= 'f') bin[i] = (ch1 - 'a' + 10);
        else return 0;
        if      (ch2 >= '0' && ch2 <= '9') bin[i] = (bin[i] << 4) + (ch2 - '0');
        else if (ch2 >= 'A' && ch2 <= 'F') bin[i] = (bin[i] << 4) + (ch2 - 'A' + 10);
        else if (ch2 >= 'a' && ch2 <= 'f') bin[i] = (bin[i] << 4) + (ch2 - 'a' + 10);
        else return 0;
    }
    return 1;
}

static void safe_hexatostr(const unsigned char *bin, const int size, char *str) {
    assert(str != NULL);
    int i;

    for (i=0; i < size; i++) {
        str[(i*2)  ] = (bin[i] & 0xF0) >> 4;
        str[(i*2)+1] = (bin[i] & 0x0F);
        if (str[(i*2)  ] < 10) str[(i*2)  ] += ('0');
        else                   str[(i*2)  ] += ('A' - 10);
        if (str[(i*2)+1] < 10) str[(i*2)+1] += ('0');
        else                   str[(i*2)+1] += ('A' - 10);
    }
    str[size*2] = '\0';
}

static inline int get_bkey_from_str(const char *str, unsigned char *bkey)
{
    if (strncmp(str, "0x", 2) == 0) { /* hexadeciaml bkey */
        if (safe_strtohexa(str+2, bkey, MAX_BKEY_LENG)) {
            return (strlen(str+2)/2);
        }
    } else { /* 64 bit unsigned integer */
        if (safe_strtoull(str, (uint64_t*)bkey)) {
            return 0;
        }
    }
    return -1;
}

static inline void BINARY_DIFF(const unsigned char *v1, const unsigned char *v2, const int nv,
                               unsigned char *r)
{
    assert(nv > 0);
    int i, j;

    for (i = (nv-1); i >= 0; ) {
        if (v1[i] >= v2[i]) {
            r[i] = v1[i] - v2[i];
            i -= 1;
        } else {
            r[i] = 0xFF - v2[i] + v1[i] + 1;
            for (j = (i-1); j >= 0; j--) {
               if (v1[j] > v2[j]) {
                   r[j] = v1[j] - 1 - v2[j];
                   break;
               } else {
                   r[j] = 0xFF - v2[j] + v1[j];
               }
            }
            assert(j >= 0);
            i = j-1;
        }
    }
}

main()
{
    char str1[128];
    char str2[128];
    char strd[128];
    unsigned char bkey1[MAX_BKEY_LENG];
    unsigned char bkey2[MAX_BKEY_LENG];
    unsigned char bkeyd[MAX_BKEY_LENG];
    int nbkey1, nbkey2;
    
    sprintf(str1, "%s", "0x2780FA1D");
    //sprintf(str2, "%s", "0x2780A01B");
    sprintf(str2, "%s", "0x1784FA0F");

    fprintf(stderr, "\tstr1=%s\n", str1);
    fprintf(stderr, "\tstr2=%s\n", str2);

    nbkey1 = get_bkey_from_str(str1, bkey1);
    if (nbkey1 == -1) {
        fprintf(stderr, "get_bkey_from_str error");
        exit(1);
    }
    nbkey2 = get_bkey_from_str(str2, bkey2);
    if (nbkey2 == -1) {
        fprintf(stderr, "get_bkey_from_str error");
        exit(1);
    }
    if (nbkey1 != nbkey2) {
        fprintf(stderr, "nbkey is diffrent.");
        exit(1);
    } 

    BINARY_DIFF(bkey1, bkey2, nbkey1, bkeyd);

    memcpy(strd, "0x", 2);
    safe_hexatostr(bkeyd, nbkey1, strd+2);
    
    fprintf(stderr, "\tstrd=%s\n", strd);
 
}
