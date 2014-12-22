DIR=`readlink -f $0`
DIR=`dirname $DIR`

ZK_CLI="$DIR/../../arcus/zookeeper/bin/zkCli.sh"
ZK_ADDR="-server localhost:2181"

$ZK_CLI $ZK_ADDR create /arcus_1_7 0

$ZK_CLI $ZK_ADDR create /arcus_1_7/client_list 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/client_list/test 0

$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_log 0

$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_list 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_list/test 0

$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_group 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_group/test 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_group/test/g0 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_group/test/g0/lock 0

$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_mapping 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_mapping/127.0.0.1:11215 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_mapping/127.0.0.1:11215/test^g0^127.0.0.1:20125^ 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_mapping/127.0.0.1:11216 0
$ZK_CLI $ZK_ADDR create /arcus_1_7/cache_server_mapping/127.0.0.1:11216/test^g0^127.0.0.1:20126^ 0
