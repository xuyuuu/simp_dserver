#ifndef net_work_h
#define net_work_h

#define NET_WORK_RING_SIZE (1 << 12) 

struct net_work_module 
{
	int (*init)(void);
	int (*run)(void);
	int (*enqueue)(void *node);
	int (*dequeue)(void **node);
};

extern struct net_work_module net_work_module;

#endif
