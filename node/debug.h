#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef ENABLE_DEBUG
#include <execinfo.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
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

#define DTL	printf("%d, %s\n", __LINE__, __FUNCTION__)

/* this only works for a circle */
static inline bool is_valid_neighbor(struct netaddr* a, struct netaddr* b) {
	if (a == NULL || b == NULL)
		return true;
	if ((a->_addr[15] == 1 && b->_addr[15] == NODES) || 
		(b->_addr[15] == 1 && a->_addr[15] == NODES))
		return true;
	return (a->_addr[15] - b->_addr[15] == 1) || (b->_addr[15] - a->_addr[15] == 1);
}

static inline void print_trace(void) {
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	printf ("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		printf ("%s\n", strings[i]);

	free (strings);
}

#else	/* no ENABLE_DEBUG */

#define DEBUG(...)
#define DEBUG_TICK

static inline bool is_valid_neighbor(struct netaddr* a, struct netaddr* b) {
	return true;
}

#endif
#endif
