#ifdef RIOT
#include <thread.h>
#include <vtimer.h>
#include <socket.h>
#include <inet_pton.h>
#include <sixlowpan/ip.h>
#include <msg.h>
#include <net_help.h>
#include <random.h>
#include <mutex.h>

#include "rfc5444/rfc5444_writer.h"

#include "constants.h"
#include "debug.h"
#include "node.h"
#include "olsr.h"
#include "reader.h"
#include "writer.h"

#if BOARD == native
static uint8_t _trans_type = TRANSCEIVER_NATIVE;
#elif BOARD == msba2
static uint8_t _trans_type = TRANSCEIVER_CC1100;
#endif

static char receive_thread_stack[KERNEL_CONF_STACKSIZE_DEFAULT];
static char sender_thread_stack[KERNEL_CONF_STACKSIZE_DEFAULT];

static struct timer_msg {
	vtimer_t timer;
	timex_t interval;
	void (*func) (void);
};

static struct timer_msg msg_hello = { .timer = {{0}}, .interval = { .seconds = REFRESH_INTERVAL, .microseconds = 0}, .func = writer_send_hello };
static struct timer_msg msg_tc = { .timer = {{0}}, .interval = { .seconds = REFRESH_INTERVAL, .microseconds = 0}, .func = writer_send_tc };

static int sock;
static sockaddr6_t sa_bcast;
static char name[5];

static mutex_t olsr_data;

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	int bytes_send = sendto(sock, buffer, length, 0, &sa_bcast, sizeof sa_bcast);

	DEBUG("write_packet(%d bytes), %d bytes sent", length, bytes_send);
}

static void olsr_receiver_thread(void) {
	char buffer[256];

	sockaddr6_t sa = {0};
	sa.sin6_family = AF_INET6;
	sa.sin6_port = HTONS(MANET_PORT);

	if (bind(sock, &sa, sizeof sa) < 0) {
		printf("Error bind failed!\n");
		close(sock);
	}

	/* wake this thread when a new UDP packet arrives */
	ipv6_register_next_header_handler(IPV6_PROTO_NUM_UDP, thread_getpid());

	int32_t recsize;
	uint32_t fromlen = sizeof sa;

	struct netaddr _src;
	_src._type = AF_INET6;
	_src._prefix_len = 64;

	while (1) {
		recsize = recvfrom(sock, &buffer, sizeof buffer, 0, &sa, &fromlen);

		memcpy(&_src._addr, &sa.sin6_addr, sizeof _src._addr);
		DEBUG("received %d bytes from %s", recsize, netaddr_to_str_s(&nbuf[0], &_src));

		mutex_lock(&olsr_data);
		reader_handle_packet(&buffer, recsize, &_src);
		mutex_unlock(&olsr_data);
	}
}

static void olsr_sender_thread() {
	DEBUG("olsr_sender_thread, pid %d\n", thread_getpid());

	while (1) {
		msg_t m;
		msg_receive(&m);
		struct timer_msg* tmsg = (struct timer_msg*) m.content.ptr;

		mutex_lock(&olsr_data);
		tmsg->func();
		mutex_unlock(&olsr_data);

		if (vtimer_set_msg(&tmsg->timer, tmsg->interval, thread_getpid(), tmsg) != 0)
			DEBUG("something went wrong");
	}
}

static void init_random(void) {
	timex_t now;
    vtimer_now(&now);
    genrand_init(now.microseconds);
    DEBUG("starting at %u", now.microseconds);
}

static char* gen_name(char* dest, const size_t len) {
	int i;
	for (i = 0; i < len - 1; ++i)
		dest[i] = 'A' +  genrand_uint32() % ('Z' - 'A');
	dest[i] = '\0';
	return dest;
}

static void ip_init(void) {
	uint8_t hw_addr = genrand_uint32() % 256;

	sixlowpan_lowpan_init(_trans_type, hw_addr, 0);

	/* we always send to the same broadcast address, prepare it once */
	sa_bcast.sin6_family = AF_INET6;
	sa_bcast.sin6_port = HTONS(MANET_PORT);
	ipv6_addr_set_all_nodes_addr(&sa_bcast.sin6_addr);

	sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	thread_create(receive_thread_stack, sizeof receive_thread_stack, PRIORITY_MAIN-1, CREATE_STACKTEST, olsr_receiver_thread, "olsr_rec");

	local_addr->_type = AF_INET6;
	local_addr->_prefix_len = 64;
	ipv6_iface_get_best_src_addr(&local_addr->_addr, &sa_bcast.sin6_addr);
}

static void init_sender() {
	int pid = thread_create(sender_thread_stack, sizeof sender_thread_stack, PRIORITY_MAIN-1, CREATE_STACKTEST, olsr_sender_thread, "olsr_snd");

	msg_t m;
	DEBUG("setting up HELLO timer");
	m.content.ptr = (char*) &msg_hello;
	msg_send(&m, pid, false);

	sleep_s(1);
	DEBUG("setting up TC timer");
	m.content.ptr = (char*) &msg_tc;
	msg_send(&m, pid, false);
}

int main(void) {

	init_random();
#ifdef ENABLE_DEBUG
	local_name = gen_name(&name, sizeof name);
#endif
	node_init();
	mutex_init(&olsr_data);
	reader_init();
	writer_init(write_packet);
	ip_init();
	init_sender();

	DEBUG("This is node %s with IP %s",
		local_name, netaddr_to_str_s(&nbuf[0], local_addr));
}

#endif /* RIOT */
