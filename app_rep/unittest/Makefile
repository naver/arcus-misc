TARGET=test_run

SRCS=$(shell ls *.cpp)
ORIG_SRCS=../arcus.cpp
OBJS=$(SRCS:.cpp=.o)
ORIG_OBJS=$(patsubst ../%.cpp, %_orig.o, $(ORIG_SRCS))

all: $(TARGET)

$(TARGET): $(OBJS) $(ORIG_OBJS)
	g++ -g -pthread -I$(HOME)/arcus/include -I$(HOME)/arcus/include/c-client-src -lmemcached -lmemcachedutil -lpthread -lm -L$(HOME)/arcus/lib -L/usr/local/lib -lgtest -o $@ $^

%_orig.o: ../%.cpp
	g++ -g -pthread -I$(HOME)/arcus/include -I$(HOME)/arcus/include/c-client-src -c -o $@ $<

.cpp.o:
	g++ -g -pthread -I$(HOME)/arcus/include -I$(HOME)/arcus/include/c-client-src -c $<

test: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET)

clean:
	rm -rf $(TARGET) $(OBJS) $(ORIG_OBJS)
