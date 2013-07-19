#include <stdio.h>

#include "writer.h"
#include "reader.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)), 
	struct rfc5444_writer_target *iface __attribute__((unused)), 
	void *buffer, size_t length) {

	printf("write_packet called\n");
}

int main() {
	reader_init();
	writer_init(write_packet);

	writer_tick();

	reader_cleanup();
	writer_cleanup();
}