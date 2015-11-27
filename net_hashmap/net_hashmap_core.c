#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "../net_common/net_common_list.h"
#include "net_hashmap_rbtree.h"
#include "net_hashmap_core.h"

static struct rb_root net_hashmap_array[NET_HASHMAP_ARRAY_SIZE];
static pthread_rwlock_t net_hashmap_lck[NET_HASHMAP_ARRAY_SIZE];

static int net_hashmap_core_module_inside_hash(const char *str, int len)
{
	const char *p = str;
	uint32_t hash = 5381;

	while (len--)
		hash = (((hash << 5) + hash) + *p++);

	return (hash & NET_HASHMAP_ARRAY_MASK);	
}

static int net_hashmap_core_module_inside_strcmp(const void *str1, const void *str2)
{
	char *p1 = (char *)str1, *p2 = (char *)str2;
	int to = NET_HASHMAP_DOMAIN_LEN;

	if (p1 == NULL)
		return -1;
	if (p2 == NULL)
		return 1;
	while(*p1 != 0 && *p2 != 0)
	{
		if (*p1 > *p2)
			return 1;
		if (*p1 < *p2)
			return -1;
		p1++;
		p2++;
		if(to-- == 0) 
			return -1;
	}

	if(*p1 == 0 && *p2 == 0)
		return 0;

	if(*p1 == 0)
		return -1;

	return 1;
}

static int net_hashmap_core_module_init(void)
{
	int i;
	for(i = 0; i < NET_HASHMAP_ARRAY_SIZE; i++)
	{
		memcpy(&net_hashmap_array[i], &RB_ROOT, sizeof(struct rb_root));
		pthread_rwlock_init(&net_hashmap_lck[i], NULL);
	}

	return 0;
}

static int net_hashmap_core_module_inside_insert(int h, struct rb_node *new)
{
	struct rb_root *root = &net_hashmap_array[h];
	struct rb_node **tmp, *parent = NULL;
	tmp = &root->rb_node;

	struct net_hashmap_node *newnode = NULL, *oldnode = NULL;
	newnode = (struct net_hashmap_node *)container_of(new, struct net_hashmap_node, rbnode);

	while(*tmp)
	{
		parent = *tmp;
		oldnode = (struct net_hashmap_node *)container_of(*tmp, struct net_hashmap_node, rbnode);

		if(net_hashmap_core_module_inside_strcmp(newnode->domain, oldnode->domain) > 0)
			tmp = &(((struct rb_node *)*tmp)->rb_left);
		else if(net_hashmap_core_module_inside_strcmp(newnode->domain, oldnode->domain) < 0)
			tmp = &(((struct rb_node *)(*tmp))->rb_right);
		else
			return -1;
	}

	/*init & insert*/
	rb_link_node(new, parent, tmp);
	rb_insert_color(new, root);

	return 0;
}

static int net_hashmap_core_module_insert(const char *domain, int domain_len, uint32_t ipv4)
{
	int ret = 0, h;
	struct net_hashmap_node *node = NULL;
	node = (struct net_hashmap_node *)malloc(sizeof(struct net_hashmap_node));
	if(!node)
	{
		ret = -1;
		goto out;
	}
	memset(node, 0x0, sizeof(struct net_hashmap_node));

	strncpy(node->domain, domain, domain_len);
	node->domain_len = domain_len;
	node->ipv4 = ipv4;

	h = net_hashmap_core_module_inside_hash(domain, domain_len);
	pthread_rwlock_wrlock(&net_hashmap_lck[h]);
	ret = net_hashmap_core_module_inside_insert(h, &node->rbnode);	
	pthread_rwlock_unlock(&net_hashmap_lck[h]);

out:
	return ret;
}


static uint32_t net_hashmap_core_module_inside_search(int h, const char *domain)
{
	uint32_t ipv4;
	struct rb_root *root = &net_hashmap_array[h];
	struct rb_node *tmp = root->rb_node;
	struct net_hashmap_node * node = NULL;

	while(tmp)
	{
		node = (struct net_hashmap_node *)container_of(tmp, struct net_hashmap_node, rbnode);
		if(net_hashmap_core_module_inside_strcmp(domain, node->domain) > 0)
			tmp = tmp->rb_left;
		else if(net_hashmap_core_module_inside_strcmp(domain, node->domain) < 0)
			tmp = tmp->rb_right;
		else
			return node->ipv4;
	}

	ipv4 = 0;	
	return ipv4;
}

static uint32_t net_hashmap_core_module_search(const char *domain, int domain_len)
{
	int h;
	h = net_hashmap_core_module_inside_hash(domain, domain_len);
	uint32_t ipv4;

	pthread_rwlock_rdlock(&net_hashmap_lck[h]);
	ipv4 = net_hashmap_core_module_inside_search(h, domain);	
	pthread_rwlock_unlock(&net_hashmap_lck[h]);

	return ipv4;
}

static int net_hashmap_core_module_earse(void)
{

	return 0;
}

static int net_hashmap_core_module_destroy(void)
{

	return 0;
}


struct net_hashmap_core_module net_hashmap_core_module =
{
	.init		= net_hashmap_core_module_init,
	.insert		= net_hashmap_core_module_insert,
	.search		= net_hashmap_core_module_search,
	.earse		= net_hashmap_core_module_earse,
	.destroy	= net_hashmap_core_module_destroy,
};

