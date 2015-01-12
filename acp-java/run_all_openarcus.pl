#!/usr/bin/perl -w

$args_compare = 0;

if ($#ARGV >= 0) {
  if ($ARGV[0] eq 'compare') {
	$args_compare = 1;
  } else {
	die "Unknown option. arg=" . $ARGV[0];
  }
}

$base_dir = "/home1/username/openarcus";
$misc_dir = "$base_dir/misc-replication/misc";
$jar_dir="$base_dir/arcus-java-client/target";
$classpath="$jar_dir/arcus-client-1.7.0.jar" .
    ":$jar_dir/zookeeper-3.4.5.jar:$jar_dir/log4j-1.2.16.jar" .
    ":$jar_dir/slf4j-api-1.6.1.jar:$jar_dir/slf4j-log4j12-1.6.1.jar";
$compare_dir = "$base_dir/misc-replication/client";
$compare_cmd="java -classpath $classpath:$compare_dir" .
    " compare -keydump $compare_dir/keydump*" .
    " -server localhost:11211 -server localhost:11212";

@script_list = (
    "simple_getset",
    "simple_set",
    "standard_mix",
    "tiny_btree",
    "torture_arcus_integration",
    "torture_btree",
    "torture_btree_bytebkey",
    "torture_btree_bytemaxbkeyrange",
    "torture_btree_decinc",
    "torture_btree_exptime",
    "torture_btree_ins_del",
    "torture_btree_maxbkeyrange",
    "torture_btree_replace",
    "torture_cas",
    "torture_list",
    "torture_list_ins_del",
    "torture_set",
    "torture_set_ins_del",
    "torture_simple_decinc",
    #"torture_simple_sticky",
);

foreach $script (@script_list) {
  # Flush all before each test
  $cmd = "./flushall.bash localhost 11211";
  print "DO_FLUSH_ALL. $cmd\n";
  system($cmd);
  sleep 4;

  # Create a temporary config file to run the test
  open CONF, ">tmp-config.txt" or die $!;
  print CONF 
	"zookeeper=127.0.0.1:2181\n" .
	"service_code=test\n" .
	"client=30\n" .
	"rate=0\n" .
	"request=0\n" .
	"time=120\n" .
	"keyset_size=1000000\n" .
	"valueset_min_size=10\n" .
	"valueset_max_size=2000\n" .
	"pool=1\n" .
	"pool_size=30\n" .
	"pool_use_random=false\n" .
	"key_prefix=tmptest:\n" .
	"client_profile=" . $script . "\n";
  close CONF;

  $cmd = "java -Xmx2g -Xms2g -Dnet.spy.log.LoggerImpl=net.spy.memcached.compat.log.Log4JLogger" .
   	     " -classpath $classpath:. acp -config tmp-config.txt";
  printf "RUN COMMAND=%s\n", $cmd;

  local $SIG{TERM} = sub { print "TERM SIGNAL\n" };

  $ret = system($cmd);
  printf "EXIT CODE=%d\n", $ret;

  # Run comparison tool
  if ($args_compare) {
	#$cmd = "/usr/bin/ssh cachehost \"rm -f $misc_dir/keydump*\"";
	#print "$cmd\n";
	#system($cmd);
	
	$cmd = "rm -f $misc_dir/keydump*";
	print "$cmd\n";
	system($cmd);
	
	$cmd = "rm -f $compare_dir/keydump*";
	print "$cmd\n";
	system($cmd);

	$cmd = "./dumpkey.bash localhost 11212";
	print "$cmd\n";
	system($cmd);

	sleep 3;
	$cmd = "mv $misc_dir/keydump* $compare_dir/keydump.11212";
	print "$cmd\n";
	system($cmd);
	
	$cmd = "./dumpkey.bash localhost 11211";
	print "$cmd\n";
	system($cmd);
	
	$cmd = "mv $misc_dir/keydump* $compare_dir/keydump.11211";
	print "$cmd\n";
	system($cmd);
	
	system($compare_cmd);
  }
}

print "END RUN_MC_TESTSCRIPTS\n";
#print "To see errors.  Try grep -e \"RUN COMMAND\" -e \"bad=\" -e \"not ok\"\n";
