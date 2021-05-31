#include "sdnbuf.h"
#include <string.h>
/*---------------------------------------------------------------------------*/

static uint16_t sdnbuf_attrs[SDNBUF_ATTR_MAX];
static uint16_t sdnbuf_default_attrs[SDNBUF_ATTR_MAX];

/*---------------------------------------------------------------------------*/
void sdnbuf_clear(void)
{
  sdn_len = 0;
  sdnbuf_clear_attr();
}
/*---------------------------------------------------------------------------*/
bool sdnbuf_set_len(uint16_t len)
{
  if (len <= SDN_LINK_MTU)
  {
    sdn_len = len;
    return true;
  }
  else
  {
    return false;
  }
}
/*---------------------------------------------------------------------------*/
uint8_t *sdnbuf_get_next_header(uint8_t *buffer, uint16_t size, uint8_t *protocol)
{
  int curr_hdr_len = 0;
  int next_hdr_len = 0;
  uint8_t *next_header = NULL;
  struct sdn_ip_hdr *sdnbuf = NULL;

  /* protocol in the IP buffer */
  sdnbuf = (struct sdn_ip_hdr *)buffer;
  *protocol = sdnbuf->proto;
  curr_hdr_len = SDN_IPH_LEN;

  /* Check first if enough space for current header */
  if (curr_hdr_len > size)
  {
    return NULL;
  }

  if (*protocol == SDN_PROTO_ND)
  {
    next_hdr_len = SDN_NDH_LEN;
  }
  else if (*protocol == SDN_PROTO_CP)
  {
    next_hdr_len = SDN_CPH_LEN;
  }
  else if (*protocol == SDN_PROTO_DATA)
  {
    next_hdr_len = SDN_DATA_LEN;
  }

  next_header = buffer + curr_hdr_len;

  /* Size must be enough to hold both the current and next header */
  if (next_hdr_len == 0 || curr_hdr_len + next_hdr_len > size)
  {
    return NULL;
  }
  return next_header;
}
/*---------------------------------------------------------------------------*/
uint8_t *cpbuf_get_next_header(uint8_t *buffer, uint16_t size, uint8_t *protocol)
{
  int curr_hdr_len = 0;
  int next_hdr_len = 0;
  uint8_t *next_header = NULL;
  struct sdn_cp_hdr *cpbuf = NULL;

  /* protocol in the IP buffer */
  cpbuf = (struct sdn_cp_hdr *)buffer;
  *protocol = cpbuf->type;
  curr_hdr_len = SDN_CPH_LEN;

  /* Check first if enough space for current header */
  if (curr_hdr_len > size)
  {
    return NULL;
  }

  if ((*protocol == SDN_PROTO_NA) || (*protocol == SDN_PROTO_NC) || (*protocol == SDN_PROTO_NC_ACK))
  {
    next_hdr_len = cpbuf->len;
  }

  next_header = buffer + curr_hdr_len;

  /* Size must be enough to hold both the current and next header */
  if (next_hdr_len == 0 || curr_hdr_len + next_hdr_len > size)
  {
    return NULL;
  }
  return next_header;
}
/*---------------------------------------------------------------------------*/
void sdnbuf_set_len_field(struct sdn_ip_hdr *hdr, uint16_t len)
{
  hdr->len = len;
  //hdr->len[0] = (len >> 8);
  //hdr->len[1] = (len & 0xff);
}
/*---------------------------------------------------------------------------*/
uint8_t
sdnbuf_get_len_field(struct sdn_ip_hdr *hdr)
{
  return hdr->len;
  // return ((uint16_t)(hdr->len[0]) << 8) + hdr->len[1];
}
/*---------------------------------------------------------------------------*/
uint8_t cpbuf_get_len_field(struct sdn_cp_hdr *hdr)
{
  return hdr->len;
  // return ((uint16_t)(hdr->len[0]) << 8) + hdr->len[1];
}
/*---------------------------------------------------------------------------*/
uint16_t sdnbuf_get_attr(uint8_t type)
{
  if (type < SDNBUF_ATTR_MAX)
  {
    return sdnbuf_attrs[type];
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int sdnbuf_set_attr(uint8_t type, uint16_t value)
{
  if (type < SDNBUF_ATTR_MAX)
  {
    sdnbuf_attrs[type] = value;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int sdnbuf_set_default_attr(uint8_t type, uint16_t value)
{
  if (type < SDNBUF_ATTR_MAX)
  {
    sdnbuf_default_attrs[type] = value;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void sdnbuf_clear_attr(void)
{
  /* set everything to "defaults" */
  memcpy(sdnbuf_attrs, sdnbuf_default_attrs, sizeof(sdnbuf_attrs));
}
/*---------------------------------------------------------------------------*/
void sdnbuf_init(void)
{
  memset(sdnbuf_default_attrs, 0, sizeof(sdnbuf_default_attrs));
  /* And initialize anything that should be initialized */
  sdnbuf_set_default_attr(SDNBUF_ATTR_MAX_MAC_TRANSMISSIONS,
                          SDN_MAX_MAC_TRANSMISSIONS_UNDEFINED);
  /* set the not-set default value - this will cause the MAC layer to
     configure its default */
  sdnbuf_set_default_attr(SDNBUF_ATTR_LLSEC_LEVEL,
                          SDNBUF_ATTR_LLSEC_LEVEL_MAC_DEFAULT);
}

/*---------------------------------------------------------------------------*/
