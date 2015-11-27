#ifndef net_forward_h
#define net_forward_h

#define NET_FORWARD_DEFLATE_MASK1 0x80
#define NET_FORWARD_DEFLATE_MASK2 0x40

#define NET_FORWARD_RING_SIZE (1 << 12) 

#define NET_FORWARD_LIST_SIZE (1 << 12)
#define NET_FORWARD_LIST_MASK (NET_FORWARD_LIST_SIZE - 1)

#define NET_FORWARD_HASH_ROUND 0x10000
#define NET_FORWARD_HASH_RADIO 0x9e3779b9
#define __NET_HASH_MIX(a, b, c) do { \
        a -= b; a -= c; a ^= (c>>13); \
        b -= c; b -= a; b ^= (a<<8); \
        c -= a; c -= b; c ^= (b>>13); \
        a -= b; a -= c; a ^= (c>>12); \
        b -= c; b -= a; b ^= (a<<16); \
        c -= a; c -= b; c ^= (b>>5); \
        a -= b; a -= c; a ^= (c>>3); \
        b -= c; b -= a; b ^= (a<<10); \
        c -= a; c -= b; c ^= (b>>15); \
} while (0)

struct net_forward_module 
{
	int (*init)(void);
	int (*run)(void);
	int (*enqueue)(void *node);
	int (*dequeue)(void **node);
};

extern struct net_forward_module net_forward_module;

#endif
