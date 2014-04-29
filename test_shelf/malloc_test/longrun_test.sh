#!/bin/sh

echo "===================================================="
echo "ptmalloc & tcmalloc test: 10 times more execution"
echo "===================================================="
TENTIMES=`expr $1 \* 10`
/usr/bin/time ./ptmalloc_naive_test 2048 $TENTIMES
/usr/bin/time ./tcmalloc_naive_test 2048 $TENTIMES
/usr/bin/time ./ptmalloc_smmgr_test 2048 $TENTIMES
/usr/bin/time ./tcmalloc_smmgr_test 2048 $TENTIMES
echo "===================================================="

