#include "redis.h"
#include "hash.h"
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

void initHashTable(struct info** table){
	int i;
	for(i=0;i<HASH_TABLE_SIZE;i++){
		table[i] = NULL;
	}
}

void updateStatus(struct core_redis_config* config){
	struct info **table = config->table;
	int currentTime = time(0);
	struct info* buf[HASH_TABLE_SIZE];
	int count=0,i,rcount =0;
	struct in_addr addr;
	char src[16],dst[16];
	redisReply* reply = NULL;
  	RTE_LOG(INFO, DPDKCAP, "Core %u is update status \n",
      		rte_lcore_id());
	redisContext *conn = redisConnect(redisHost, redisPort);
	for(;;){
		while(currentTime == time(0)){
			usleep(1);
		}
		if(conn->err){
			redisFree(conn);
			conn =  redisConnect(redisHost, redisPort);
		}
		currentTime = time(0);
		for(i=0;i<HASH_TABLE_SIZE;i++){
			if(table[i]==NULL){
				continue;
			}else{
				buf[count]=table[i];
				table[i] = NULL;
				count++;
			}
		}
		for(i=0;i<count;i++){
			struct info *last;
			struct info *p = buf[i];
			/* 同步数据到redis中 */
			while(p){
				addr.s_addr = p->src;
				strncpy(src,inet_ntoa(addr),16);
				addr.s_addr = p->dst;
				strncpy(dst,inet_ntoa(addr),16);
				if(p->tcp > 0){
					if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-tcp %d",p->time,src,dst,p->tcp))
					rcount++;
				}
				if(p->icmp > 0){
					if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-icmp %d",p->time,src,dst,p->icmp))
						rcount++;
					}
				if(p->udp > 0){
					if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-udp %d",p->time,src,dst,p->udp))
						rcount++;
					}
				if(p->syn > 0){
					if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-syn %d",p->time,src,dst,p->syn))
						rcount++;
					}
				if(p->ack > 0){
					if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-ack %d",p->time,src,dst,p->ack))
						rcount++;
					}
				if(p->http > 0){
					if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-http %d",p->time,src,dst,p->http))
						rcount++;
					}
				if(p->other > 0){
					if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-other %d",p->time,src,dst,p->other))
						rcount++;
					}
				if(REDIS_OK == redisAppendCommand(conn,"hincrby %d %s-%s-count %d",p->time,src,dst,p->count))
						rcount++;
				last = p;
				p = p->next;
				free(last);
			}
		}
		count = 0;
		for(i=0;i<rcount;i++){
                	redisGetReply(conn,(void**)&reply);
                	freeReplyObject(reply);
		}
		rcount=0;
	}
	redisFree(conn);
}
