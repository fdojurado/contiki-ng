#include "sdn-energy.h"
#include "sys/energest.h"

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* EWMA (exponential moving average) used to maintain statistics over time */
#define EWMA_SCALE 100
#define EWMA_ALPHA 40

#define DEC2FIX(h, d) ((h * 64L) + (unsigned long)((d * 64L) / 1000L))

unsigned long energy;

PROCESS(sdn_energy, "SDN energy module");

/*---------------------------------------------------------------------------*/
static unsigned long
to_permil(unsigned long delta_metric, unsigned long delta_time)
{
  return (1000ul * (delta_metric)) / delta_time;
}
/*---------------------------------------------------------------------------*/
static unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / RTIMER_ARCH_SECOND);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_energy, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();

  static unsigned long last_all_time = 0, last_all_cpu = 0, last_all_lpm = 0,
                       last_all_transmit = 0, last_all_listen = 0;

  unsigned long cpu, lpm, transmit, listen;
  unsigned long all_time, all_cpu, all_lpm, all_transmit, all_listen;

  unsigned long avg_power, sample_energy;
  unsigned long stime; //, ttime;

  static unsigned count = 0;

  energy = NODE_INIT_ENERGY; // 10000 mJ or 10 J

  etimer_set(&periodic_timer, CLOCK_SECOND * 10);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    /* Update all energest times. */
    energest_flush();

    /* update energy consumed at each state */
    /* total time */
    all_time = ENERGEST_GET_TOTAL_TIME();
    all_cpu = energest_type_time(ENERGEST_TYPE_CPU);
    all_lpm = energest_type_time(ENERGEST_TYPE_LPM);
    all_transmit = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    all_listen = energest_type_time(ENERGEST_TYPE_LISTEN);
    /* time for this period */
    stime = all_time - last_all_time;
    cpu = all_cpu - last_all_cpu;
    lpm = all_lpm - last_all_lpm;
    transmit = all_transmit - last_all_transmit;
    listen = all_listen - last_all_listen;
    /* sample & total time */
    // ttime = all_cpu + all_lpm;
    /* save last energy period */
    last_all_time = all_time;
    last_all_cpu = all_cpu;
    last_all_lpm = all_lpm;
    last_all_transmit = all_transmit;
    last_all_listen = all_listen;
    /* average power consumption */
    avg_power = (NODE_VOLTAGE *
                 (cpu * DEC2FIX(1L, 800L) +
                  lpm * DEC2FIX(0L, 545L) +
                  transmit * DEC2FIX(17L, 400L) +
                  listen * DEC2FIX(20L, 0))) /
                ((64L * stime) / 1000);
    /* energy consume in sample */
    sample_energy = avg_power * to_seconds(stime);
    /* total energy consumed */
    // convert to mJ
    // sample_energy = sample_energy * 1L / 1000L;
    /* Compute EWMA */
    energy = ((int32_t)energy * (EWMA_SCALE - EWMA_ALPHA) +
              ((int32_t)sample_energy * EWMA_ALPHA)) /
             EWMA_SCALE;

    // PRINTF("\nEnergest:\n");
    // PRINTF("--- Period summary #%u (%lu seconds)\n", count++, stime / ENERGEST_SECOND);
    // PRINTF("Total time  : %10lu\n", stime);
    // PRINTF("CPU         : %10lu/%10lu (%lu permil)\n", cpu, stime, to_permil(cpu, stime));
    // PRINTF("LPM         : %10lu/%10lu (%lu permil)\n", lpm, stime, to_permil(lpm, stime));
    // // PRINTF("Deep LPM    : %10lu/%10lu (%lu permil)\n", delta_deep_lpm, stime, to_permil(delta_deep_lpm, stime));
    // PRINTF("Radio Tx    : %10lu/%10lu (%lu permil)\n", transmit, stime, to_permil(transmit, stime));
    // PRINTF("Radio Rx    : %10lu/%10lu (%lu permil)\n", listen, stime, to_permil(listen, stime));
    // PRINTF("Radio total : %10lu/%10lu (%lu permil)\n", transmit + listen, stime, to_permil(transmit + listen, stime));
    // PRINTF("Power (uW): %lu\n", avg_power);
    // PRINTF("Energy in sample (uJ): %lu\n", sample_energy);
    // PRINTF("Moving average (uJ): %lu\n", energy);
    PRINTF("2, %ld, %lu, %lu, , , , , , , , ,\n",
           energy,
           sample_energy,
           avg_power);
  }
  PROCESS_END();
}