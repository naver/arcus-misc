TARGET=example
OBJS=arcus.o example.o
SRCS=$(OBJS:.o=.cpp)
CXXFLAGS=-g -I$(HOME)/arcus/include -I$(HOME)/arcus/include/c-client-src
LDFLAGS=-lmemcached -lmemcachedutil -pthread -lm -L$(HOME)/arcus/lib

all: $(TARGET)

.cpp.o: .depend
	g++ $(CXXFLAGS) -c $<

$(TARGET): $(OBJS)
	g++ $(LDFLAGS) -o $@ $^

.depend:
	g++ -MM $(SRCS) > .depend

-include .depend

clean:
	rm -rf $(TARGET) $(OBJS) .depend
