DIR=`readlink -f $0`
DIR=`dirname $DIR`
MEMC_DIR=$DIR/../../arcus-memcached

$MEMC_DIR/memcached -E $MEMC_DIR/.libs/default_engine.so -X $MEMC_DIR/.libs/syslog_logger.so -X $MEMC_DIR/.libs/ascii_scrub.so -d -v -r -R5 -U 0 -D: -b 8192 -m1500 -p 11215 -c 1000 -t 6 -z 127.0.0.1:2181
$MEMC_DIR/memcached -E $MEMC_DIR/.libs/default_engine.so -X $MEMC_DIR/.libs/syslog_logger.so -X $MEMC_DIR/.libs/ascii_scrub.so -d -v -r -R5 -U 0 -D: -b 8192 -m2000 -p 11216 -c 1000 -t 6 -z 127.0.0.1:2181
#$MEMC_DIR/memcached -E $MEMC_DIR/.libs/default_engine.so -X $MEMC_DIR/.libs/syslog_logger.so -X $MEMC_DIR/.libs/ascii_scrub.so -d -v -r -R5 -U 0 -D: -b 8192 -m1500 -p 11216 -c 1000 -t 6 -z 127.0.0.1:2181
