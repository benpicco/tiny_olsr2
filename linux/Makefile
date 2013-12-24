CC=clang

INCLUDE=-I../../oonf_api/src-api -I../../oonf_api/build -I../node/olsr2
LIBDIR=../../oonf_api/build

SRC=../node/olsr2

CFLAGS=-Wall -Wextra -O3 -ggdb -std=gnu99 -DENABLE_NAME -DENABLE_DEBUG_OLSR $(INCLUDE)
LDFLGS=-L$(LIBDIR) -loonf_rfc5444 -loonf_common

.PHONY: clean run

NODES := $(shell grep -- -\> graph.gv | while read line; do for w in $$line; do echo $$w; done; done | grep [Aa-Zz] | sort | uniq | wc -l)
LOG_DIR := log

objects = $(SRC)/main.o $(SRC)/routing.o $(SRC)/list.o $(SRC)/node.o $(SRC)/reader.o $(SRC)/writer.o $(SRC)/nhdp.o $(SRC)/olsr.o $(SRC)/util.o

node:	$(objects)
	cc $(objects) -o $(SRC)/node $(LDFLGS)

dispatcher:	dispatcher.o

topology_generator: topology_generator.o $(SRC)/list.o

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
	find $(SRC) -name '*.o' -type f -delete
	rm $(SRC)/node
	rm dispatcher.o
	rm dispatcher
	rm graph.pdf
	rm log/*
	-rm mpr_graph.pdf
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
		LD_LIBRARY_PATH=$(LIBDIR) stdbuf -oL -eL $(SRC)/node 127.0.0.1 9000 2001::$$i > $(LOG_DIR)/$$i.log 2>&1 & \
	done
