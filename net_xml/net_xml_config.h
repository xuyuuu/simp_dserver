#ifndef net_xml_config_h
#define net_xml_config_h

#define NET_XML_CONFIG_FILE_PATH "../DNS.conf"

#define NET_XML_CONFIG_DEFAULT_DNSNAME "8.8.8.8"

struct net_xml_config_server_node
{
	uint32_t bindip; /*BE*/
	uint16_t port; /*LE*/

}__attribute__((__aligned__(64)));

struct net_xml_config_forward_node
{
	uint32_t forwdip; /*BE*/
}__attribute__((__aligned__(64)));

struct net_xml_config_module
{
	int (* init)(void);
	int (* release)(void);
	int (* parse)(void);
	int (* getserver)(struct net_xml_config_server_node **);
	int (* getforwd)(struct net_xml_config_forward_node **);
};

extern struct net_xml_config_module net_xml_config_module;

#endif
