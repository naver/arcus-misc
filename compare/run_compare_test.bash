base_dir=/home1/username/arcus-rep
misc_dir=$base_dir/rep_proto/misc
compare_dir=$base_dir/rep_proto/client
jar_dir=$base_dir/java-memcached-client/target

/usr/bin/ssh hostname02 rm -f $misc_dir/keydump*
rm -f $misc_dir/keydump*
rm -f $compare_dir/keydump*

echo "replication dumpkey" | nc hostname02 11212
echo "replication dumpkey" | nc hostname01 11211

/usr/bin/scp hostname02:$misc_dir/keydump* $compare_dir/keydump02
mv $misc_dir/keydump* $compare_dir/keydump01

/usr/bin/java -classpath $jar_dir/arcus-client-1.6.3.0.jar:$jar_dir/zookeeper-3.3.3-p1.jar:$jar_dir/log4j-1.2.16.jar:$compare_dir \
  compare -keydump $compare_dir/keydump* \
  -server hostname01:11211 -server hostname02:11212 $@

