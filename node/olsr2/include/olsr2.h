#include "sixlowpan/types.h"

void olsr_init(void);
void print_topology_set(void);
#ifdef ENABLE_NAME
ipv6_addr_t* get_ip_by_name(char* name);
#endif
