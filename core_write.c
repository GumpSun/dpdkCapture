#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>

#include <dpdk/rte_ring.h>
#include <dpdk/rte_lcore.h>
#include <dpdk/rte_log.h>
#include <dpdk/rte_mbuf.h>
#include <dpdk/rte_branch_prediction.h>
#include <dpdk/rte_tcp.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_ether.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_ip.h>
#include <dpdk/rte_eal.h>

#include "core_write.h"
#include "redis.h"
#define MIN(a,b) (((a)<(b))?(a):(b))

#define RTE_LOGTYPE_DPDKCAP RTE_LOGTYPE_USER1



/*
static inline void
prepare_one_packet(struct rte_mbuf *pkts_in,struct redisStatus *rst)
{
	struct ipv4_hdr *ip;
	struct rte_mbuf *pkt = pkts_in;
	struct ether_hdr *eth_hdr = rte_pktmbuf_mtod_offset(pkt, struct ether_hdr *,0);
	ip = rte_pktmbuf_mtod_offset(pkt, struct ipv4_hdr *,sizeof(struct ether_hdr));
	if (eth_hdr->ether_type == 0x0008) {
	time_t tt = time(0);
	
        //检测时间，如果当前时间大于currentTime更新redis状态
	if(tt > rst->currentTime){
		updateRedisStatus(rst);
	}
	rst->count += pkts_in->data_len;
	if(ip->next_proto_id ==6){
		rst->tcp++;
		struct tcp_hdr *tcp = rte_pktmbuf_mtod_offset(pkt, struct tcp_hdr *,
						   sizeof(struct ether_hdr)+sizeof(struct ipv4_hdr));
		if(ntohs(tcp->src_port) == 80 || ntohs(tcp->src_port) == 443 || ntohs(tcp->src_port) == 8080 || ntohs(tcp->dst_port)==443 || ntohs(tcp->dst_port)==80 || ntohs(tcp->dst_port)==8080){
			rst->http++;
		}
		if(tcp->tcp_flags & 2){
			rst->syn++;
		}else if(tcp->tcp_flags & 16){
			rst->ack++;
		}
	}else if(ip->next_proto_id ==17){
		rst->udp++;
	}else if(ip->next_proto_id ==1){
		rst->icmp++;
	}else{
		rst->other++;
	}
	} else if (RTE_ETH_IS_IPV6_HDR(pkt->packet_type)) {
		// Fill acl structure 

	} else {
		// Unknown type, drop the packet 
	}
}

*/

/*
 * process the packets form the write ring into redis
 */
int write_core(const struct core_write_config * config) {
  unsigned int packet_length, wire_packet_length = 0;
  int result;
  void* dequeued[DPDKCAP_WRITE_BURST_SIZE];
  struct rte_mbuf* bufptr;
  int retval = 0;
  //Init current time
  int i = 0;
  //Init stats
  *(config->stats) = (struct core_write_stats) {
    .core_id=rte_lcore_id(),
      .packets = 0,
      .bytes = 0,
      .compressed_bytes = 0,
  };


  //Log
  RTE_LOG(INFO, DPDKCAP, "Core %d is process packet.\n",
      rte_lcore_id());

  for (;;) {
    if (unlikely(*(config->stop_condition))) {
      break;
    }

    //Get packets from the ring
    result = rte_ring_dequeue_burst(config->ring,
        dequeued, DPDKCAP_WRITE_BURST_SIZE);
    if (result == 0) {
      continue;
    }

    //Update stats
    config->stats->packets += result;
    for (i = 0; i < result; i++) {
      //Cast to packet
      bufptr = dequeued[i];
      // add packet info to hash table
      addPacket(config->table,bufptr);
      packet_length = wire_packet_length;
      config->stats->bytes += packet_length;

      //Free buffer
      rte_pktmbuf_free(bufptr);
    }
  }
  RTE_LOG(INFO, DPDKCAP, "count core %d\n", rte_lcore_id());
  return retval;
}

