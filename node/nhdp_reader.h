#ifndef READER_H_
#define READER_H_

#include "common/common_types.h"
#include "rfc5444/rfc5444_reader.h"

void nhdp_reader_init(void);
int nhdp_reader_handle_packet(void* buffer, size_t length);
void nhdp_reader_cleanup(void);

#endif /* READER_H_ */
