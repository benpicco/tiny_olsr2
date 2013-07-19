INCLUDE=-I../upstream/oonf_api/src-api -I../upstream/oonf_api/build
LIBDIR=../upstream/oonf_api/build

CFLAGS=-Wall -std=c99 $(INCLUDE)
LDFLAGS=$(LIBDIR)/liboonf_rfc5444.so $(LIBDIR)/liboonf_common.so

.PHONY: clean run

node:	node/node.o node/reader.o node/writer.o $(LDFLAGS)

dispatcher:	dispatcher.o $(LDFLAGS)

clean:
	find -name '*.o' -type f -delete
	rm dispatcher

run:	dispatcher
	LD_LIBRARY_PATH=$(LIBDIR) ./dispatcher 9000
