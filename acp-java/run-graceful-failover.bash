JAR_DIR=/home1/username/arcus-rep/java-memcached-client/target

java -Xmx8g -Xms8g "-Dnet.spy.log.LoggerImpl=net.spy.memcached.compat.log.Log4JLogger" "-Darcus.demoGracefulFailover=true" "-Darcus.demoPrintClusterDiff=true" -classpath $JAR_DIR/arcus-client-1.6.3.0.jar:$JAR_DIR/zookeeper-3.3.3-p1.jar:$JAR_DIR/log4j-1.2.16.jar:. acp  -config config-demo-graceful-failover.txt -pretty-stat $@
