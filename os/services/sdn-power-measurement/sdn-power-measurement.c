#include "contiki.h"
#include "sys/energest.h"
#include "sdn-power-measurement.h"
#include <stdio.h>
#include <limits.h>

/* Log configuration */
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "SDN power"
#define LOG_LEVEL LOG_LEVEL_INFO

/* EWMA (exponential moving average) used to maintain statistics over time */
#define EWMA_SCALE 100
#define EWMA_ALPHA 40

static uint64_t last_tx, last_rx, last_time, last_cpu, last_lpm, last_deep_lpm;

uint64_t moving_avg_power;

PROCESS(sdn_power_measurement_process, "SDN power measurement module");

/*---------------------------------------------------------------------------*/
static uint64_t
to_permil(uint64_t delta_metric, uint64_t delta_time)
{
  return (1000ul * delta_metric) / delta_time;
}
/*---------------------------------------------------------------------------*/
static void
log_energest(const char *name, uint64_t delta, uint64_t delta_time)
{
  LOG_INFO("%-12s: %10" PRIu64 "/%10" PRIu64 " (%" PRIu64 " permil)\n",
           name, delta, delta_time, to_permil(delta, delta_time));
}
/*---------------------------------------------------------------------------*/
static uint64_t
calc_power(uint64_t cpu, uint64_t lpm, uint64_t dlpm, uint64_t transmit, uint64_t listen,
           uint64_t stime)
{
  return 1000 * SDN_POWER_MEASUREMENT_VOLTAGE *
         (cpu * SDN_POWER_MEASUREMENT_CPU +
          lpm * SDN_POWER_MEASUREMENT_LPM +
          dlpm * SDN_POWER_MEASUREMENT_DLPM +
          transmit * SDN_POWER_MEASUREMENT_TX +
          listen * SDN_POWER_MEASUREMENT_RX) /
         stime;
}
/*---------------------------------------------------------------------------*/
static void
simple_energest_step(void)
{
  static unsigned count = 0;
  uint64_t curr_tx, curr_rx, curr_time, curr_cpu, curr_lpm, curr_deep_lpm;
  uint64_t delta_time;
  uint64_t power;

  energest_flush();

  curr_time = ENERGEST_GET_TOTAL_TIME();
  curr_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  curr_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  curr_deep_lpm = energest_type_time(ENERGEST_TYPE_DEEP_LPM);
  curr_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  curr_rx = energest_type_time(ENERGEST_TYPE_LISTEN);

  delta_time = MAX(curr_time - last_time, 1);

  /* Calculate the sample average power consumption */
  power = calc_power(curr_cpu - last_cpu, curr_lpm - last_lpm,
                     curr_deep_lpm - last_deep_lpm, curr_tx - last_tx, curr_rx - last_rx, delta_time);

  /* Calculate the moving average */
  moving_avg_power = (moving_avg_power * (EWMA_SCALE - EWMA_ALPHA) +
                      power * EWMA_ALPHA) /
                     EWMA_SCALE;

  LOG_INFO("--- Period summary #%u (%" PRIu64 " seconds)\n",
           count++, delta_time / ENERGEST_SECOND);
  LOG_INFO("Total time  : %10" PRIu64 "\n", delta_time);
  log_energest("CPU", curr_cpu - last_cpu, delta_time);
  log_energest("LPM", curr_lpm - last_lpm, delta_time);
  log_energest("Deep LPM", curr_deep_lpm - last_deep_lpm, delta_time);
  log_energest("Radio Tx", curr_tx - last_tx, delta_time);
  log_energest("Radio Rx", curr_rx - last_rx, delta_time);
  log_energest("Radio total", curr_tx - last_tx + curr_rx - last_rx,
               delta_time);
  log_energest("Power (uW)", power, delta_time);
  log_energest("EMA (uW)", moving_avg_power, delta_time);

  last_time = curr_time;
  last_cpu = curr_cpu;
  last_lpm = curr_lpm;
  last_deep_lpm = curr_deep_lpm;
  last_tx = curr_tx;
  last_rx = curr_rx;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_power_measurement_process, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();

  moving_avg_power = SDN_POWER_MEASUREMENT_INIT_POWER;

  etimer_set(&periodic_timer, SDN_POWER_MEASUREMENT_PERIOD);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    simple_energest_step();
  }
  PROCESS_END();
  //   PROCESS_BEGIN();

  //   static unsigned long last_all_time = 0, last_all_cpu = 0, last_all_lpm = 0,
  //                        last_all_transmit = 0, last_all_listen = 0;

  //   unsigned long cpu, lpm, transmit, listen;
  //   unsigned long all_time, all_cpu, all_lpm, all_transmit, all_listen;

  //   unsigned long avg_power;
  //   unsigned long stime; //, ttime;

  // #if DEBUG
  //   static unsigned int count = 0;
  // #endif

  //   ewma_power = NODE_INIT_ENERGY; // 10000 mJ or 10 J

  //   etimer_set(&periodic_timer, CLOCK_SECOND * 10);
  //   while (1)
  //   {
  //     PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  //     etimer_reset(&periodic_timer);

  //     /* Update all energest times. */
  //     energest_flush();

  //     /* update energy consumed at each state */
  //     /* total time */
  //     all_time = ENERGEST_GET_TOTAL_TIME();
  //     all_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  //     all_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  //     all_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  //     all_listen = energest_type_time(ENERGEST_TYPE_LISTEN);
  //     /* time for this period */
  //     stime = all_time - last_all_time;
  //     cpu = all_cpu - last_all_cpu;
  //     lpm = all_lpm - last_all_lpm;
  //     transmit = all_transmit - last_all_transmit;
  //     listen = all_listen - last_all_listen;
  //     /* sample & total time */
  //     // ttime = all_cpu + all_lpm;
  //     /* save last energy period */
  //     last_all_time = all_time;
  //     last_all_cpu = all_cpu;
  //     last_all_lpm = all_lpm;
  //     last_all_transmit = all_transmit;
  //     last_all_listen = all_listen;
  //     /* average power consumption */
  //     avg_power = (NODE_VOLTAGE *
  //                  (cpu * DEC2FIX(1L, 800L) +
  //                   lpm * DEC2FIX(0L, 545L) +
  //                   transmit * DEC2FIX(17L, 400L) +
  //                   listen * DEC2FIX(20L, 0))) /
  //                 ((64L * stime) / 1000);
  //     /* energy consume in sample */
  //     // sample_energy = avg_power * to_seconds(stime);
  //     /* total energy consumed */
  //     // convert to mJ
  //     // sample_energy = sample_energy * 1L / 1000L;
  //     /* Compute EWMA */
  //     ewma_power = ((int32_t)ewma_power * (EWMA_SCALE - EWMA_ALPHA) +
  //                   ((int32_t)avg_power * EWMA_ALPHA)) /
  //                  EWMA_SCALE;

  //     PRINTF("\nEnergest:\n");
  //     PRINTF("--- Period summary #%u (%lu seconds)\n", count++, stime / ENERGEST_SECOND);
  //     PRINTF("Total time  : %10lu\n", stime);
  //     PRINTF("CPU         : %10lu/%10lu (%lu permil)\n", cpu, stime, to_permil(cpu, stime));
  //     PRINTF("LPM         : %10lu/%10lu (%lu permil)\n", lpm, stime, to_permil(lpm, stime));
  //     PRINTF("Radio Tx    : %10lu/%10lu (%lu permil)\n", transmit, stime, to_permil(transmit, stime));
  //     PRINTF("Radio Rx    : %10lu/%10lu (%lu permil)\n", listen, stime, to_permil(listen, stime));
  //     PRINTF("Radio total : %10lu/%10lu (%lu permil)\n", transmit + listen, stime, to_permil(transmit + listen, stime));
  //     PRINTF("Power (uW): %lu\n", avg_power);
  //     PRINTF("Moving average (uW): %lu\n", ewma_power);
  //     // PRINTF("2, %ld, %lu, %lu, , , , , , , , ,\n",
  //     //        energy,
  //     //        sample_energy,
  //     //        avg_power);
  //   }
  //   PROCESS_END();
}

/*---------------------------------------------------------------------------*/
void sdn_power_measurement_init(void)
{
  energest_flush();
  last_time = ENERGEST_GET_TOTAL_TIME();
  last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  last_deep_lpm = energest_type_time(ENERGEST_TYPE_DEEP_LPM);
  last_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  last_rx = energest_type_time(ENERGEST_TYPE_LISTEN);
  process_start(&sdn_power_measurement_process, NULL);
}

/** @} */