#include "writer_common.h"

void writer_common_init(write_packet_func_ptr ptr) {
	interface.packet_buffer = packet_buffer;
	interface.packet_size	= sizeof(packet_buffer);
	interface.sendPacket	= ptr;
}
