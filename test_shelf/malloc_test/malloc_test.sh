#!/bin/sh

# The First
echo "===================================================="
echo "ptmalloc & tcmalloc test"
echo "===================================================="
/usr/bin/time ./ptmalloc_naive_test 64   $1
/usr/bin/time ./tcmalloc_naive_test 64   $1
/usr/bin/time ./ptmalloc_naive_test 128  $1
/usr/bin/time ./tcmalloc_naive_test 128  $1
/usr/bin/time ./ptmalloc_naive_test 256  $1
/usr/bin/time ./tcmalloc_naive_test 256  $1
/usr/bin/time ./ptmalloc_naive_test 512  $1
/usr/bin/time ./tcmalloc_naive_test 512  $1
/usr/bin/time ./ptmalloc_naive_test 1024 $1
/usr/bin/time ./tcmalloc_naive_test 1024 $1
/usr/bin/time ./ptmalloc_naive_test 2048 $1
/usr/bin/time ./tcmalloc_naive_test 2048 $1
echo "===================================================="

# The Second
echo "===================================================="
echo "ptmalloc & tcmalloc test with small memory allocator"
echo "===================================================="
/usr/bin/time ./ptmalloc_smmgr_test 64   $1
/usr/bin/time ./tcmalloc_smmgr_test 64   $1
/usr/bin/time ./ptmalloc_smmgr_test 128  $1
/usr/bin/time ./tcmalloc_smmgr_test 128  $1
/usr/bin/time ./ptmalloc_smmgr_test 256  $1
/usr/bin/time ./tcmalloc_smmgr_test 256  $1
/usr/bin/time ./ptmalloc_smmgr_test 512  $1
/usr/bin/time ./tcmalloc_smmgr_test 512  $1
/usr/bin/time ./ptmalloc_smmgr_test 1024 $1
/usr/bin/time ./tcmalloc_smmgr_test 1024 $1
/usr/bin/time ./ptmalloc_smmgr_test 2048 $1
/usr/bin/time ./tcmalloc_smmgr_test 2048 $1
echo "===================================================="

# The Third
echo "===================================================="
echo "ptmalloc & tcmalloc test: 10 times more execution"
echo "===================================================="
TENTIMES=`expr $1 \* 10`
/usr/bin/time ./ptmalloc_naive_test 2048 $TENTIMES
/usr/bin/time ./tcmalloc_naive_test 2048 $TENTIMES
/usr/bin/time ./ptmalloc_smmgr_test 2048 $TENTIMES
/usr/bin/time ./tcmalloc_smmgr_test 2048 $TENTIMES
echo "===================================================="

