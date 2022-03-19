
#ifndef HASH_H
#define HASH_H
#include <dpdk/rte_mbuf.h>
#include <stdint.h>
#define HASH_TABLE_SIZE 65535
struct info{
	int time;
	int icmp;
	int tcp;
	int udp;
	int syn;
	int ack;
	int http;
	int other;
	long count;
	uint32_t src;
	uint32_t dst;
	struct info *next;
};

/* 获取hash table 下标 */
int getIndex(uint32_t src,uint32_t dst);
/* 增加数据包 */
int addPacket(struct info **table,struct rte_mbuf *pkt);
/* 初始化 hash table */
void initTable(struct info **table);


#endif
