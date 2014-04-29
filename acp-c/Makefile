#HOSTNAME:=$(shell hostname)
BASEDIR=/home/username/arcus-rep

CC = gcc
CFLAGS = -Wall -Werror -g \
	-I$(BASEDIR)/include \
	-I$(BASEDIR)/include/c-client-src
LIB_INC_FLAGS = -L$(BASEDIR)/lib
LFLAGS = $(LIB_INC_FLAGS) \
	-lmemcached -lmemcachedutil

ACP_SRC = acp.c common.c config.c keyset.c valueset.c client.c lat_vec.c \
	standard_mix.c simple_set.c torture_btree.c torture_set.c \
	torture_list.c torture_set_ins_del.c torture_btree_ins_del.c \
	torture_btree_replace.c torture_btree_decinc.c torture_btree_exptime.c \
	torture_btree_maxbkeyrange.c torture_btree_bytebkey.c \
	torture_list_ins_del.c torture_simple_decinc.c \
	torture_simple_zero_exptime.c torture_simple_sticky.c \
	torture_simple_cas.c torture_btree_bytemaxbkeyrange.c \
	simple_getset.c
ACP_OBJ = $(ACP_SRC:.c=.o)

all: acp

acp: $(ACP_OBJ)
	gcc -Wall -Werror -lpthread $(LFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	$(CC) $(CFLAGS) -MM -o $@ $<

clean:
	rm -f $(ACP_OBJ) acp $(ACP_SRC:.c=.d)

-include $(ACP_SRC:.c=.d)
