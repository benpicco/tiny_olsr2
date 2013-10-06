CC=clang
INCLUDE=-I../../oonf_api/src-api -I../oonf_api/build -I../node
LIBDIR=../../oonf_api/build
CFLAGS=-Wall -std=c99 -DENABLE_DEBUG $(INCLUDE)
LDFLAGS=$(LIBDIR)/liboonf_rfc5444.so $(LIBDIR)/liboonf_common.so

avl_eval:	avl_eval.o $(LDFLAGS)
		cc avl_eval.o $(LDFLAGS) -o avl_eval

test_list: test_list.c ../node/list.c cunit/cunit.c

run: avl_eval
	LD_LIBRARY_PATH=$(LIBDIR) ./avl_eval
