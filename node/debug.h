#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef ENABLE_DEBUG
#include <stdio.h>
#include "common/netaddr.h"

struct netaddr_str nbuf[4];
int debug_ticks;

// #define DEBUG(fmt, ...) printf((fmt "\n"), ##__VA_ARGS__);
#define DEBUG(fmt, ...) printf(("[%d] " fmt "\n"), debug_ticks, ##__VA_ARGS__);

#define DEBUG_TICK		debug_ticks++

#else	/* no ENABLE_DEBUG */

#define DEBUG(...)
#define DEBUG_TICK

#endif
#endif
