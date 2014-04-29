SRCS=sendboxtest.c lua_sendbox.c
OBJS=$(SRCS:.c=.o)
TARGET=test

all: $(TARGET)

test : $(SRCS)
	gcc -g -I$(HOME)/local/include -lm -ldl -lpthread -o $@ $(SRCS) $(HOME)/local/lib/liblua.a

clean :
	rm -rf $(TARGET)
