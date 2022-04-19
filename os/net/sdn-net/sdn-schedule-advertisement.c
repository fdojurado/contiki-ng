#include "net/sdn-net/sdn-schedule-advertisement.h"
#include "net/sdn-net/sd-wsn.h"

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint16_t sequence_number = 0;

/*---------------------------------------------------------------------------*/
int sdn_sa_input(void)
{
    PRINTF("Processing SA pkt.\n");
    // We first check whether we have seen this packet before.
    if (sdnip_htons(SDN_SA_BUF->seq) < sequence_number)
    {
        PRINTF("pkt already processed, dropping\n");
        return 0;
    }
    // Update sequence number received
    sequence_number = sdnip_htons(SDN_SA_BUF->seq) + 1;
    // Process schedules
    uint8_t num_schedules, i, type, channel_offset, time_offset;
    linkaddr_t scr, dst;
    num_schedules = SDN_SA_BUF->payload_len / SDN_SAPL_LEN;
    for (i = 0; i < num_schedules; i++)
    {
        // Only process SA for us
        scr.u16 = sdnip_htons(SDN_SA_PAYLOAD(i)->scr.u16);
        if (linkaddr_cmp(&scr, &linkaddr_node_addr))
        {
            type = SDN_SA_PAYLOAD(i)->type;
            channel_offset = SDN_SA_PAYLOAD(i)->channel_offset;
            time_offset = SDN_SA_PAYLOAD(i)->time_offset;
            dst.u16 = sdnip_htons(SDN_SA_PAYLOAD(i)->dst.u16);
            PRINTF("Type: %u, chan: %u, time: %u, scr: %d.%d, dst= %d.%d\n",
                   type, channel_offset, time_offset, scr.u8[0], scr.u8[1], dst.u8[0], dst.u8[1]);
        }
    }
    return 1;
}
/*---------------------------------------------------------------------------*/
