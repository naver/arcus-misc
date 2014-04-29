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
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

int disk_open(const char *fname, int flags, int mode)
{
    int fd;
    do {
        if ((fd = open(fname, flags, mode)) == -1) {
            if (errno == EINTR) continue;
            fprintf(stderr, "disk open error (%d:%s)\n", errno, strerror(errno));   
            break;
        }
    } while(0);
    return fd;
}

int disk_close(int fd)
{
    do {
        if (close(fd) != 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "disk close error (%d:%s)\n", errno, strerror(errno));   
            return -1;
        }
    } while(0);
    return 0; 
}

int disk_unlink(const char *fname)
{
    if (unlink(fname) != 0) {
        fprintf(stderr, "disk unlink error (%d:%s)\n", errno, strerror(errno));   
        return -1; 
    }
    return 0;
}

int disk_page_write(int fd, char *page, size_t pgnum, size_t pgsiz)
{
    do {
        if (pwrite(fd, (void*)page, pgsiz, (off_t)(pgnum*pgsiz)) != pgsiz) {
            if (errno == EINTR) continue;
            fprintf(stderr, "disk page write error (%d:%s)\n", errno, strerror(errno));   
            return -1;
        }
    } while(0);
    return 0;
}

int disk_page_read(int fd, char *page, size_t pgnum, size_t pgsiz)
{
    ssize_t nread; 
    do {
        if ((nread = pread(fd, (void*)page, pgsiz, (off_t)(pgnum*pgsiz))) != pgsiz) {
            if (errno == EINTR) continue;
            fprintf(stderr, "disk page read error (%d:%s)\n", errno, strerror(errno));   
            return -1;
        }
    } while(0);
    return 0;
}

int disk_byte_write(int fd, void *buf, size_t count, off_t offset)
{
    do {
        if (pwrite(fd, buf, count, offset) != count) {
            if (errno == EINTR) continue;
            fprintf(stderr, "disk byte write error (%d:%s)\n", errno, strerror(errno));   
            return -1;
        }
    } while(0);
    return 0;
}

int disk_byte_read(int fd, void *buf, size_t count, off_t offset)
{
    ssize_t nread; 
    do {
        if ((nread = pread(fd, buf, count, offset)) != count) {
            if (errno == EINTR) continue;
            fprintf(stderr, "disk byte read error (%d:%s)\n", errno, strerror(errno));   
            return -1;
        }
    } while(0);
    return 0;
}

int disk_sync(int fd)
{
    if (fsync(fd) != 0) {
        fprintf(stderr, "disk sync erorr (%d:%s)\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

main()
{
    char  *file_name_1 = "./sample_data_full";
    char  *file_name_2 = "./sample_data_stride";
    char   buf_1[1024*16];
    char   buf_2[1024*16];
    int    i, fd;
    size_t page_size  = getpagesize(); 
    size_t page_count = 1024 * 10;

    long   difftime_us;
    struct timeval start_time;
    struct timeval end_time;

    disk_unlink(file_name_1);
    disk_unlink(file_name_2);

    for (i = 0; i < page_size; i++) {
        buf_1[i] = 'A';
    }
    for (i = 0; i < page_size; i++) {
        buf_1[i] = 'B';
    }

    fprintf(stderr, "full write test: start\n");
    gettimeofday(&start_time, 0);
    do {
        fd = disk_open(file_name_1, O_RDWR|O_CREAT, 0644);
        if (fd == -1) {
            fprintf(stderr, "file open error\n");
            break;
        } 

        for (i = 0; i < page_count; i += 1) {
            if (disk_page_write(fd, buf_1, i, page_size) != 0) {
                fprintf(stderr, "page write error\n");
                break;
            }
        }
        if (i < page_count) break;
        
        disk_sync(fd);
        disk_close(fd);
    } while(0); 
    gettimeofday(&end_time, 0);
    difftime_us = (end_time.tv_sec*1000000+end_time.tv_usec) -
                  (start_time.tv_sec*1000000 + start_time.tv_usec);
    fprintf(stderr, "full write test: end (time=%f)\n", difftime_us/1000000.0);

    fprintf(stderr, "stride write test: start\n");
    gettimeofday(&start_time, 0);
    do {
        fd = disk_open(file_name_2, O_RDWR|O_CREAT, 0644);
        if (fd == -1) {
            fprintf(stderr, "file open error\n");
            break;
        } 

        for (i = 0; i < page_count; i += 2) {
            if (disk_page_write(fd, buf_2, i, page_size) != 0) {
                fprintf(stderr, "page write error\n");
                break;
            }
        }
        if (i < page_count) break;
        
        disk_sync(fd);
        disk_close(fd);
    } while(0);
    gettimeofday(&end_time, 0);
    difftime_us = (end_time.tv_sec*1000000+end_time.tv_usec) -
                  (start_time.tv_sec*1000000 + start_time.tv_usec);
    fprintf(stderr, "stride write test: end (time=%f)\n", difftime_us/1000000.0);
}
