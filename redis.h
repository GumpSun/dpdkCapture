#include<stdio.h>
#include<hiredis/hiredis.h>
#include"hash.h"
#ifndef FILE_REDIS_H
#define FILE_REDIS_H

#define redisHost "127.0.0.1"
#define redisPort 6379

struct core_redis_config{
	struct info* table[HASH_TABLE_SIZE];
};

void initHashTable(struct info** table);
/*  */
void updateStatus(struct core_redis_config* config);

#endif
