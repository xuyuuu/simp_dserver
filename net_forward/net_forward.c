#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <linux/socket.h>
#include <linux/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>

#include "../net_ring/net_ring_common.h"
#include "../net_common/net_common_list.h"
#include "../net_common/net_common_struct.h"
#include "../net_hashmap/net_hashmap_rbtree.h"
#include "../net_hashmap/net_hashmap_core.h"
#include "../net_bundle/net_bundle_core.h"
#include "../net_xml/net_xml_config.h"
#include "net_forward.h"

/*forward ring*/
static struct net_ring_common * net_forward_module_ring;
/*forward list*/
struct list_head net_forward_module_list_head[NET_FORWARD_LIST_SIZE];

static int net_forward_module_fudpfd;
static struct sockaddr_in net_forward_module_seraddr;
static struct event_base* net_forward_module_evbase;

/*for udp event*/
static struct event net_forward_module_fudpevent;

/*cond mutex for async*/
static pthread_mutex_t net_forward_module_mutex;
static pthread_cond_t net_forward_module_cond;

static int net_forward_module_inside_integer_hash(uint16_t iden)
{
	uint32_t hnum = iden;
	uint32_t a, b, c;

	a = iden + NET_FORWARD_HASH_RADIO;
	b = NET_FORWARD_HASH_RADIO;
	c = NET_FORWARD_HASH_ROUND;

	__NET_HASH_MIX(a, b, c);

	return (c & NET_FORWARD_LIST_MASK);
}

static void net_forward_module_inside_gethostaddr(struct net_common_ring_node *node)
{
	int ret, sz, h;
	//(*node->flags) |=  0x0100; /*set RD*/
	(*node->flags) |=  0x1; /*set RD*/

	/*push into list*/
	h = net_forward_module_inside_integer_hash(node->iden);
	list_add_tail(&node->node, &net_forward_module_list_head[h]);

	sendto(net_forward_module_fudpfd, node->data.request, node->datalen, 0,\
			(struct sockaddr *)&net_forward_module_seraddr, sizeof(net_forward_module_seraddr));
}


static int net_forward_module_inside_parse_response(char *buff, int bufflen)
{
	uint16_t iden;
	int len = bufflen, h, reslen;
	uint8_t *pos = buff;

	struct net_common_ring_node *item = NULL, *nxt = NULL;
	struct net_common_dnsheader *hdr = (struct net_common_dnsheader *)pos;
	iden = hdr->iden;

	h = net_forward_module_inside_integer_hash(iden);
	list_for_each_entry_safe(item, nxt, &net_forward_module_list_head[h], node)
	{
		if(item->iden == iden)	
		{
			list_del(&item->node);
			break;
		}
	}

	/*skip request*/	
	pos += sizeof(struct net_common_dnsheader) + item->host_len + 6;
	len -= (sizeof(struct net_common_dnsheader) + item->host_len + 6);
	if(item)
	{
next:
		if((int)(pos - (uint8_t *)buff) >= bufflen)
			goto success;

		/*is deflate*/
		if((*pos & NET_FORWARD_DEFLATE_MASK1) && (*pos & NET_FORWARD_DEFLATE_MASK2))	
		{
			pos += 10;
			reslen = ntohs(*(uint16_t *)pos);
			if(reslen == 4)
			{
				pos += 2;	
				item->ipv4 = *(uint32_t *)pos;
				goto success;
			}
			else
			{
				pos += (reslen + 2);	
				goto next;
			}
		}
		else /*is not deflate*/

		{
			while( *pos++ != 0);
			/*skip response type class ttl */
			pos += 8;
			reslen = ntohs(*(uint16_t *)pos);
			if(reslen == 4)
			{
				pos += 2;	
				item->ipv4 = *(uint32_t *)pos;
				goto success;
			}
			else
			{
				pos += (reslen + 2);	
				goto next;
			}
		}

success:
		/*insert into hashmap*/
		net_hashmap_core_module.insert(item->shost, item->host_len, item->ipv4);	
		/*push into ring*/
		net_bundle_core_module.enqueue(item);
	}

}

static void net_forward_module_inside_udp_cb(int fd, short event, void *arg)
{
	int h, sz;
	char buff[2048];

	int clilen;
	struct sockaddr_in cliaddr;

	if(event & EV_READ)
	{
		/*data receive*/
		sz = recvfrom(fd, buff, sizeof(buff), 0, (struct sockaddr *)&cliaddr, &clilen);
		net_forward_module_inside_parse_response(buff, sz);
	}
}

static void *net_forward_module_inside_loop(void* arg)
{
	int ret;
	struct timeval tv;
	struct net_common_ring_node *node;
	/*detach*/
	pthread_detach(pthread_self());

	/*wait*/
        pthread_cond_wait(&net_forward_module_cond,
                        &net_forward_module_mutex);
        pthread_mutex_unlock(&net_forward_module_mutex);

	tv.tv_sec = 0; 
	tv.tv_usec = 1000;

	while(1)
	{
		ret = net_forward_module.dequeue((void **)&node);
		if(!ret)
		{
			/*get host addr*/
			net_forward_module_inside_gethostaddr(node);
		}

		/*loop epoll*/
		event_base_loop(net_forward_module_evbase, 0);
		event_add(&net_forward_module_fudpevent, &tv);
	}
	return NULL;
}

static int net_forward_module_inside_loop_init(void)
{
	int ret;
	pthread_t loopid;
	if(pthread_create(&loopid, NULL, net_forward_module_inside_loop, NULL))
	{
		printf("net_forward_module_inside_loop_init error in create.\n");			
		ret = -1;
		goto out;
	}


	ret = 0;
out:
	return ret;
}

static int net_forward_module_inside_ring_init(void)
{
	int ret;
	net_forward_module_ring = net_ring_common_module.create(NET_FORWARD_RING_SIZE);
	if(!net_forward_module_ring)
	{
		ret = -1;	
		printf("net_ring_common_module.create.\n");
		goto out;
	}

out:
	return ret;
}

static int net_forward_module_inside_mutex_init()
{
	int ret;
        if(pthread_mutex_init(&net_forward_module_mutex, NULL) ||
                pthread_cond_init(&net_forward_module_cond, NULL))
        {
                ret = -1;
                goto out;
        }
        if(pthread_mutex_lock(&net_forward_module_mutex))
        {
                ret = -1;
                goto out;
        }

        ret = 0;
out:
        return ret;
}

static int net_forward_module_inside_sock_init()
{
	int ret;
	struct net_xml_config_forward_node *node;

	net_forward_module_fudpfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(net_forward_module_fudpfd <= 0)
	{
		ret = -1;	
		goto out;
	}

	net_xml_config_module.getforwd(&node);
	net_forward_module_seraddr.sin_family = AF_INET;
	net_forward_module_seraddr.sin_port = htons(53);
	net_forward_module_seraddr.sin_addr.s_addr = node->forwdip;

	net_forward_module_evbase = event_base_new();
	if(!net_forward_module_evbase)
	{
		close(net_forward_module_fudpfd);	
		ret = -1;
		goto out;
	}

	 event_assign(&net_forward_module_fudpevent, 
                net_forward_module_evbase,
                net_forward_module_fudpfd, EV_READ, 
                net_forward_module_inside_udp_cb, NULL);


	ret = 0;
out:
	return ret;
}

static int net_forward_module_inside_list_init(void)
{
	int i;
	for(i = 0; i < NET_FORWARD_LIST_SIZE; i++)
	{
		INIT_LIST_HEAD(&net_forward_module_list_head[i]);
	}
	return 0;
}


static int net_forward_module_init(void)
{
	int ret = 0;

	if(net_forward_module_inside_sock_init() ||
		net_forward_module_inside_ring_init() ||
		net_forward_module_inside_list_init() ||
		net_forward_module_inside_mutex_init() ||
		net_forward_module_inside_loop_init())
	{
		printf("net_forward_module_init error .\n");
		ret = -1;	
	}

	return ret;
}

static int net_forward_module_enqueue(void *node)
{
	return net_ring_common_module.enqueue(net_forward_module_ring, node);
}

static int net_forward_module_dequeue(void **node)
{
	return net_ring_common_module.dequeue(net_forward_module_ring, node);	
}

static int net_forward_module_run(void)
{
	pthread_mutex_lock(&net_forward_module_mutex);
	pthread_cond_signal(&net_forward_module_cond);
	pthread_mutex_unlock(&net_forward_module_mutex);

	return 0;
}


struct net_forward_module net_forward_module = 
{
	.init		= net_forward_module_init,
	.run		= net_forward_module_run,
	.enqueue	= net_forward_module_enqueue,
	.dequeue	= net_forward_module_dequeue,
};


