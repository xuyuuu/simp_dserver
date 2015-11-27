#ifndef net_ring_common_h
#define net_ring_common_h

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define NET_RING_COMMON_X86_ALIGN_SIZE 64
#define NET_RING_COMMON_X86_ALIGN_MASK (NET_RING_COMMON_X86_ALIGN_SIZE - 1)

#define NET_RING_COMMON_COMPILER_BARRIER() do {\
	        asm volatile ("" : : : "memory");\
} while(0)

typedef struct net_ring_common net_ring_common_t;

struct net_ring_common_module
{
	net_ring_common_t *(* create)(uint32_t nem);
	int (* enqueue)(net_ring_common_t *ring, void *item);
	int (* dequeue)(net_ring_common_t *ring, void **item);
	int (* destroy)(void);
}__attribute__((packed));


struct net_ring_common
{
	/*net_ring_common producer status*/
	struct producer
	{
		uint32_t watermark;
		uint32_t sp_enqueue;
		uint32_t size;
		uint32_t mask;

		volatile uint32_t head;
		volatile uint32_t tail;
	}prod __attribute__((__aligned__(64)));

	/*net_ring_common consumer status */
	struct consumer
	{
		uint32_t sc_dequeue;	
		uint32_t size;
		uint32_t mask;

		volatile uint32_t head;
		volatile uint32_t tail;
	}cons __attribute__((__aligned__(64)));

	void *ring[0];
}__attribute__((packed));

extern struct net_ring_common_module net_ring_common_module;

#endif

