DIR=`readlink -f $0`
DIR=`dirname $DIR`

ZK_CLI="$DIR/../../arcus/zookeeper/bin/zkCli.sh"
ZK_ADDR="-server localhost:2181"

$ZK_CLI $ZK_ADDR create /arcus_repl 0

$ZK_CLI $ZK_ADDR create /arcus_repl/client_list 0
$ZK_CLI $ZK_ADDR create /arcus_repl/client_list/test 0

$ZK_CLI $ZK_ADDR create /arcus_repl/cache_server_log 0

$ZK_CLI $ZK_ADDR create /arcus_repl/cache_list 0
$ZK_CLI $ZK_ADDR create /arcus_repl/cache_list/test 0
# ehpemeral znode = <group>^M^<ip:port-hostname> 0 // created by cache node
# ehpemeral znode = <group>^S^<ip:port-hostname> 0 // created by cache node

$ZK_CLI $ZK_ADDR create /arcus_repl/group_list 0
$ZK_CLI $ZK_ADDR create /arcus_repl/group_list/test 0
$ZK_CLI $ZK_ADDR create /arcus_repl/group_list/test/g0 0
# ehpemeral/sequence znode = <nodeip:port>^<listenip:port>^<sequence> 0
# ehpemeral/sequence znode = <nodeip:port>^<listenip:port>^<sequence> 0

$ZK_CLI $ZK_ADDR create /arcus_repl/cache_server_mapping 0
$ZK_CLI $ZK_ADDR create /arcus_repl/cache_server_mapping/127.0.0.1:11215 0
$ZK_CLI $ZK_ADDR create /arcus_repl/cache_server_mapping/127.0.0.1:11215/test^g0^127.0.0.1:20125^ 0
$ZK_CLI $ZK_ADDR create /arcus_repl/cache_server_mapping/127.0.0.1:11216 0
$ZK_CLI $ZK_ADDR create /arcus_repl/cache_server_mapping/127.0.0.1:11216/test^g0^127.0.0.1:20126^ 0
