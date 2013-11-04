CC=clang
INCLUDE=-I../../oonf_api/src-api -I../oonf_api/build -I../node/olsr2
LIBDIR=../../oonf_api/build
SRC=../node/olsr2
CFLAGS=-Wall -std=c99 -ggdb -DENABLE_DEBUG_OLSR -DENABLE_NAME $(INCLUDE)
LDFLAGS=-L$(LIBDIR) -loonf_rfc5444 -loonf_common

objects = $(SRC)/routing.o $(SRC)/list.o $(SRC)/node.o $(SRC)/nhdp.o $(SRC)/olsr.o $(SRC)/util.o cunit/cunit.o

test_list: test_list.c ../node/olsr2/list.c cunit/cunit.c

test_olsr: $(objects) test_olsr.o
	cc $(objects) test_olsr.o -o test_olsr $(LDFLAGS)

run:	test_list test_olsr
	LD_LIBRARY_PATH=$(LIBDIR) ./test_list
	LD_LIBRARY_PATH=$(LIBDIR) ./test_olsr

clean:
	rm test_list
	rm test_olsr
