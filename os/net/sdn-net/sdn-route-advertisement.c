#include "net/sdn-net/sdn-route-advertisement.h"
#include "net/sdn-net/sd-wsn.h"
#include "net/sdn-net/sdn-neighbor-discovery.h"
#include "sdn-ds-route.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RA"
#if LOG_CONF_LEVEL_RA
#define LOG_LEVEL LOG_CONF_LEVEL_RA
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif /* LOG_CONF_LEVEL_RA */

static uint16_t sequence_number = 0;

/*---------------------------------------------------------------------------*/
int sdn_ra_input(void)
{
    uint16_t seq = sdnip_htons(SDN_RA_BUF->seq);
    LOG_INFO("Received (SEQ:%u)\n", seq);
    // We first check whether we have seen this packet before.
    if (seq < sequence_number)
    {
        LOG_WARN("pkt already processed, dropping\n");
        return 0;
    }
    // Update sequence number received
    sequence_number = seq + 1;
    // Process schedules
    uint8_t num_routes, i;
    linkaddr_t scr, dst, via;
    num_routes = SDN_RA_BUF->payload_len / SDN_RAPL_LEN;
    for (i = 0; i < num_routes; i++)
    {
        // Only process RA for us
        scr.u16 = sdnip_htons(SDN_RA_PAYLOAD(i)->scr.u16);
        if (linkaddr_cmp(&scr, &linkaddr_node_addr))
        {
            dst.u16 = sdnip_htons(SDN_RA_PAYLOAD(i)->dest.u16);
            via.u16 = sdnip_htons(SDN_RA_PAYLOAD(i)->via.u16);
            // PRINTF("scr: %d.%d, dst: %d.%d, via: %d.%d\n",
            //        scr.u8[0], scr.u8[1], dst.u8[0], dst.u8[1], via.u8[0], via.u8[1]);
            sdn_ds_route_add(&dst, 0, &via, CONTROLLER);
        }
    }
    /* If we are the hop limit, we do not forward the packet */
    if (SDN_RA_BUF->hop_limit <= my_rank.rank)
    {
        // Dont forward the packet, hop limit reached.
        LOG_WARN("Hop limit reached (SEQ:%u).\n", seq);
        return 0;
    }
    return 1;
}
/*---------------------------------------------------------------------------*/
