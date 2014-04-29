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
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct sockaddr_in addr_inet;
    struct hostent    *host;
    char   rcbuf[200] = "";

    if (!inet_aton(argv[1], &addr_inet.sin_addr)) {
        printf("Conversion Error \n");
        host = gethostbyname(argv[1]);
        if (!host) {
            printf("Invalid IP/hostname in arg: %s\n", argv[1]);
        } else {
            memcpy((char *) &addr_inet.sin_addr, host->h_addr_list[0], host->h_length);
            inet_ntop(AF_INET, &addr_inet.sin_addr, rcbuf, sizeof(rcbuf));
            printf("Converted IP: %s\n", rcbuf);
        }
    } else {
        printf("Unsinged long addr(network ordered) : %x \n", addr_inet.sin_addr.s_addr);
    }
    return 0;
}
