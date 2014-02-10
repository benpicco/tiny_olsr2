#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <thread.h>
#include <vtimer.h>
#include <rtc.h>
#include <random.h>
#include <destiny.h>
#include <transceiver.h>
#include <sixlowpan/icmp.h>

#include <shell.h>
#include <posix_io.h>
#include <board_uart0.h>

#include <olsr2.h>

#if defined(BOARD_NATIVE)
#include <unistd.h>
static uint8_t _trans_type = TRANSCEIVER_NATIVE;
static uint16_t get_node_id(void) {
	return getpid();
}
#elif defined(BOARD_MSBA2)
#include <config.h>

static uint8_t _trans_type = TRANSCEIVER_CC1100;
static uint16_t generate_node_id(void) {
	int _node_id;
	const int size = 1024;

	uint8_t* buffer = malloc(size);
	for (int i=0; i<size; ++i)
		_node_id += buffer[i];
	free(buffer);

	genrand_init(_node_id);
	_node_id = (uint16_t) genrand_uint32();

	return (uint16_t) _node_id;
}

static uint16_t get_node_id(void) {
	static bool first_run = true;;

	if (first_run) {
		first_run = false;
		config_load();

		if (sysconfig.radio_address == 0) {
			sysconfig.id = generate_node_id();
			sysconfig.radio_address = (uint8_t) sysconfig.id;
			sysconfig.radio_channel = 1;

			config_save();
		}
	}

	return sysconfig.id;
}
#endif

#ifdef ENABLE_NAME

static void handle_echo_reply(ipv6_hdr_t *ipv6_buf, icmpv6_echo_reply_hdr_t *echo_buf) {
	printf("received PONG, id = %d, seq = %d\n", echo_buf->id, echo_buf->seq);
}

static void ping(char* str) {
	static uint16_t id = 0;
	id++;
	str += strlen("ping ");
	int packets = 10;

	ipv6_addr_t* dest = get_ip_by_name(str);
	if (dest == NULL) {
		printf("Unknown node: %s\n", str);
		return;
	}

	icmpv6_register_echo_reply_handler(handle_echo_reply);

	char addr_str[IPV6_MAX_ADDR_STR_LEN];
	ipv6_addr_to_str(addr_str, dest);

	char payload[] = "foobar";

	for (int i = 0; i < packets; ++i) {
		printf("sending %u bytes to %s\n", sizeof payload, addr_str);
		icmpv6_send_echo_request(dest, id, i, payload, sizeof payload);
		vtimer_usleep(1000000);
	}
}
#endif /* ENABLE_NAME */

static void set_id(char* str) {
	str += strlen("set_id ");
	uint16_t id = atoi(str);

	sysconfig.id = id;
	sysconfig.radio_address = (uint8_t) id;

#ifdef ENABLE_NAME
	char* name = strstr(str, " ") + 1;
	if (name != NULL)
		strncpy(sysconfig.name, name, sizeof sysconfig.name);
#endif
	config_save();
}

static void init(char *str) {

	rtc_enable();
	genrand_init(get_node_id());
	sixlowpan_lowpan_init(_trans_type, get_node_id(), 0);

	olsr_init();
}

const shell_command_t shell_commands[] = {
#ifndef INIT_ON_START
	{"init", "start the IP stack with OLSRv2", init},
#endif
	{"routes", "print all known nodes and routes", print_topology_set},
	{"set_id", "", set_id},
#ifdef ENABLE_NAME
	{"ping", "send packets to a node", ping},
#endif
	{NULL, NULL, NULL}
};

int main(void) {
#ifdef INIT_ON_START
	init(0);
#endif

	posix_open(uart0_handler_pid, 0);

	shell_t shell;
	shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

	shell_run(&shell);

	return 0;
}
