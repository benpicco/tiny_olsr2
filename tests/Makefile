CC=clang
INCLUDE=-I../../oonf_api/src-api -I../oonf_api/build
LIBDIR=../../oonf_api/build
CFLAGS=-Wall -std=c99 -DDEBUG $(INCLUDE)
LDFLAGS=$(LIBDIR)/liboonf_rfc5444.so $(LIBDIR)/liboonf_common.so

avl_eval:	avl_eval.o $(LDFLAGS)
		cc avl_eval.o $(LDFLAGS) -o avl_eval

run: avl_eval
	LD_LIBRARY_PATH=$(LIBDIR) ./avl_eval
