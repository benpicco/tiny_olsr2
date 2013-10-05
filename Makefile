CC=clang

INCLUDE=-I../oonf_api/src-api -I../oonf_api/build
LIBDIR=../oonf_api/build

CFLAGS=-Wall -std=gnu99 -DENABLE_DEBUG $(INCLUDE)
LDFLAGS=$(LIBDIR)/liboonf_rfc5444.so $(LIBDIR)/liboonf_common.so

.PHONY: clean run

NODES := `grep -- -\> graph.gv | grep -o . | sort | grep [[:alnum:]] | uniq | wc -l`
LOG_DIR := log

node:	node/main.o node/node.o node/reader.o node/writer.o node/nhdp.o node/olsr.o node/util.o $(LDFLAGS)
	cc node/main.o node/node.o node/reader.o node/writer.o node/nhdp.o node/olsr.o node/util.o $(LDFLAGS) -o node/node

dispatcher:	dispatcher.o $(LDFLAGS)

graph.pdf: graph.gv
	-neato -Tpdf graph.gv > graph.pdf

mpr_graph.pdf: graph.gv log/*.log
	grep -h \"MPR\" log/* | sort | uniq | cat graph.gv - | neato -Tpdf > mpr_graph.pdf

clean:
	find -name '*.o' -type f -delete
	rm graph.pdf
	rm mpr_graph.pdf
	rm node/node
	rm dispatcher

kill:
	-killall node
	-killall dispatcher

run:	dispatcher node kill graph.pdf
	LD_LIBRARY_PATH=$(LIBDIR) ./dispatcher graph.gv 9000 &

	@mkdir -p $(LOG_DIR)
	@for i in $(shell seq 1 ${NODES}) ; do \
		LD_LIBRARY_PATH=$(LIBDIR) stdbuf -oL -eL ./node/node 127.0.0.1 9000 2001::$$i > $(LOG_DIR)/$$i.log 2>&1 & \
	done
