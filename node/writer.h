#ifndef NHDP_WRITER_H_
#define NHDP_WRITER_H_

#include "common/common_types.h"
#include "rfc5444/rfc5444_writer.h"

typedef void (*write_packet_func_ptr)(
	struct rfc5444_writer *wr, struct rfc5444_writer_target *iface, void *buffer, size_t length);

void writer_init(write_packet_func_ptr ptr);
void writer_send_hello(void);
void writer_send_tc(void);
void writer_cleanup(void);

#endif /* NHDP_WRITER_H_ */
