#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <libxml/xmlreader.h> 

#include "net_xml_config.h"
#include "net_xml_common.h"
#include "../net_hashmap/net_hashmap_rbtree.h"
#include "../net_hashmap/net_hashmap_core.h"

static xmlDocPtr net_xml_config_pdoc;
/*server node structure*/
static struct net_xml_config_server_node net_xml_config_server_node;
/*forward node structure*/
static struct net_xml_config_forward_node net_xml_config_forward_node;


static int net_xml_config_module_init(void)
{
	int ret;
	net_xml_config_pdoc = config_load(NET_XML_CONFIG_FILE_PATH);
	if(!net_xml_config_pdoc)
	{
		ret = -1;	
		goto out;
	}


	ret = 0;
out:
	return ret;
}

static int net_xml_config_module_parser(void)
{
	int ret;
	uint32_t ipv4;

	if(!net_xml_config_pdoc)
	{
		ret = -1;	
		goto out;
	}

	xmlNodePtr ptr;
	/*parse BindSource*/
	ptr = config_search3(net_xml_config_pdoc, "DNSCONFIG", "Server", "BindSource");
	if(ptr)
	{
		const char *ipaddr = config_search_attr_value(ptr, "ipaddr");
		const char *port   = config_search_attr_value(ptr, "port");
		if(ipaddr && strlen(ipaddr) > 0)
			inet_pton(AF_INET, ipaddr, &net_xml_config_server_node.bindip);
		else
			net_xml_config_server_node.bindip = INADDR_ANY;

		if(port && strlen(port) > 0)
			net_xml_config_server_node.port = atoi(port);
		else
			net_xml_config_server_node.port = 53;

		config_attr_free(ipaddr);
		config_attr_free(port);
	}
	/*parse Record*/
	ptr = config_search3(net_xml_config_pdoc, "DNSCONFIG", "Record", "Forward");
	if(ptr)
	{
		const char *ipaddr = config_search_attr_value(ptr, "ipaddr");	
		if(ipaddr && strlen(ipaddr) > 0)
			inet_pton(AF_INET, ipaddr, &net_xml_config_forward_node.forwdip);
		else
			inet_pton(AF_INET, NET_XML_CONFIG_DEFAULT_DNSNAME, \
					&net_xml_config_forward_node.forwdip);

		config_attr_free(ipaddr);
	}
	/*parse root*/
	for(ptr = config_search3(net_xml_config_pdoc, "DNSCONFIG", "Root", "IPSource"); ptr != NULL;\
			ptr = config_search_next(ptr, "IPSource"))
	{
		if(ptr)
		{
			const char *domain = config_search_attr_value(ptr, "Domain");
			const char *ipaddr = config_search_attr_value(ptr, "ipaddr");
			if(domain && strlen(domain) < NET_HASHMAP_DOMAIN_LEN && ipaddr && strlen(ipaddr))
			{
				ipv4 = 0;
				inet_pton(AF_INET, ipaddr, &ipv4);
				net_hashmap_core_module.insert(domain, strlen(domain), ipv4);	
			}

			config_attr_free(ipaddr);
			config_attr_free(domain);
		}	
	}

	ret = 0;
out:
	return ret;
}

static int net_xml_config_module_getserver(struct net_xml_config_server_node **node)
{
	*node = &net_xml_config_server_node;

	return 0;
}

static int net_xml_config_module_release(void)
{
	if(net_xml_config_pdoc)
		config_free(net_xml_config_pdoc);

	return 0;
}

static int net_xml_config_module_getforwd(struct net_xml_config_forward_node **node)
{
	*node = &net_xml_config_forward_node;

	return 0;
}


struct net_xml_config_module net_xml_config_module = 
{
	.init		= net_xml_config_module_init,
	.release	= net_xml_config_module_release,
	.parse		= net_xml_config_module_parser,
	.getserver	= net_xml_config_module_getserver,
	.getforwd	= net_xml_config_module_getforwd
};
