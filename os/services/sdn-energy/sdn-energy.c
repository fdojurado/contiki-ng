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

#define DEC2FIX(h, d) ((h * 64L) + (unsigned long)((d * 64L) / 1000L))

signed long energy;

PROCESS(sdn_energy, "SDN energy module");

static unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / RTIMER_ARCH_SECOND);
}

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
    energy = sample_energy * 1L / 1000L;

    // PRINTF("\nEnergest:\n");
    // PRINTF(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
    //        to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
    //        to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
    //        to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
    //        to_seconds(all_time));
    // PRINTF(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
    //        to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
    //        to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
    //        to_seconds(all_time - energest_type_time(ENERGEST_TYPE_TRANSMIT) - energest_type_time(ENERGEST_TYPE_LISTEN)));
    // PRINTF("total time spent in this sample: %4lus\n", to_seconds(stime));
    // PRINTF("total time: %4lus\n", to_seconds(all_time));
    // PRINTF("Total average power consumed in this sample (uW): %lu\n", avg_power);
    // PRINTF("Total energy consumed in sample (uJ): %lu\n", sample_energy);
    PRINTF("2, %ld, %lu, %lu, , , , , , , , ,\n",
           energy,
           sample_energy,
           avg_power);
  }
  PROCESS_END();
}