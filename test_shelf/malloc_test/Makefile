CFLAGS=-O2

all:
	gcc ${CFLAGS} -o ptmalloc_naive_test malloc_test.c
	gcc ${CFLAGS} -o ptmalloc_smmgr_test malloc_test.c -DENABLE_INTERNAL_SMALL_MEMMGR 
	gcc ${CFLAGS} -o tcmalloc_naive_test malloc_test.c -DUSE_TCMALLOC -ltcmalloc
	gcc ${CFLAGS} -o tcmalloc_smmgr_test malloc_test.c -DENABLE_INTERNAL_SMALL_MEMMGR -DUSE_TCMALLOC -ltcmalloc

clean:
	\rm ptmalloc_naive_test tcmalloc_naive_test ptmalloc_smmgr_test tcmalloc_smmgr_test
	 
