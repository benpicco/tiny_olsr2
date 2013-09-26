#ifndef WRITER_H_
#define WRITER_H_

#include "common/common_types.h"
#include "rfc5444/rfc5444_writer.h"
#include "constants.h"

void writer_init(write_packet_func_ptr ptr);
void writer_send_hello(void);
void writer_send_tc(void);
void writer_cleanup(void);

#endif /* WRITER_H_ */
