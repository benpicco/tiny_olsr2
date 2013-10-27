#include <stdio.h>
#include <thread.h>
#include <vtimer.h>
#include <rtc.h>
#include <random.h>
#include <destiny.h>
#include <transceiver.h>

#include <shell.h>
#include <posix_io.h>
#include <board_uart0.h>

#include <olsr2.h>
#include <udp_ping.h>

#if BOARD == native
#include <unistd.h>
static uint8_t _trans_type = TRANSCEIVER_NATIVE;
static uint16_t get_node_id() {
	return getpid();
}
#elif BOARD == msba2
static uint8_t _trans_type = TRANSCEIVER_CC1100;
static uint16_t get_node_id() {
	// TODO
	return 23;
}
#endif

void init(char *str) {

	rtc_enable();
	genrand_init(get_node_id());
	destiny_init_transport_layer();
	sixlowpan_lowpan_init(_trans_type, get_node_id(), 0);

	olsr_init();
}

#ifdef ENABLE_NAME
void ping_callback(struct ping_packet* pong) {
	timex_t now;
	vtimer_now(&now);
	printf("\tpong [%u], %u µs %u hops", pong->id,
		now.microseconds - pong->timestamp, pong->hops);
}

void ping(char* str) {
	ipv6_addr_t* dest = get_ip_by_name(str);
	if (dest == NULL) {
		puts("Unknown node");
		return;
	}

	char addr_str[IPV6_MAX_ADDR_STR_LEN];
	ipv6_addr_to_str(addr_str, dest);

	ping_start_listen(ping_callback);

	for (int i = 0; i < 10; ++i) {
		printf("sending %d bytes to %s\n", sizeof(struct ping_packet), addr_str);
		ping_send(dest);
		vtimer_usleep(1000000);
	}

	ping_stop_listen();
}
#endif /* ENABLE_NAME */

const shell_command_t shell_commands[] = {
    {"init", "start the IP stack with OLSRv2", init},
    {"routes", "print all known nodes and routes", print_topology_set},
#ifdef ENABLE_NAME
    {"ping", "send packets to a node", ping},
#endif
    {NULL, NULL, NULL}
};

int main(void) {
    posix_open(uart0_handler_pid, 0);

    shell_t shell;
    shell_init(&shell, shell_commands, uart0_readc, uart0_putc);

    shell_run(&shell);

    return 0;
}
