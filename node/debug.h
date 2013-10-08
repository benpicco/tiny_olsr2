#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef ENABLE_DEBUG
#include <stdio.h>
#include <time.h>
#include "common/netaddr.h"

struct netaddr_str nbuf[4];
int debug_ticks;

time_t _t_tmr;
char _t_buf[9];

// #define DEBUG(fmt, ...) printf((fmt "\n"), ##__VA_ARGS__);
#define DEBUG(fmt, ...) {	\
	_t_tmr = time(0);			\
	strftime(_t_buf, sizeof _t_buf, "%H:%M:%S", localtime(&_t_tmr));	\
	printf(("[%d, %s] " fmt "\n"), debug_ticks, _t_buf, ##__VA_ARGS__);	\
	}

#define DEBUG_TICK		debug_ticks++

#else	/* no ENABLE_DEBUG */

#define DEBUG(...)
#define DEBUG_TICK

#endif
#endif
