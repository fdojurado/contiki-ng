CONTIKI_PROJECT = sdn-node
all: $(CONTIKI_PROJECT)

CONTIKI = ../../../

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

MAKE_WITH_SECURITY ?= 0

PLATFORMS_EXCLUDE = nrf52dk

MODULES += $(CONTIKI_NG_SERVICES_DIR)/sdn-energy

# Here, we want to select whether we want this node to be controller or not.
WITH_SDN_CONTROLLER=0

ifeq ($(MAKE_WITH_SECURITY),1)
CFLAGS += -DWITH_SECURITY=1
endif

#use this to enable TSCH: MAKE_MAC = MAKE_MAC_TSCH
MAKE_MAC = MAKE_MAC_TSCH
MAKE_NET = MAKE_NET_SDN
MAKE_ROUTING = MAKE_ROUTING_SDN
include $(CONTIKI)/Makefile.include
