CONTIKI_PROJECT = sdn-controller
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# PLATFORMS_EXCLUDE = nrf52dk

MODULES += /os/net/sdn-net/sdn-controller
#PROJECT_SOURCEFILES += sd-wsn.c sdn-net.c sdn.c sdn-routing.c neighbor-discovery.c sdnbuf.c
# Here, we want to select whether we want this node to be controller or not.
WITH_SDN_CONTROLLER=1
ifeq ($(WITH_SDN_CONTROLLER),1)
CFLAGS += -DSDN_CONTROLLER=1
else
CFLAGS += -DSDN_CONTROLLER=0
endif

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC = MAKE_MAC_TSCH
MAKE_NET = MAKE_NET_SDN
MAKE_ROUTING = MAKE_ROUTING_REMAINING_ENERGY
include $(CONTIKI)/Makefile.include
