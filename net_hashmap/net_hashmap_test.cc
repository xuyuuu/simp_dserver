#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "../net_common/net_common_list.h"
#include "net_hashmap_rbtree.h"
#include "net_hashmap_core.h"

struct testnode
{
	int len;
	char domain[512];
	uint32_t ipv4;
} test[512] = 
{
	{13 ,"www.baidu.com", 1000000},
	{15 , "www.sohu.com.cn", 1000001},
	{15 , "www.sina.com.cn", 1000002},
	{14 , "www.hao123.com", 1000003},
	{10 , "www.360.cn", 1000004},
	{14 , "www.taobao.com", 1000005},

};


int main(int argc, char **argv)
{
	uint32_t i, ipaddr;
	net_hashmap_core_module.init();

	for(i = 0; i < 6; i++)
	{
		net_hashmap_core_module.insert(test[i].domain, test[i].len, test[i].ipv4);	
	}

	ipaddr = net_hashmap_core_module.search("www.sina.com.cn", 15);
	printf("search ip: %u\n", ipaddr);

	return 0;
}
