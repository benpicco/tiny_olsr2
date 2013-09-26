#ifndef READER_H_
#define READER_H_

#include "common/common_types.h"
#include "rfc5444/rfc5444_reader.h"
#include "constants.h"

void reader_init(write_packet_func_ptr ptr);
int reader_handle_packet(void* buffer, size_t length);
void reader_cleanup(void);

#endif /* READER_H_ */
