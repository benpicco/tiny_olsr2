#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef ENABLE_DEBUG
#include <stdio.h>
#include "common/netaddr.h"

struct netaddr_str nbuf[4];

#define DEBUG(...)	printf(__VA_ARGS__)

#else	/* no ENABLE_DEBUG */

#define DEBUG(...)

#endif
#endif