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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "config.h"

#define LOOP_CNT 10000000
#define CHILD_CNT 10

void printMapedData(MAPED_DATA *data) 
{
	lock(data);
	printf("%ld, %s\n", data->ver, data->str);
	unlock(data);
}

void runChild(MAPED_DATA *data)
{
	long i;
	for (i=0;i<LOOP_CNT;++i) {
		lock(data);
		++data->ver;
		unlock(data);
	}
	exit(0);
}

void forkChild(MAPED_DATA *data)
{
	pid_t pid = fork();
	if (pid == 0) {
		runChild(data);
	}
	else if (pid <0) {
		perror("fork :");
		exit(-1);
	}
	else {
		printf("Create Child : %d\n", pid);
	}
}

int main(void)
{
	int i;
	int fd = open(MAPED_FILE, O_RDWR | O_TRUNC | O_CREAT );
    //int fd = open(MAPED_FILE, O_RDWR | O_TRUNC | O_CLOEXEC | O_CREAT );

	if (fd < 0) {
		perror("open :");
		exit(-1);
	}

	if (lseek(fd, MMAP_LENGTH, SEEK_SET) < 0) {
		perror("lseek :");
		exit(-1);
	}

	if (write(fd, "\0", 1) != 1) {
		perror("lseek :");
		exit(-1);
	}

	if (unlink(MAPED_FILE) < 0) {
		perror("lseek :");
		exit(-1);
	}

	void *addr = mmap(NULL, MMAP_LENGTH, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

	if (addr == MAP_FAILED) {
		perror("mmap :");
		exit(-1);
	}

	MAPED_DATA *data = (MAPED_DATA *)addr;
	data->lock = 1;
	data->ver = 0;
	strcpy(data->str, "Hello, World");

	for (i=0;i<CHILD_CNT;++i) {
		forkChild(data);
	}

	while (1) {
		printMapedData(data);
		lock(data);
		sleep(1);
		if (data->ver == LOOP_CNT * CHILD_CNT) {
			unlock(data);
			break;
		}
		unlock(data);
	}
}
