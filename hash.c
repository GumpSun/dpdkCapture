#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "hash.h"
#include "redis.h"

#include <dpdk/rte_lcore.h>
#include <dpdk/rte_log.h>
#include <dpdk/rte_tcp.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_ether.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_ip.h>
#include <dpdk/rte_eal.h>

#define RTE_LOGTYPE_DPDKCAP RTE_LOGTYPE_USER1

/* 计算hashtable下标 */
int getIndex(uint32_t src,uint32_t dst){
        uint32_t index = src+dst;
        return ((index>>16) + (index&65535))&65535;
}

/* 增加一个数据包 */
int addPacket(struct info **table,struct rte_mbuf *pkt){
	struct ipv4_hdr *ip;
	struct ether_hdr *eth_hdr = rte_pktmbuf_mtod_offset(pkt, struct ether_hdr *,0);
	ip = rte_pktmbuf_mtod_offset(pkt, struct ipv4_hdr *,sizeof(struct ether_hdr));
	/* 处理IPv4数据包 */
	if (eth_hdr->ether_type == 0x0008) {
		int index = getIndex(ip->src_addr,ip->dst_addr);
		struct info *p;
		/* 查找hash table中该数据包对应的节点 */
		if(table[index]==NULL){
			p = (struct info *)malloc(sizeof(struct info));
			p->src = ip->src_addr;
			p->dst = ip->dst_addr;
                        p->icmp = 0;
                        p->tcp = 0;
                        p->http = 0;
                        p->udp = 0;
                        p->syn = 0;
                        p->ack = 0;
                        p->count = 0;
                        p->other = 0;
			p->next = NULL;
			p->time = time(0);
			table[index] = p;
		}else{
			p = table[index];
			while(p->next != NULL){
				if(p->src == ip->src_addr && p->dst == ip->dst_addr){
					break;
				}
				p = p->next;
			}
			/* p是尾节点并且和当前数据包信息不匹配 */
			if(p->next != NULL){
				if(p->src != ip->src_addr || p->dst != ip->dst_addr){
					p->next =  (struct info *)malloc(sizeof(struct info));
					p = p->next;
					p->src = ip->src_addr;
					p->dst = ip->dst_addr;
					p->icmp = 0;
					p->tcp = 0;
					p->http = 0;
					p->udp = 0;
					p->syn = 0;
					p->ack = 0;
					p->count = 0;
					p->other = 0;
					p->time = time(0);
					p->next = NULL;
				}
			}
		}
		p->count += pkt->data_len;
		/* 处理tcp数据包 */
		if(ip->next_proto_id ==6){
			p->tcp++;
		struct tcp_hdr *tcp = rte_pktmbuf_mtod_offset(pkt, struct tcp_hdr *,
						   sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
		if(ntohs(tcp->src_port) == 80 || ntohs(tcp->src_port) == 443 || ntohs(tcp->src_port) == 8080 || ntohs(tcp->dst_port)==443 || ntohs(tcp->dst_port)==80 || ntohs(tcp->dst_port)==8080){
			p->http++;
		}
			if(tcp->tcp_flags & 2){
				p->syn++;
			}else if(tcp->tcp_flags & 16){
				p->ack++;
			}
		/* 处理udp数据包 */
		}else if(ip->next_proto_id ==17){
			p->udp++;
		/* 处理icmp数据包 */
		}else if(ip->next_proto_id ==1){
			p->icmp++;
		/* 其它协议的包 */
		}else{
			p->other++;
		}
	} else if (RTE_ETH_IS_IPV6_HDR(pkt->packet_type)) {
		/* Fill acl structure */

	} else {
		/* Unknown type, drop the packet */
	}
	return 0;
}

void initTable(struct info **table){
	int i;
	for(i=0;i<HASH_TABLE_SIZE;i++){
		table[i]=NULL;
	}
}

