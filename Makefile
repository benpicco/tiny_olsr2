INCLUDE=-I/home/benpicco/µkleos/upstream/oonf_api/src-api -I/home/benpicco/µkleos/upstream/oonf_api/build 
LNCLUDE=/home/benpicco/µkleos/upstream/oonf_api/build

olsr.o:	main.c
	gcc -Wall -std=c99 $(INCLUDE) -c main.c -o olsr.o

olsr:	olsr.o
	gcc olsr.o $(LNCLUDE)/liboonf_rfc5444.so $(LNCLUDE)/liboonf_common.so -o olsr

run:	olsr
	LD_LIBRARY_PATH=$(LNCLUDE) ./olsr

clean:
	rm olsr.o
	rm olsr
