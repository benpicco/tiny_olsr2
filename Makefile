CC=clang

SRC:=../riot/RIOT/sys/net/routing/olsr2

INCLUDE=-I../oonf_api/src-api -I../oonf_api/build -I$(SRC)
LIBDIR=../oonf_api/build


CFLAGS=-Wall -Wextra -O3 -ggdb -std=gnu99 -DENABLE_NAME -DENABLE_DEBUG_OLSR $(INCLUDE)
LDFLGS=-L$(LIBDIR) -loonf_rfc5444 -loonf_common

.PHONY: clean run

GRAPH ?= graph.gv
DOT   ?= neato
ID    ?= 1

NODES := $(shell grep -- -\> ${GRAPH} | while read line; do for w in $$line; do echo $$w; done; done | grep [Aa-Zz] | sort | uniq | wc -l)
LOG_DIR := log

objects = $(SRC)/main.o $(SRC)/routing.o $(SRC)/list.o $(SRC)/node.o $(SRC)/reader.o $(SRC)/writer.o $(SRC)/nhdp.o $(SRC)/olsr.o $(SRC)/util.o

node:	$(objects)
	cc $(objects) -o node $(LDFLGS)

dispatcher:	dispatcher.o

topology_generator: topology_generator.o $(SRC)/list.o

graph.pdf: ${GRAPH}
	-${DOT} -Tpdf ${GRAPH} > graph.pdf

mpr_graph.pdf: ${GRAPH} log/*.log
	@for i in $(shell seq 1 ${NODES}) ; do \
		tac log/$$i.log | sed '1,/END MPR/d' | sed '/BEGIN MPR/,$$d' | tac >> /tmp/mprs.gv ; \
		{ cat ${GRAPH} ; echo "subgraph mpr {" ; echo "edge [ color = blue ]" ; cat /tmp/mprs.gv ; echo "}}" ;} | ${DOT} -Tpdf > mpr_graph.pdf ; \
	done
	@rm /tmp/mprs.gv

routing_graphs: ${GRAPH} log/*.log
	@for i in $(shell seq 1 ${NODES}) ; do \
		{ cat ${GRAPH}; tac log/$$i.log | sed '1,/END ROUTING GRAPH/d' | sed '/BEGIN ROUTING GRAPH/,$$d' | tac; echo }; } | ${DOT} -Tpdf > log/routing_graph_$$i.pdf ; \
	done

clean:
	find $(SRC) -name '*.o' -type f -delete
	rm node
	rm dispatcher.o
	rm dispatcher
	rm topology_generator
	rm graph.pdf
	rm log/*
	-rm mpr_graph.pdf
stop:
	kill -STOP $(shell pgrep -f 2001::$(ID))

cont:
	kill -CONT $(shell pgrep -f 2001::$(ID))

kill:
	-killall node
	-killall dispatcher

run:	dispatcher node kill graph.pdf
	@echo Starting ${NODES} nodes
	LD_LIBRARY_PATH=$(LIBDIR) ./dispatcher ${GRAPH} 9000 &

	@mkdir -p $(LOG_DIR)
	@for i in $(shell seq 1 ${NODES}) ; do \
		sleep 0.1;	\
		LD_LIBRARY_PATH=$(LIBDIR) stdbuf -oL -eL ./node 127.0.0.1 9000 2001::$$i > $(LOG_DIR)/$$i.log 2>&1 & \
#		xterm -e env LD_LIBRARY_PATH=$(LIBDIR) gdb -ex run --args ./node 127.0.0.1 9000 2001::$$i &	\
	done
