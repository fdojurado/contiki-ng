CONTIKI_PROJECT = sdn-node
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# MODULES += os/services/orchestra-sdn-centralised

MAKE_WITH_SECURITY ?= 0

# force Orchestra from command line
MAKE_WITH_SDN_ORCHESTRA = 1

PLATFORMS_EXCLUDE = nrf52dk

ifdef NODE_ID
CFLAGS += -DNODE_ID=$(NODE_ID)
endif

include $(CONTIKI)/Makefile.dir-variables

MODULES += $(CONTIKI_NG_SERVICES_DIR)/sdn-energy

# Here, we want to select whether we want this node to be controller or not.
WITH_SDN_CONTROLLER=0

ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

ifeq ($(MAKE_WITH_SDN_ORCHESTRA),1)
MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra-sdn-centralised
endif

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC = MAKE_MAC_TSCH
MAKE_NET = MAKE_NET_SDN
MAKE_ROUTING = MAKE_ROUTING_SDN
include $(CONTIKI)/Makefile.include
