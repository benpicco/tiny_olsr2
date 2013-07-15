INCLUDE=-I/home/benpicco/µkleos/upstream/oonf_api/src-api -I/home/benpicco/µkleos/upstream/oonf_api/build 
LNCLUDE=/home/benpicco/µkleos/upstream/oonf_api/build

CC=gcc -Wall -std=c99 $(INCLUDE) 

olsr.o:	main.c
	$(CC) -c main.c -o olsr.o

node.o: node/node.c
	$(CC) -c node/node.c -o node.o

reader.o: node/reader.c
	$(CC) -c node/reader.c -o reader.o

writer.o: node/writer.c
	$(CC) -c node/writer.c -o writer.o

olsr:	olsr.o node.o reader.o writer.o
	gcc olsr.o node.o reader.o writer.o $(LNCLUDE)/liboonf_rfc5444.so $(LNCLUDE)/liboonf_common.so -o olsr

run:	olsr
	LD_LIBRARY_PATH=$(LNCLUDE) ./olsr

clean:
	rm *.o
	rm olsr
