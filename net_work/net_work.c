#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <linux/socket.h>
#include <linux/types.h>
#include <arpa/inet.h>

#include "net_work.h"
#include "../net_ring/net_ring_common.h"
#include "../net_common/net_common_list.h"
#include "../net_common/net_common_struct.h"
#include "../net_hashmap/net_hashmap_rbtree.h"
#include "../net_hashmap/net_hashmap_core.h"
#include "../net_bundle/net_bundle_core.h"
#include "../net_forward/net_forward.h"

static struct net_ring_common * net_work_module_ring;

/*cond mutex for async*/
static pthread_mutex_t net_work_module_mutex;
static pthread_cond_t net_work_module_cond;

static int net_work_module_inside_parser(struct net_common_ring_node *node)
{
	uint32_t ipaddr;
	int n, ret = 0, len = 0;
	char *pos1 = node->data.request, *pos2 = NULL, *h = node->shost;
	struct net_common_dnsheader *hdr = (struct net_common_dnsheader*)pos1; 

	if((n = ntohs(hdr->qdcount)) != 1)
	{
		ret = -1;
		goto out;
	}

	if((n = ntohs(hdr->ancount)) != 0)
	{
		ret = -1;
		goto out;
	}

	if((n = ntohs(hdr->nscount)) != 0)
	{
		ret = -1;
		goto out;
	}

	if((n = ntohs(hdr->arcount)) > 1 )
	{
		ret = -1;
		goto out;
	}

	node->iden  = hdr->iden;
	node->flags = &(hdr->flags);
	/*parse dns request host*/
	pos2 = (char *)(hdr + 1);
	if(pos2[0] <= 0)
	{
		/*don`t parse root domain*/
		ret = -1;
		goto out;
	}
	int tmp, j;
	while((tmp = *pos2) != 0)
	{
		pos2++;

		for(j = 0; j < tmp; j++)
		{
			*h++ = *pos2++;
			len ++;
		}

		if(*pos2 != 0)
		{
			*h++ = '.';
			len ++;
		}
	}

	/*skip ending zero*/
	pos2++;
	/*only parse ipv4 */
	if(NET_REQUEST_TYPE_A != ntohs(*(uint16_t *)pos2))
	{
		ret = -1;	
		goto out;
	}

	pos2 += 2;

	node->phost = (char *)(hdr + 1);
	node->host_len = len;
	/*search hashmap*/
	ipaddr = net_hashmap_core_module.search(node->shost, node->host_len);
	if(ipaddr == 0)
	{
		/*not find ipaddr in hashmap*/	
		net_forward_module.enqueue(node);
	}
	else
	{
		/*combination response packet*/	
		node->ipv4 = ipaddr;
		net_bundle_core_module.enqueue(node);
	}

out:
	return ret;
}

static void *net_work_module_inside_loop(void* arg)
{
	int ret;
	struct net_common_ring_node *node;
	/*detach*/
	pthread_detach(pthread_self());

	/*wait*/
        pthread_cond_wait(&net_work_module_cond,
                        &net_work_module_mutex);
        pthread_mutex_unlock(&net_work_module_mutex);

	while(1)
	{
		ret = net_work_module.dequeue((void **)&node);
		if(ret)
		{
			usleep(1000);
			continue;
		}
		/*parse data*/
		net_work_module_inside_parser(node);
	}
	return NULL;
}

static int net_work_module_inside_loop_init(void)
{
	int ret;
	pthread_t loopid;
	if(pthread_create(&loopid, NULL, net_work_module_inside_loop, NULL))
	{
		printf("net_work_module_inside_loop_init error in create.\n");			
		ret = -1;
		goto out;
	}


	ret = 0;
out:
	return ret;
}

static int net_work_module_inside_ring_init(void)
{
	int ret;
	net_work_module_ring = net_ring_common_module.create(NET_WORK_RING_SIZE);
	if(!net_work_module_ring)
	{
		ret = -1;	
		printf("net_ring_common_module.create.\n");
		goto out;
	}

	ret = 0;
out:
	return ret;
}

static int net_work_module_inside_mutex_init()
{
	int ret;
        if(pthread_mutex_init(&net_work_module_mutex, NULL) ||
                pthread_cond_init(&net_work_module_cond, NULL))
        {
		printf("net_work_module mutex init error.\n");
                ret = -1;
                goto out;
        }
        if(pthread_mutex_lock(&net_work_module_mutex))
        {
		printf("net_work_module mutex lock error.\n");
                ret = -1;
                goto out;
        }

        ret = 0;
out:
        return ret;
}


static int net_work_module_init(void)
{
	int ret = 0;

	if(net_work_module_inside_ring_init() ||
			net_work_module_inside_mutex_init() ||
			net_work_module_inside_loop_init())
	{
		printf("net_work_module_init error .\n");
		ret = -1;	
	}

	return ret;
}

static int net_work_module_enqueue(void *node)
{
	return net_ring_common_module.enqueue(net_work_module_ring, node);
}

static int net_work_module_dequeue(void **node)
{
	return net_ring_common_module.dequeue(net_work_module_ring, node);	
}

static int net_work_module_run(void)
{
	pthread_mutex_lock(&net_work_module_mutex);
	pthread_cond_signal(&net_work_module_cond);
	pthread_mutex_unlock(&net_work_module_mutex);

	return 0;
}


struct net_work_module net_work_module = 
{
	.init		= net_work_module_init,
	.run		= net_work_module_run,
	.enqueue	= net_work_module_enqueue,
	.dequeue	= net_work_module_dequeue,
};


