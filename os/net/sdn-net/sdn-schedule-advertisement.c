#include "net/sdn-net/sdn-schedule-advertisement.h"
#include "net/sdn-net/sd-wsn.h"
#include "net/sdn-net/sdn-neighbor-discovery.h"
#include "net/sdn-net/sdn-data-packets.h"

#if BUILD_WITH_SDN_ORCHESTRA
#include "net/mac/tsch/tsch.h"
#include "services/orchestra-sdn-centralised/orchestra.h"
#endif /* BUILD_WITH_SDN_ORCHESTRA */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "SA"
#define LOG_LEVEL LOG_LEVEL_INFO

static uint16_t sequence_number = 0;

/*---------------------------------------------------------------------------*/
int sdn_sa_input(void)
{
    uint16_t seq = sdnip_htons(SDN_SA_BUF->seq);
    LOG_INFO("Received (SEQ:%u)\n", seq);
    // We first check whether we have seen this packet before.
    if (seq < sequence_number)
    {
        LOG_WARN("pkt already processed, dropping\n");
        return 0;
    }
    // Update sequence number received
    sequence_number = seq + 1;
    // Set the slotframe size
    uint16_t sf_len = sdnip_htons(SDN_SA_BUF->sf_len);
#if BUILD_WITH_SDN_ORCHESTRA
    if (sf_len != 0)
        NETSTACK_CONF_SDN_SLOTFRAME_SIZE_CALLBACK(sf_len, sequence_number);
#endif /* BUILD_WITH_SDN_ORCHESTRA */
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
            // PRINTF("Type: %u, chan: %u, time: %u, scr: %d.%d, dst= %d.%d\n",
            //        type, channel_offset, time_offset, scr.u8[0], scr.u8[1], dst.u8[0], dst.u8[1]);
#if BUILD_WITH_SDN_ORCHESTRA
            NETSTACK_CONF_SDN_SA_LINK_CALLBACK(type, channel_offset, time_offset, &dst);
#endif /* BUILD_WITH_SDN_ORCHESTRA */
        }
    }
    /* Look in the nbr TSCH queue with packets and update ts and ch */
    tsch_queue_reset();
    /* Reset the data packets sequence number */
#if !(SDN_CONTROLLER || BUILD_WITH_SDN_CONTROLLER_SERIAL)
    sdn_data_reset_seq();
#endif
    /* If we are the hop limit, we do not forward the packet */
    if (SDN_SA_BUF->hop_limit <= my_rank.rank)
    {
        // Dont forward the packet, hop limit reached.
        LOG_WARN("Hop limit reached (SEQ:%u).\n", seq);
        return 0;
    }
    return 1;
}
/*---------------------------------------------------------------------------*/
