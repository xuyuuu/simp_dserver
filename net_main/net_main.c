#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/socket.h>
#include <arpa/inet.h>

#include "../net_common/net_common_list.h"
#include "../net_common/net_common_struct.h"
#include "../net_bundle/net_bundle_core.h"
#include "../net_xml/net_xml_config.h"
#include "../net_work/net_work.h"
#include "../net_forward/net_forward.h"
#include "../net_hashmap/net_hashmap_rbtree.h"
#include "../net_hashmap/net_hashmap_core.h"

void help(const char *program)
{
	printf("usage:\n\
-d run [%s] in background\n\
-h show help\n", program);
}

void show_version(const char *program)
{
	printf("\n%s version : V0.0.1\n", program);
}

int main(int argc, char **argv)
{
	int opt, dmon = 0;
	while((opt = getopt(argc, argv, "dhv")) != -1)
	{
		switch (opt)
		{
			case 'd':		
				dmon = 1;
				break;
			case 'h':
				help(argv[0]);
				return 0;
			case 'v':
				show_version(argv[0]);
				return 0;
			default:
				break;

		}
	}

	if(dmon)
		daemon(1, 1);

	printf("\nnetdispatch is running now.......\n\n");

	net_hashmap_core_module.init();

	net_xml_config_module.init();
	net_xml_config_module.parse();

	net_work_module.init();
	net_work_module.run();

	net_forward_module.init();
	net_forward_module.run();

	net_bundle_core_module.init();
	net_bundle_core_module.run();

	return 0;
}
