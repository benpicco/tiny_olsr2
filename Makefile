####
#### Sample Makefile for building apps with the RIOT OS
####
#### The Sample Filesystem Layout is:
#### /this makefile
#### ../../RIOT 
#### ../../boards   for board definitions (if you have one or more)
#### 

# name of your project
export PROJECT =olsr_node

# for easy switching of boards
ifeq ($(strip $(BOARD)),)
	export BOARD = native
endif

# this has to be the absolute path of the RIOT-base dir
export RIOTBASE = $(CURDIR)/../riot/RIOT
export RIOTBOARD= $(RIOTBASE)/../boards
export RIOTLIBS = $(RIOTBASE)/../riot-libs
export OONFBASE = $(CURDIR)/../oonf_api
export OLSR_NODE= $(CURDIR)/node

EXTERNAL_MODULES +=$(RIOTLIBS)
EXTERNAL_MODULES +=$(OONFBASE)
EXTERNAL_MODULES +=$(OLSR_NODE)
export EXTERNAL_MODULES

export CFLAGS = -DRIOT -DENABLE_NAME

## Modules to include. 

USEMODULE += auto_init
USEMODULE += vtimer
USEMODULE += rtc
USEMODULE += net_help
USEMODULE += sixlowpan
USEMODULE += destiny
USEMODULE += uart0
USEMODULE += posix
USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += random
USEMODULE += transceiver
USEMODULE += oonf_common
USEMODULE += oonf_rfc5444
USEMODULE += compat_misc
USEMODULE += olsr2
USEMODULE += udp_ping
ifeq ($(BOARD),native)
	USEMODULE += nativenet
	export CFLAGS += -DBOARD_NATIVE -DINIT_ON_START -ggdb
endif
ifeq ($(BOARD),msba2)
	USEMODULE += gpioint
	USEMODULE += cc110x_ng
	export CFLAGS += -DBOARD_MSBA2 -DINIT_ON_START
endif

export INCLUDES += -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/sys/net/destiny/include -I$(RIOTBASE)/drivers/cc110x_ng/include \
		-I$(RIOTBASE)/sys/net/sixlowpan/include -I$(RIOTBASE)/sys/net/net_help/ -I$(OONFBASE)/src-api -I$(RIOTLIBS) \
		-I$(OLSR_NODE)/udp_ping/include -I$(OLSR_NODE)/olsr2/include

include $(RIOTBASE)/Makefile.include
