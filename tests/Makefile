CC=clang
INCLUDE=-I../../oonf_api/src-api -I../oonf_api/build -I../node/olsr2
LIBDIR=../../oonf_api/build
CFLAGS=-Wall -std=c99 -DENABLE_DEBUG $(INCLUDE)

test_list: test_list.c ../node/olsr2/list.c cunit/cunit.c

clean:
	rm test_list
