#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>

#include "net_bundle_core.h"
#include "../net_ring/net_ring_common.h"
#include "../net_xml/net_xml_config.h"
#include "../net_common/net_common_list.h"
#include "../net_common/net_common_struct.h"
#include "../net_work/net_work.h"

static struct event_base * net_bundle_core_module_globase;
/*for tcp event*/
static struct event net_bundle_core_module_tcpevent;
/*for udp event*/
static struct event net_bundle_core_module_udpevent;

/*cond mutex for async*/
static pthread_mutex_t net_bundle_core_module_mutex;
static pthread_cond_t net_bundle_core_module_cond;

/*send ring*/
static struct net_ring_common *net_bundle_core_module_ring;


static int net_bundle_core_module_inside_sockinit(int fd, int family)
{
	int ret, flags, on = 1;

	/*set nonblock*/
	if((flags = fcntl(fd, F_GETFL, NULL)) < 0)
	{ 
		ret = -1;
		goto out;
	}    
	if(fcntl(fd, F_SETFL, flags|O_NONBLOCK) == -1)
	{
		ret = -1;
		goto out;
	}
	/*set keepalive*/
	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on)))
	{
		ret = -1;
		goto out;
	}

	/*set reuse*/
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on)))
	{
		ret = -1;
		goto out;
	}

	struct net_xml_config_server_node *sernode;
	net_xml_config_module.getserver(&sernode);

	struct sockaddr_in bsin;
	memset(&bsin, 0x0, sizeof(bsin));
	bsin.sin_family = AF_INET;
	bsin.sin_port = htons(sernode->port);
	bsin.sin_addr.s_addr = sernode->bindip;

	/*bind*/
	if((ret = bind(fd, (struct sockaddr *)&bsin, sizeof(struct sockaddr))) != 0)
	{
		ret = -1;	
		printf("net_bundle_core_module_inside_sockinit error in bind.\n");
		goto out;
	}

	if(family == SOCK_STREAM)
	{
		/*tcp listen*/
		listen(fd, 5);
	}

	ret = 0;
out:
	return ret;
}

static void net_bundle_core_module_inside_udp_cb(int fd, short event, void *arg)
{
	int sz;
	struct net_common_ring_node *node;
	node = (struct net_common_ring_node *)malloc(sizeof(struct net_common_ring_node));
	memset(node, 0x0, sizeof(*node));

	node->prot = UDPPROT;
	node->fd = fd;
	if(event & EV_READ)
	{
		node->clilen = sizeof(struct sockaddr_in);
		sz = recvfrom(fd, node->data.request, sizeof(node->data.request), 0, (struct sockaddr *)&(node->cliaddr), &(node->clilen));
		if(sz > 0)
		{
			node->datalen = sz;
			/*push in ring*/
			net_work_module.enqueue((void *)node);
		}
	}
	return;
}

static void net_bundle_core_module_inside_tcp_cb(int fd, short event, void *arg)
{
	printf("-----------have a request in tcp-------------------\n");
	return;
}

static int net_bundle_core_module_inside_loop_init(void)
{
	int ret;
	if(pthread_mutex_init(&net_bundle_core_module_mutex, NULL) ||
		pthread_cond_init(&net_bundle_core_module_cond, NULL))
	{
		ret = -1;	
		goto out;
	}
	if(pthread_mutex_lock(&net_bundle_core_module_mutex))
	{
		ret = -1;	
		goto out;
	}

	ret = 0;
out:
	return ret;
}

static int net_bundle_core_module_inside_write_back(struct net_common_ring_node *item)
{
	uint8_t *pos = (uint8_t *)item->data.request;
	struct net_common_dnsheader *hdr = (struct net_common_dnsheader *)pos;

	/*set QR*/
	hdr->flags |= 0x0080;
	/*set RA*/
	hdr->flags |= 0x08000;
	/*set ancount*/
	hdr->ancount = htons(0x1);

	pos += item->datalen;
	/*set offset*/
	*pos++ = 0xc0;
	*pos++ = 0x0c;
	item->datalen += 2;

	/*set type A*/
	*(uint16_t *)pos = htons(NET_REQUEST_TYPE_A);
	pos += 2;
	item->datalen += 2;

	/*set class*/
	*(uint16_t *)pos = htons(NET_REQUEST_CLASS_IP);
	pos += 2;
	item->datalen += 2;

	/*set TTL*/
	*(uint16_t *)pos = htons(0x80);
	pos += 4;
	item->datalen += 4;

	/*set length*/
	*(uint16_t *)pos = htons(0x4);
	pos += 2;
	item->datalen += 2;

	/*set A Type Result*/
	*(uint32_t *)pos = item->ipv4;
	item->datalen += 4;
	/*send back*/
	int temp;
	temp = sendto(item->fd, item->data.response, item->datalen, 0, (struct sockaddr *)&(item->cliaddr), item->clilen);
	if(temp == -1)
	{
		perror("sendto \n");
	}

	/*release source*/
	free(item);
}

static void *net_bundle_core_module_inside_loop(void *arg)
{
	int ret;
	struct net_common_ring_node *item;

	/*detach thread*/
	pthread_detach(pthread_self());
	/*wait*/
	pthread_cond_wait(&net_bundle_core_module_cond,
			&net_bundle_core_module_mutex);
	pthread_mutex_unlock(&net_bundle_core_module_mutex);
	/*do loop reading ring*/
	while(1)
	{
		ret = net_bundle_core_module.dequeue((void **)&item);
		if(ret)
		{
			usleep(1000);
			continue;
		}
		/*have response*/
		net_bundle_core_module_inside_write_back(item);		
	}
	return NULL;
}

static int net_bundle_core_module_inside_subtask_init()
{
	int ret;
	pthread_t taskid;
	ret = pthread_create(&taskid, NULL, 
			net_bundle_core_module_inside_loop,NULL);
	if(ret)
	{
		ret = -1;	
		goto err;
	}

	ret = 0;
err:
	return ret;
}

static int net_bundle_core_module_inside_init(void)
{
	int tcpfd, udpfd;

	tcpfd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcpfd <= 0)
		goto out;
	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpfd <= 0)
		goto out;

	if(net_bundle_core_module_inside_sockinit(tcpfd, SOCK_STREAM) ||
	net_bundle_core_module_inside_sockinit(udpfd, SOCK_DGRAM))
	{
		printf("net_bundle_core_module_inside_sockinit has an error.\n");
		goto out;
	}

	event_assign(&net_bundle_core_module_tcpevent, 
		net_bundle_core_module_globase,
		tcpfd, EV_READ|EV_PERSIST, 
		net_bundle_core_module_inside_tcp_cb, NULL);

	event_assign(&net_bundle_core_module_udpevent, 
		net_bundle_core_module_globase,
		udpfd, EV_READ|EV_PERSIST, 
		net_bundle_core_module_inside_udp_cb, NULL);

	event_add(&net_bundle_core_module_tcpevent, NULL);
	event_add(&net_bundle_core_module_udpevent, NULL);

	net_bundle_core_module_inside_loop_init();
	net_bundle_core_module_inside_subtask_init();


	return 0;
out:
	if(tcpfd > 0)
		close(tcpfd);
	if(udpfd > 0)
		close(udpfd);
	return -1;
}

static int net_bundle_core_module_init(void)
{
	int ret;

	/*new a base*/
	net_bundle_core_module_globase = event_base_new();
	if(!net_bundle_core_module_globase)
	{
		printf("net_bundle_core_module_init error in event_base_new.\n");	
		ret = -1;
		goto out;
	}
	ret = net_bundle_core_module_inside_init();
	if(ret)
	{
		printf("net_bundle_core_module_init error in net_bundle_core_module_inside_init.\n");
		ret = -1;	
		goto out;
	}

	net_bundle_core_module_ring = net_ring_common_module.create(NET_BUNDLE_RING_SIZE);
	if(!net_bundle_core_module_ring)
	{
		printf("net_bundle_core_modle_ring error .\n");
		ret = -1;
		goto out;
	}

	ret  = 0;
out:
	return ret;
}

static int net_bundle_core_module_run(void)
{
	pthread_mutex_lock(&net_bundle_core_module_mutex);

	pthread_cond_signal(&net_bundle_core_module_cond);

	pthread_mutex_unlock(&net_bundle_core_module_mutex);

	/*loop event*/
	event_base_dispatch(net_bundle_core_module_globase);

	return 0;
}

static int net_bundle_core_module_enqueue(void *node)
{
	return net_ring_common_module.enqueue(net_bundle_core_module_ring, node);
}

static int net_bundle_core_module_dequeue(void **node)
{
	return net_ring_common_module.dequeue(net_bundle_core_module_ring, node);
}


struct net_bundle_core_module net_bundle_core_module = 
{
	.init		= net_bundle_core_module_init,
	.run		= net_bundle_core_module_run,
	.enqueue	= net_bundle_core_module_enqueue,
	.dequeue	= net_bundle_core_module_dequeue,
};
