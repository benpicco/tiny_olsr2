CC=clang

INCLUDE=-I../oonf_api/src-api -I../oonf_api/build
LIBDIR=../oonf_api/build

CFLAGS=-Wall -std=gnu99 -DENABLE_DEBUG $(INCLUDE)
LDFLAGS=-L$(LIBDIR) -loonf_rfc5444 -loonf_common

.PHONY: clean run

NODES = $(shell grep -- -\> graph.gv | grep -o . | sort | grep [A-Z] | uniq | wc -l)
LOG_DIR := log

objects = node/main.o node/routing.o node/list.o node/node.o node/reader.o node/writer.o node/nhdp.o node/olsr.o node/util.o

node:	$(objects)
	cc $(objects) -o node/node $(LDFLAGS)

dispatcher:	dispatcher.o

graph.pdf: graph.gv
	-neato -Tpdf graph.gv > graph.pdf

mpr_graph.pdf: graph.gv log/*.log
	@for i in $(shell seq 1 ${NODES}) ; do \
		tac log/$$i.log | sed '1,/END MPR/d' | sed '/BEGIN MPR/,$$d' | tac >> /tmp/mprs.gv ; \
		{ cat graph.gv ; echo "subgraph mpr {" ; echo "edge [ color = blue ]" ; cat /tmp/mprs.gv ; echo "}}" ;} | neato -Tpdf > mpr_graph.pdf ; \
	done
	@rm /tmp/mprs.gv

routing_graphs: graph.gv log/*.log
	@for i in $(shell seq 1 ${NODES}) ; do \
		{ cat graph.gv; tac log/$$i.log | sed '1,/END ROUTING GRAPH/d' | sed '/BEGIN ROUTING GRAPH/,$$d' | tac; echo }; } | neato -Tpdf > log/routing_graph_$$i.pdf ; \
	done

clean:
	find -name '*.o' -type f -delete
	rm node/node
	rm dispatcher
	rm graph.pdf
	rm mpr_graph.pdf

stop:
	kill -STOP $(shell pidof -s node)

cont:
	kill -CONT $(shell pidof -s node)

kill:
	-killall node
	-killall dispatcher

run:	dispatcher node kill graph.pdf
	LD_LIBRARY_PATH=$(LIBDIR) ./dispatcher graph.gv 9000 &

	@mkdir -p $(LOG_DIR)
	@for i in $(shell seq 1 ${NODES}) ; do \
		sleep 0.1;	\
		LD_LIBRARY_PATH=$(LIBDIR) stdbuf -oL -eL ./node/node 127.0.0.1 9000 2001::$$i > $(LOG_DIR)/$$i.log 2>&1 & \
	done
