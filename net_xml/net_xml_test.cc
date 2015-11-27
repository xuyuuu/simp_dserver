#include <stdio.h>
#include <stdint.h>

#include "net_xml_config.h"

int main(int argc, char **argv)
{
	struct net_xml_config_server_node *sernode;
	net_xml_config_module.init();	
	net_xml_config_module.parse();

	net_xml_config_module.getserver(&sernode);

	printf("ipaddr: %u  port : %u\n", sernode->bindip, sernode->port);

	net_xml_config_module.release();

	return 0;
}
