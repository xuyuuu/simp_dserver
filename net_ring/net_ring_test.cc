#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <pthread.h>

#include "net_ring_common.h"

char *spattern = "abcdefghijklmnopqrstuvwxyz0123456789";
struct tqueue
{
	int i;
	char name[6];
};

void * enqueue(void *arg)
{
	int i, ret;
	struct tqueue *node;
	struct net_ring_common *ring = (struct net_ring_common *)arg;

	while(1)
	{
		node = (struct tqueue *)malloc(sizeof(struct tqueue));
		if(!node)
		{
			sleep(1);
			continue;
		}
		memset(node, 0x0, sizeof(struct tqueue));

		for(i = 0; i < 5; i++)
		{
			node->name[i] = spattern[rand() % 36];
		}
		node->i = rand();
		ret = net_ring_common_module.enqueue(ring, node);
		if(ret != 0)
		{
			printf("--------------enqueue error-----------\n");	
			usleep(1000);
			continue;
		}
	}

	return 0;
}

void *dequeue(void *arg)
{
	int ret;
	struct tqueue *node;
	struct net_ring_common *ring = (struct net_ring_common *)arg;

	while(1)
	{
		node = NULL;	
		ret = net_ring_common_module.dequeue(ring, (void **)&node);
		if(ret != 0)
		{
			printf("--------------dequeue error-----------\n");	
			usleep(1000);
			continue;
		}
		if(node != NULL)
		{
			printf("%d  %s\n", node->i, node->name);
			free(node);
		}
	}

	return 0;
}


int main(int argc, char **argv)
{
	int i;
	pthread_t ep[5]; 
	pthread_t dp[5];
	struct net_ring_common * ring;
	ring = net_ring_common_module.create(10240);
	if(!ring)
	{
		printf("net_ring_common_module.create error .\n");	
		return -1;
	}

	for(i = 0; i < 5; i++)
	{
		pthread_create(&ep[i], NULL, enqueue, (void *)ring);
		pthread_create(&dp[i], NULL, dequeue, (void *)ring);
	}
	for(i = 0; i < 5; i++)
	{
		pthread_join(ep[i], NULL);	
		pthread_join(dp[i], NULL);	
	}

	return 0;
}
