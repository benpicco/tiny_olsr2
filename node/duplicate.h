#ifndef DUPLICATE_H_
#define DUPLICATE_H_

#include "common/netaddr.h"
#include <stdbool.h>

bool is_known_msg(struct netaddr* src, uint16_t seq_no, uint8_t validity);
void duplicate_init();

#endif