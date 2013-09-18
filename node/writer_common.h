typedef void (*write_packet_func_ptr)(
	struct rfc5444_writer *wr, struct rfc5444_writer_target *iface, void *buffer, size_t length);

/* we never create two packages at once, so this is save */
uint8_t msg_buffer[128];
uint8_t msg_addrtlvs[1000];
uint8_t packet_buffer[128];