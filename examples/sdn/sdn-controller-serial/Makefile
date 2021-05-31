CONTIKI_PROJECT = sdn-controller-serial
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# PLATFORMS_EXCLUDE = nrf52dk

# MODULES += core/net/sdn-net/sdn-controller
#PROJECT_SOURCEFILES += sd-wsn.c sdn-net.c sdn.c sdn-routing.c neighbor-discovery.c sdnbuf.c
# Here, we want to select whether we want this node to be controller or not.
# WITH_SDN_CONTROLLER=1
# Serial controller connected
WITH_SERIAL_SDN_CONTROLLER=1
# ifeq ($(WITH_SDN_CONTROLLER),1)
# CFLAGS += -DSDN_CONTROLLER=1
# #PROJECT_SOURCEFILES += kruskal_algorithm.c dfs.c
# else
# CFLAGS += -DSDN_CONTROLLER=0
# endif

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC = MAKE_MAC_TSCH
MAKE_NET = MAKE_NET_SDN
MAKE_ROUTING = MAKE_ROUTING_SDN
include $(CONTIKI)/Makefile.include
