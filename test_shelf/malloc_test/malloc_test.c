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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <inttypes.h>
#ifdef USE_TCMALLOC
#include <google/tcmalloc.h>
#include <google/malloc_extension_c.h>
#else
#include <malloc.h>
#endif

#define ITEM_MAX_SIZE (1024*1024) 

typedef struct _item {
    struct _item *prev;
    struct _item *next;
    int      nbytes;
    char     value[1]; 
} item_t;

item_t *item_head = NULL;
item_t *item_tail = NULL;
unsigned int item_count = 0;

unsigned int mem_limit = 0;
unsigned int mem_alloc = 0;

char sample_data[1024];

#ifdef USE_TCMALLOC
#define malloc(size) tc_malloc(size)
#define free(ptr)    tc_free(ptr)
#define TCMALLOC_PROPERTY_COUNT 7
char *prop_name[TCMALLOC_PROPERTY_COUNT] = { "generic.current_allocated_bytes",
                                             "generic.heap_size",
                                             "tcmalloc.pageheap_free_bytes",
                                             "tcmalloc.pageheap_unmapped_bytes",
                                             "tcmalloc.slack_bytes",
                                             "tcmalloc.max_total_thread_cache_bytes",
                                             "tcmalloc.current_total_thread_cache_bytes" };
size_t heap_size;
#endif

#if defined(ENABLE_INTERNAL_SMALL_MEMMGR)
typedef struct _mem_chunk {
    uint32_t total_item_count;
    uint32_t free_item_count;
    struct _mem_chunk *prev;
    struct _mem_chunk *next;
    struct _mem_chunk *free_prev;
    struct _mem_chunk *free_next;
    void              *free_item;
} mem_chunk_t;

typedef struct _mem_anchor {
  uint32_t total_chunk_count;
  uint32_t free_chunk_count;
  mem_chunk_t *chunk_list;
  mem_chunk_t *chunk_free_head;
  mem_chunk_t *chunk_free_tail;
} mem_anchor_t;

#define MEM_CHUNK_SIZE  (8*1024)
#define MEM_LIMIT_SIZE  (3*1024)
#define MEM_CLASS_COUNT (MEM_LIMIT_SIZE/8)

static bool mem_anchor_init_flag = false;
static mem_anchor_t mem_anchor[MEM_CLASS_COUNT];
#endif

#if defined(ENABLE_INTERNAL_SMALL_MEMMGR)
static void do_mem_anchor_init() {
    int i;
    for (i = 0; i < MEM_CLASS_COUNT; i++) {
        mem_anchor[i].chunk_list        = NULL;
        mem_anchor[i].chunk_free_head   = NULL;
        mem_anchor[i].chunk_free_tail   = NULL;
        mem_anchor[i].total_chunk_count = 0;
        mem_anchor[i].free_chunk_count  = 0;
    }
}

static mem_chunk_t *do_mem_chunk_alloc(int memid) {
    mem_chunk_t *chunk = (mem_chunk_t *)malloc(MEM_CHUNK_SIZE);
    if (chunk != NULL) {
        int i;
        int item_size = (memid + 1) * 8;
        void    **item;
        uint16_t *pidx; /* physical item index */

        chunk->total_item_count = (MEM_CHUNK_SIZE - sizeof(mem_chunk_t)) / item_size;
        chunk->free_item_count  = chunk->total_item_count;
        for (i = 0; i < chunk->total_item_count; i++) {
             item  = (void**)((char *)chunk + sizeof(mem_chunk_t) + (i*item_size));
             *item = ((i == (chunk->total_item_count-1)) ? (void*)NULL
                                                         : (void*)((char*)item + item_size));
             pidx  = (uint16_t *)((char*)item + item_size - sizeof(uint16_t));
             *pidx = i;
        }
        chunk->free_item = (void *)((char *)chunk + sizeof(mem_chunk_t));
    }
    return chunk;
}

static void do_mem_chunk_link(int memid, mem_chunk_t *chunk, bool chunk_list_flag) {
    if (chunk_list_flag == true) {
        chunk->prev = NULL;
        chunk->next = mem_anchor[memid].chunk_list;
        if (chunk->next != NULL) chunk->next->prev = chunk;
        mem_anchor[memid].chunk_list = chunk;
        mem_anchor[memid].total_chunk_count++;
    }

    chunk->free_prev = mem_anchor[memid].chunk_free_tail;
    chunk->free_next = NULL;
    if (chunk->free_prev != NULL) chunk->free_prev->free_next = chunk;
    mem_anchor[memid].chunk_free_tail = chunk;
    if (mem_anchor[memid].chunk_free_head == NULL)
        mem_anchor[memid].chunk_free_head = chunk;
    mem_anchor[memid].free_chunk_count++;
}

static void do_mem_chunk_unlink(int memid, mem_chunk_t *chunk, bool chunk_list_flag) {
    if (chunk->free_next != NULL)
        chunk->free_next->free_prev = chunk->free_prev;
    if (chunk->free_prev != NULL)
        chunk->free_prev->free_next = chunk->free_next;
    if (mem_anchor[memid].chunk_free_head == chunk)
        mem_anchor[memid].chunk_free_head = chunk->free_next;
    if (mem_anchor[memid].chunk_free_tail == chunk)
        mem_anchor[memid].chunk_free_tail = chunk->free_prev;
    chunk->free_prev = chunk->free_next = NULL;
    mem_anchor[memid].free_chunk_count--;

    if (chunk_list_flag == true) {
        if (chunk->next != NULL)
            chunk->next->prev = chunk->prev;
        if (chunk->prev != NULL)
            chunk->prev->next = chunk->next;
        if (mem_anchor[memid].chunk_list == chunk)
            mem_anchor[memid].chunk_list = chunk->next;
        chunk->prev = chunk->next = NULL;
        mem_anchor[memid].total_chunk_count--;
    }
}
#endif

static int init_malloc_test() {
    int i;

    /* initialize sample data */
    for (i = 0; i < 1024; i++) {
        sample_data[i] = 'A' + (i % 26);
    }
#if defined(USE_TCMALLOC) && defined(LIMIT_HEAP_SIZE)
    /***** set heap_size : impossible ***********************
    if (!MallocExtension_SetNumericProperty("generic.heap_size", mem_limit)) {
        fprintf(stderr, "tcmalloc set propery generic.heapsize error\n");
        return -1;
    }
    *********************************************************/
    if (!MallocExtension_GetNumericProperty("generic.heap_size", &heap_size)) {
        fprintf(stderr, "tcmalloc get propery generic.heapsize error\n");
        return -1;
    }
#endif
#if defined(ENABLE_INTERNAL_SMALL_MEMMGR)
    if (mem_anchor_init_flag == false) {
        do_mem_anchor_init();
        mem_anchor_init_flag = true;
    }
#endif
    return 0;
}

static int get_data_len(int item_count) {
    int data_len;
    /*** Method 1: 
           24 <= item_size < 4096 : 90% 
         4096 <= item_size <  1MB : 10%
    *********************************
    int remain = item_count % 100; 
    if (remain < 90) {
        data_len = random() % (4096-sizeof(item_t));
    } else {
        data_len = 4096 + (random() % (ITEM_MAX_SIZE-4095-sizeof(item_t)));
    }
    *********************************/
    /*** Method 2: 
           24 <= item_size <  128 : 90% 
          128 <= item_size < 1024 :  7% 
         1024 <= item_size < 4096 :  2% 
         4096 <= item_size <  1MB :  1%
    *********************************
    int remain = item_count % 100; 
    if (remain < 90) {
        data_len = random() % (128-sizeof(item_t));
    } else if (remain < 97) {
        data_len =  128 + (random() % (1024-127-sizeof(item_t)));
    } else if (remain < 99) {
        data_len = 1024 + (random() % (4096-1023-sizeof(item_t)));
    } else {
        data_len = 4096 + (random() % (ITEM_MAX_SIZE-4095-sizeof(item_t)));
    }
    *********************************/
    /*** Method 3: 
           24 <= item_size <   64 : 90.0% 
           64 <= item_size <  128 :  9.0% 
          128 <= item_size < 1024 :  0.7% 
         1024 <= item_size < 4096 :  0.2% 
         4096 <= item_size <  1MB :  0.1%
    ********************************/
    int remain = item_count % 1000; 
    if (remain < 900) {
        data_len = random() % (64-sizeof(item_t));
    } else if (remain < 990) {
        data_len =   64 + (random() % (128-63-sizeof(item_t)));
    } else if (remain < 997) {
        data_len =  128 + (random() % (1024-127-sizeof(item_t)));
    } else if (remain < 999) {
        data_len = 1024 + (random() % (4096-1023-sizeof(item_t)));
    } else {
        data_len = 4096 + (random() % (ITEM_MAX_SIZE-4095-sizeof(item_t)));
    }
    return data_len;
}

static void init_item(item_t *item_ptr, int data_len) {
    int val_offset = 0;  
    int copy_len;

    item_ptr->nbytes = data_len;
    while (data_len > 0) {
        copy_len = (data_len > 1024 ? 1024 : data_len);
        memcpy(&item_ptr->value[val_offset], sample_data, copy_len);
        val_offset += copy_len;
        data_len   -= copy_len; 
    }
}

static void insert_item_into_head(item_t *item_ptr) {
    item_ptr->prev = NULL;
    item_ptr->next = item_head; 
    if (item_ptr->next != NULL)
        item_ptr->next->prev = item_ptr;
    item_head = item_ptr;
    if (item_tail == NULL) item_tail = item_ptr;
    item_count++; 
}

static item_t *remove_item_from_tail() {
    item_t *item_ptr = item_tail;
    if (item_ptr != NULL) {
        if (item_ptr->prev != NULL)
            item_ptr->prev->next = NULL;
        item_tail = item_ptr->prev;
        if (item_tail == NULL)
            item_head = NULL;
        item_ptr->prev = item_ptr->next = NULL; 
        item_count--;
    } else {
        assert(item_count == 0);
    } 
    return item_ptr;
}

static inline void *do_malloc(size_t size) {
    void *ptr;
#if defined(ENABLE_INTERNAL_SMALL_MEMMGR)
    if ((size+2) <= MEM_LIMIT_SIZE) { /* small memory */
        int memid = (size + 2 - 1) / 8; /* 2 : for keeping physical item index in the chunk */
        mem_chunk_t *chunk;

        if (mem_anchor[memid].chunk_free_head == NULL) {
            chunk = do_mem_chunk_alloc(memid);
            if (chunk == NULL) {
                fprintf(stderr, "do_mem_chunk_alloc fail\n");
                return NULL;
            }
            do_mem_chunk_link(memid, chunk, true);
            assert(mem_anchor[memid].chunk_free_head != NULL);
        }

        chunk = mem_anchor[memid].chunk_free_head;
        ptr   = chunk->free_item;
        chunk->free_item = *((void**)chunk->free_item);
        chunk->free_item_count--;

        if (chunk->free_item_count == 0) {
            assert(chunk->free_item == NULL);
            do_mem_chunk_unlink(memid, chunk, false);
        }
    } else { /* large memory */
        if ((ptr = malloc(size)) == NULL) {
            fprintf(stderr, "memory allocation fail: size=%d\n",size);
            return NULL;
        }
    }
#else
    if ((ptr = malloc(size)) == NULL) {
        fprintf(stderr, "memory allocation fail: size=%d\n",size);
        return NULL;
    }
#endif
    mem_alloc += size;
    return ptr;
}

static inline void do_free(void *ptr, int size) {
#if defined(ENABLE_INTERNAL_SMALL_MEMMGR)
    if ((size+2) <= MEM_LIMIT_SIZE) { /* small memory */
        int memid = (size + 2 - 1) / 8; /* 2 : for keeping physical item index in the chunk */
        int item_size = (memid + 1) * 8;
        uint16_t *pidx = (uint16_t*)((char*)ptr + item_size - sizeof(uint16_t));
        mem_chunk_t *chunk = (mem_chunk_t *)((char*)ptr - sizeof(mem_chunk_t) - ((*pidx)*item_size));

        *(void**)ptr = chunk->free_item;
        chunk->free_item = ptr;
        chunk->free_item_count++;

        if (chunk->free_item_count == 1) {
            do_mem_chunk_link(memid, chunk, false);
        }
        if (chunk->free_item_count >= chunk->total_item_count) {
            do_mem_chunk_unlink(memid, chunk, true);
            free(chunk);
        }
    } else { /* large memory */
        free(ptr);
    }
#else
    free(ptr);
#endif
    mem_alloc -= size;
}

static void dump_malloc_info() {
#ifdef USE_TCMALLOC
    /******
    if (1) {
        int i;
        size_t prop_data[TCMALLOC_PROPERTY_COUNT]; 
        for (i = 0; i < TCMALLOC_PROPERTY_COUNT; i++) {
            if (!MallocExtension_GetNumericProperty(prop_name[i], &prop_data[i])) {
                fprintf(stderr, "tcmalloc get propery error: name=%s\n", prop_name[i]);
                exit(1);
            }
            fprintf(stderr, "[tcmalloc] property: %s=%d\n", prop_name[i], prop_data[i]);
        } 
    }
    *****/
    if (1) {
        char buffer[200]; 
        MallocExtension_GetStats(buffer, sizeof(buffer));
        fprintf(stderr, "%s\n", buffer);
    }
    /*****
    if (1) {
        int i, blocks;
        int hist[64];
        size_t total;
        fprintf(stderr, "[tcmalloc] get stats: buffer=%s\n", buffer);
        MallocExtension_MallocMemoryStats(&blocks, &total, hist);
        fprintf(stderr, "[tcmalloc] memory stats: blocks=%d total=%d hist=[\n", blocks, total);
        for (i = 0; i < 64; i++) {
            fprintf(stderr, "%d, ", hist[i]);
        }
        fprintf(stderr, "]\n");
    }
    *****/
#else
    unsigned int total, used, free;
    char buffer[200];
    struct mallinfo info = mallinfo();
    total = (unsigned int) info.arena;
    used  = (unsigned int) info.uordblks;
    free  = (unsigned int) info.fordblks;
    sprintf(buffer, "-----------------------------------\n"
                    "MALLOC: %12u ( %6.1f MB) Heap size\n"
                    "MALLOC: %12u ( %6.1f MB) Bytes Used\n"
                    "MALLOC: %12d ( %6.1f MB) Bytes Free\n",
                    total, (float)total/(1024*1024),
                    used , (float)used/(1024*1024),
                    free,  (float)free/(1024*1024));
    fprintf(stderr, "%s", buffer);
    /********** 
    fprintf(stderr, "arena=%d ordblks=%d hblks=%d hblkhd=%d uordblks=%d fordblks=%d keepcost=%d\n",
            info.arena, info.ordblks, info.hblks, info.hblkhd, info.uordblks, info.fordblks, info.keepcost);
    **********/
#endif
}

main(int argc, void **argv)
{
    unsigned int mlimit_MB;
    unsigned int max_count;
    unsigned int try_count;
    int data_len; 
    int item_len; 
    int free_len;    
    item_t *item_ptr;

    if (argc != 3) {
        fprintf(stderr, "Usage: utility_name <mem_limit(unit: MB)> <malloc_count>\n");
        exit(1);
    }
    mlimit_MB = atoi(argv[1]);
    mem_limit = mlimit_MB * 1024 * 1024;
    max_count = atoi(argv[2]);

    fprintf(stderr, "[%s] mem_limit=%u(%u MB) malloc_count=%u\n",
                    argv[0], mem_limit, mlimit_MB, max_count); 

    if (init_malloc_test() != 0) {
        fprintf(stderr, "init_malloc_test fail\n");
        exit(1);
    }

    try_count = 0;
    while (try_count < max_count)
    {
        data_len = get_data_len(try_count);
        item_len = sizeof(item_t) + data_len;

#if defined(USE_TCMALLOC) && defined(LIMIT_HEAP_SIZE)
        while ((item_len + heap_size) > mem_limit) {
            item_ptr = remove_item_from_tail(); 
            if (item_ptr == NULL) {
                fprintf(stderr, "empty list: mem_alloc=%u\n", mem_alloc);
                exit(1);
            } 
            free_len = (sizeof(item_t) + item_ptr->nbytes);
            do_free(item_ptr, free_len);
            MallocExtension_ReleaseToSystem(free_len);
            MallocExtension_ReleaseFreeMemory();
            if (!MallocExtension_GetNumericProperty("generic.heap_size", &heap_size)) {
                fprintf(stderr, "tcmalloc get propery generic.heapsize error\n");
                exit(1);
            }
        }
#else
        while ((item_len + mem_alloc) > mem_limit) {
            item_ptr = remove_item_from_tail(); 
            if (item_ptr == NULL) {
                fprintf(stderr, "empty list: mem_alloc=%u\n", mem_alloc);
                exit(1);
            } 
            free_len = (sizeof(item_t) + item_ptr->nbytes);
            do_free(item_ptr, free_len);
        }
#endif

        item_ptr = (item_t*)do_malloc(item_len);
        if (item_ptr == NULL) {
            fprintf(stderr, "do_malloc fail: item_len=%d\n", item_len);
            exit(1);
        }

        init_item(item_ptr, data_len);
        insert_item_into_head(item_ptr);

        try_count++;
        if ((try_count < max_count) && ((try_count % 10000000) == 0)) {
            fprintf(stderr, "execution(%u) mem_alloc=%u item_count=%u\n",
                            try_count, mem_alloc, item_count);
            dump_malloc_info();
        }
    }

    fprintf(stderr, "execution(%u) mem_alloc=%u item_count=%u\n",
                    try_count, mem_alloc, item_count);
    dump_malloc_info();

    while ((item_ptr = remove_item_from_tail()) != NULL) {
        free_len = (sizeof(item_t) + item_ptr->nbytes);
        do_free(item_ptr, free_len);
    }  
    assert(item_head == NULL);
    assert(item_tail == NULL);
    assert(item_count == 0);
    assert(mem_alloc == 0);

    exit(0);
}

