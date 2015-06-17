JAVA_CLIENT=$HOME/src/arcus-java-client/target/arcus-java-client-1.8.1.jar
JAVA_CLIENT_LIB_DIR=$HOME/src/arcus-java-client/target

LIB_CP=$JAVA_CLIENT:$JAVA_CLIENT_LIB_DIR/zookeeper-3.4.5.jar:$JAVA_CLIENT_LIB_DIR/log4j-1.2.16.jar:$JAVA_CLIENT_LIB_DIR/slf4j-api-1.6.1.jar:$JAVA_CLIENT_LIB_DIR/slf4j-log4j12-1.6.1.jar

rm -rf ./bin
mkdir bin

javac -sourcepath src -d bin -Xlint:deprecation -cp $JAVA_CLIENT:. src/CASMutatorExpireTest.java
java -ea -Xmx2g -Xms2g "-Dnet.spy.log.LoggerImpl=net.spy.memcached.compat.log.Log4JLogger" "-Dlog4j.configuration=file://$PWD/log4j.properties" -cp $LIB_CP:bin CASMutatorExpireTest $@
