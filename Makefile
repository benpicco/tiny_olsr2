CC=clang

INCLUDE=-I../oonf_api/src-api -I../oonf_api/build
LIBDIR=../oonf_api/build

CFLAGS=-Wall -std=c99 $(INCLUDE)
LDFLAGS=$(LIBDIR)/liboonf_rfc5444.so $(LIBDIR)/liboonf_common.so

.PHONY: clean run

node:	node/node.o node/reader.o node/writer.o node/list.o node/nhdp.o $(LDFLAGS)
	cc node/node.o node/reader.o node/writer.o node/list.o node/nhdp.o $(LDFLAGS) -o node/node

dispatcher:	dispatcher.o $(LDFLAGS)

clean:
	find -name '*.o' -type f -delete
	rm node/node
	rm dispatcher

kill:
	-killall node
	-killall dispatcher

run:	dispatcher node kill
	LD_LIBRARY_PATH=$(LIBDIR) ./dispatcher graph.gv 9000 &
	LD_LIBRARY_PATH=$(LIBDIR) ./node/node 127.0.0.1 9000 2001::1 > /dev/stdout	&
	LD_LIBRARY_PATH=$(LIBDIR) ./node/node 127.0.0.1 9000 2001::2 > /dev/null	&
	LD_LIBRARY_PATH=$(LIBDIR) ./node/node 127.0.0.1 9000 2001::3 > /dev/null	&
