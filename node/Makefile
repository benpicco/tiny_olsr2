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
export RIOTBASE = $(CURDIR)/../../riot/RIOT
export RIOTBOARD= $(RIOTBASE)/../boards
export RIOTLIBS = $(RIOTBASE)/../riot-libs
export OONFBASE = $(CURDIR)/../../oonf_api

EXTERNAL_MODULES +=$(RIOTLIBS)
EXTERNAL_MODULES +=$(OONFBASE)
export EXTERNAL_MODULES

export CFLAGS = -DRIOT -DENABLE_DEBUG

## Modules to include. 

USEMODULE += auto_init
USEMODULE += vtimer
USEMODULE += config
USEMODULE += net_help
USEMODULE += sixlowpan
USEMODULE += destiny
USEMODULE += uart0
USEMODULE += posix
USEMODULE += ltc4150
USEMODULE += transceiver
USEMODULE += oonf_common
USEMODULE += oonf_rfc5444
USEMODULE += compat_misc
ifeq ($(BOARD),native)
	USEMODULE += nativenet
else ifeq($(BOARD),msba2)
	USEMODULE += gpioint
	USEMODULE += cc110x_ng
endif

export INCLUDES += -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/sys/net/destiny \
		-I$(RIOTBASE)/sys/net/sixlowpan/include -I$(RIOTBASE)/sys/net/net_help/ -I$(OONFBASE)/src-api -I$(RIOTLIBS)
include $(RIOTBASE)/Makefile.include
