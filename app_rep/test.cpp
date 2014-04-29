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
#include "arcus.hpp"

#include <iostream>
#include <vector>
#include <pthread.h>

#define THREAD_COUNT 10
#define MAX_THREAD_COUNT 10
#define HISTOGRAM_TICK 100000

#define STOP_WATCH_INIT              \
            struct timeval s, e;     \
            double elapsed_ms = 0.0;

#define STOP_WATCH(func) {                                                                              \
            gettimeofday(&s, NULL);                                                                     \
            func;                                                                                       \
            gettimeofday(&e, NULL);                                                                     \
            elapsed_ms = ((e.tv_sec - s.tv_sec) * 1000.0) + ((e.tv_usec - s.tv_usec) / 1000.0);         \
            pthread_mutex_lock(&lock); \
            ++histogram[(long)(elapsed_ms/10)*10];                                                      \
            if (++total_requests % HISTOGRAM_TICK == 0) {                                               \
                struct timeval timeval;                                                                 \
                struct tm *current_time;                                                                \
                char time_string[256];                                                                  \
                gettimeofday(&timeval, 0);                                                              \
                current_time = localtime(&timeval.tv_sec);                                              \
                snprintf(time_string, sizeof(time_string), "%4d-%02d-%02d %02d:%02d:%02d",              \
                         current_time->tm_year + 1900, current_time->tm_mon + 1, current_time->tm_mday, \
                         current_time->tm_hour, current_time->tm_min, current_time->tm_sec);            \
                fprintf(stdout, "%s\n", time_string);                                                   \
                for (int i=0; i<500; i+=10) {                                                           \
                    if (histogram[i] > 1) {                                                             \
                        fprintf(stdout, "[histogram] %3d ms : %d\n", i, histogram[i]);                  \
                    }                                                                                   \
                }                                                                                       \
            }                                                                                           \
            pthread_mutex_unlock(&lock); \
        }

using namespace arcus;

static volatile bool running = true;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

long total_requests = 0;
long histogram[100000] = { 0, };

static void sig_handler(int sig)
{
    running = false;
}

static void *do_test(void *ctx)
{
    ::base_client *client = (::base_client*)ctx;
    memcached_return_t rc;

    long valuelen;
    long curcount = 0;
    long maxcount = 1000000000;
    char countbuf[32];

    std::stringstream ss;
    ss << "joontest:" << pthread_self() << "-";

    std::string key;
    std::string data;

    valuelen = 256;
    data = "";
    data.append(valuelen, 'a');

    STOP_WATCH_INIT

    while (running) {
        if ((++curcount) >= maxcount) break;

        key = ss.str();
        sprintf(countbuf, "%010d", curcount);
        key.append(countbuf);

        STOP_WATCH(client->kv_set(key, data, &rc));

        if ((curcount % 1000) == 0)
        {
            long randcnt, i;
            for (i = 0; i < 1000; i++) {
                randcnt = random() % curcount; 
                key = ss.str();
                sprintf(countbuf, "%010d", randcnt);
                key.append(countbuf);
                STOP_WATCH(client->touch(key, 87400, &rc));
            }  
        }
    }
}

#if 0 // version 1
static void *do_test(void *ctx)
{
    ::base_client *client = (::base_client*)ctx;
    memcached_return_t rc;

    long valuelen;
    long curcount = 0;
    long maxcount = 1000000000;
    char countbuf[32];

    std::stringstream ss;
    ss << "joontest:" << pthread_self() << "-";

    std::string key;
    std::string data;

    STOP_WATCH_INIT

    while (running) {
        if ((++curcount) >= maxcount) break;

        key = ss.str();
        sprintf(countbuf, "%d", curcount);
        key.append(countbuf);

        valuelen = 1 + random() % 1024;
        data = "";
        data.append(valuelen, 'a');

        STOP_WATCH(client->kv_set(key, data, &rc));
    }
}
#endif

#if 0 // Original Function 
static void *do_test(void *ctx)
{
    ::base_client *client = (::base_client*)ctx;
    memcached_return_t rc;

    std::stringstream ss;
    ss << "pthread:" << pthread_self() << "test";

    std::string key = ss.str();
    std::string byte20 = "1234567901234567890";

    STOP_WATCH_INIT

    while (running) {
        STOP_WATCH(client->kv_set(key, byte20, &rc));
    }
}
#endif

int main(int argc, char *argv[])
{
    int thread_count = THREAD_COUNT;
    if (argc == 2) {
        thread_count = atoi(argv[1]);
    }

    ::base_client client;
    client.init("localhost:2181", "test", 1, thread_count, 500);

    signal(SIGINT, sig_handler);

    pthread_t workers[MAX_THREAD_COUNT];

    for (int i=0; i<thread_count; i++) {
        pthread_create(&workers[i], NULL, do_test, &client);
    }

    for (int i=0; i<thread_count; i++) {
        pthread_join(workers[i], NULL);
    }

    return EXIT_SUCCESS;
}
