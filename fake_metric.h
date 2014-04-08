#define METRIC_MAGIC (0x01574C3)
#define METRIC_MAX (128)
struct packet {
	uint32_t magic;
	uint8_t metric;
};
