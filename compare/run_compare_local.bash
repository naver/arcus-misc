base_dir=/home1/username/openarcus
misc_dir=$base_dir/misc-replication/misc
compare_dir=$base_dir/misc-replication/client
jar_dir=$base_dir/arcus-java-client/target

rm -f $misc_dir/keydump*
rm -f $compare_dir/keydump*

echo "replication dumpkey" | nc localhost 11212
sleep 2
mv $misc_dir/keydump* $compare_dir/keydump.11212

echo "replication dumpkey" | nc localhost 11211
sleep 2
mv $misc_dir/keydump* $compare_dir/keydump.11211

CP=$jar_dir/arcus-client-1.7.0.jar:$jar_dir/zookeeper-3.4.5.jar:$jar_dir/log4j-1.2.16.jar:$jar_dir/slf4j-api-1.6.1.jar:$jar_dir/slf4j-log4j12-1.6.1.jar

/usr/bin/java -classpath $CP:$compare_dir \
  compare -keydump $compare_dir/keydump* \
  -server localhost:11211 -server localhost:11212 $@
