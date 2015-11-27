#ifndef net_bundle_core_h
#define net_bundle_core_h

#define NET_BUNDLE_RING_SIZE (1 << 12)

struct net_bundle_core_module
{
	int (* init)(void);
	int (* run)(void);
	int (*enqueue)(void *node);
	int (*dequeue)(void **node);
};



extern struct net_bundle_core_module net_bundle_core_module;

#endif
