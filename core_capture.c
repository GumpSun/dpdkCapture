#include <stdbool.h>
#include <signal.h>
#include <string.h>

#include <dpdk/rte_log.h>
#include <dpdk/rte_lcore.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_ether.h>
#include <dpdk/rte_ethdev.h>
#include <dpdk/rte_ip.h>
#include <arpa/inet.h>

#include "core_capture.h"

#define RTE_LOGTYPE_DPDKCAP RTE_LOGTYPE_USER1

// 从给定的port/queue tuple捕获流量
int capture_core(const struct core_capture_config * config) {
  struct rte_mbuf *bufs[DPDKCAP_CAPTURE_BURST_SIZE];
  uint16_t nb_rx;
  int nb_rx_enqueued;

  RTE_LOG(INFO, DPDKCAP, "Core %u is capturing packets for port %u\n",
      rte_lcore_id(), config->port);

  /* Init stats */
  *(config->stats) = (struct core_capture_stats) {
    .core_id=rte_lcore_id(),
    .packets = 0,
    .missed_packets = 0,
  };

  /* Run until the application is quit or killed. */
  for (;;) {
    /* Stop condition */
    if (unlikely(*(config->stop_condition))) {
      break;
    }

    /* Retrieve packets and put them into the ring */
    nb_rx = rte_eth_rx_burst(config->port, config->queue,
        bufs, DPDKCAP_CAPTURE_BURST_SIZE);
    if (likely(nb_rx > 0)) {
      nb_rx_enqueued = rte_ring_enqueue_burst(config->ring, (void*) bufs,
          nb_rx);

      /* Update stats */
      config->stats->packets+=nb_rx_enqueued;
      config->stats->missed_packets+=nb_rx-nb_rx_enqueued;

      /* Free whatever we can't put in the write ring */
      for (; nb_rx_enqueued < nb_rx; nb_rx_enqueued++) {
        rte_pktmbuf_free(bufs[nb_rx_enqueued]);
      }
    }
  }

  RTE_LOG(INFO, DPDKCAP, "Closed capture core %d (port %d)\n",
      rte_lcore_id(), config->port);

  return 0;
}
