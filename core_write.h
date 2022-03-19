#ifndef DPDKCAP_CORE_WRITE_H
#define DPDKCAP_CORE_WRITE_H

#include <stdbool.h>
#include "hash.h"
#define DPDKCAP_OUTPUT_FILENAME_LENGTH 100
#define DPDKCAP_WRITE_BURST_SIZE 256

/* Writing core configuration */
struct core_write_config {
  struct rte_ring * ring;
  bool volatile * stop_condition;
  struct core_write_stats * stats;
  unsigned long rotate_seconds;
  struct info ** table;
};

/* Statistics structure */
struct core_write_stats {
  int core_id;
  unsigned long packets;
  unsigned long bytes;
  unsigned long compressed_bytes;
};

/* Launches a write task */
int write_core(const struct core_write_config * config);

#endif
