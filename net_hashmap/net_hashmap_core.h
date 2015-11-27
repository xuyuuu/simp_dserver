#ifndef net_hashmap_core_h
#define net_hashmap_core_h

#define NET_HASHMAP_ARRAY_SIZE (1 << 10)
#define NET_HASHMAP_ARRAY_MASK (NET_HASHMAP_ARRAY_SIZE - 1)

#define NET_HASHMAP_DOMAIN_LEN 512

struct net_hashmap_node
{
	struct rb_node rbnode;

	int domain_len;
	char domain[NET_HASHMAP_DOMAIN_LEN];	

	uint32_t ipv4;
};

struct net_hashmap_core_module
{
	int (* init)(void);
	int (* insert)(const char *, int, uint32_t);
	uint32_t (* search)(const char *, int);
	int (* earse)(void);
	int (* destroy)(void);
};

extern struct net_hashmap_core_module net_hashmap_core_module;

#endif
