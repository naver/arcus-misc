DIR=`readlink -f $0`
DIR=`dirname $DIR`
if test -d "$DIR/../../arcus-java-client" ; then
  JAR_DIR=$DIR/../../arcus-java-client/target
  CP=$JAR_DIR/arcus-java-client-1.7.0.jar
else
  if test -d "$DIR/../../java-memcached-client" ; then
    JAR_DIR=$DIR/../../java-memcached-client/target
    CP=$JAR_DIR/arcus-client-1.6.3.0.jar
  else
    echo "Cannot find arcus jar directory."
    exit
  fi
fi

javac -classpath $CP compare.java JenkinsHash.java
