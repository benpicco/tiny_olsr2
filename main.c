#include <stdio.h>
#include <thread.h>
#include <vtimer.h>
#include <rtc.h>
#include <random.h>
#include <destiny.h>
#include <transceiver.h>

#include <shell.h>
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

const shell_command_t shell_commands[] = {
    {"init", "start the IP stack with OLSRv2", init},
    {"routes", "print all known nodes and routes", print_topology_set},
    {NULL, NULL, NULL}
};

int main(void) {
    posix_open(uart0_handler_pid, 0);

    shell_t shell;
    shell_init(&shell, shell_commands, uart0_readc, uart0_putc);

    shell_run(&shell);

    return 0;
}
