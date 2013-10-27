#include <sixlowpan/ip.h>

struct ping_packet {
	uint32_t id;
	uint32_t timestamp;
	uint8_t hops;
	uint8_t received;
};

void ping_send(ipv6_addr_t* dest);
void ping_start_listen(void(*callback)(struct ping_packet*));
void ping_stop_listen(void);
