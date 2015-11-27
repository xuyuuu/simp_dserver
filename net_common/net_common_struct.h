#ifndef net_common_struct_h
#define net_common_struct_h

#define NET_REQ_DATA_LEN 2048
#define NET_HOST_DATA_LEN 512

#define NET_REQUEST_TYPE_A 1
#define NET_REQUEST_TYPE_CNAME 5

#define NET_REQUEST_CLASS_IP 1

#define UDPPROT 0
#define TCPPROT 1

struct net_common_ring_node
{
	/*hook*/
	struct list_head node;

	int fd, prot;
	/*network structure*/
	struct sockaddr_in cliaddr, seraddr; 
	socklen_t clilen;

	int datalen;
	union data_t
	{
		char request[NET_REQ_DATA_LEN];
		char response[NET_REQ_DATA_LEN];
	}data;

	char *header; /*a pointer to header*/
	char *phost; /*a pointer to request host*/
	char shost[NET_HOST_DATA_LEN];
	int host_len;

	uint16_t iden;
	uint16_t *flags;/*a pointer to request flags*/
	uint32_t ipv4;
}__attribute__((packed));

struct net_common_dnsheader
{
	uint16_t iden, flags;
	uint16_t qdcount, ancount, nscount, arcount;
}__attribute__((packed));


#endif
