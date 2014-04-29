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
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define MMAP_LENGTH 8192
#define MAPED_FILE ".map"

typedef struct {
	int lock;
	long ver;
	char str[255];
} MAPED_DATA;

void lock(MAPED_DATA *data);
void unlock(MAPED_DATA *data);
void printMapedData(MAPED_DATA *data);

#endif /* __CONFIG_H__ */
