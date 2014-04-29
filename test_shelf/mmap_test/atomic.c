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
#include "config.h"
#include "sched.h"

static inline int InterlockedIncrement( volatile int *val )
{
	register int rv;
	__asm__ __volatile__
	(
		"lock; xaddl %0,%1"
		: "=r" (rv)
		: "m" (*val), "0"(1)
		: "memory"
	);
	return rv;
}

static inline int InterlockedDecrement( volatile int *val )
{
	register int rv;
	__asm__ __volatile__
	(
		"lock; xaddl %0,%1"
		: "=r" (rv)
		: "m" (*val), "0"(-1)
		: "memory"
	);
	return rv;
}

void lock(MAPED_DATA *data)
{
	while ( 0 >= InterlockedDecrement(&(data->lock)) ) {
		InterlockedIncrement(&(data->lock));
		sched_yield();
	}
}

void unlock(MAPED_DATA *data) 
{
	InterlockedIncrement(&(data->lock));
}
