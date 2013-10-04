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
export BOARD = msba2

# this has to be the absolute path of the RIOT-base dir
export RIOTBASE = $(CURDIR)/../../riot/RIOT
export RIOTBOARD= $(RIOTBASE)/../boards
export RIOTLIBS = $(RIOTBASE)/../riot-libs
export OONFBASE = $(CURDIR)/../../oonf_api

EXTERNAL_MODULES +=$(RIOTLIBS)
EXTERNAL_MODULES +=$(OONFBASE)
export EXTERNAL_MODULES

export CFLAGS = -DRIOT

## Modules to include. 

USEMODULE += auto_init
USEMODULE += vtimer
USEMODULE += cc110x_ng
USEMODULE += config
USEMODULE += net_help
USEMODULE += sixlowpan
USEMODULE += destiny
USEMODULE += uart0
USEMODULE += posix
USEMODULE += ltc4150
USEMODULE += oonf_common
USEMODULE += oonf_rfc5444
USEMODULE += compat_misc

export INCLUDES += -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/sys/net -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/drivers/cc110x_ng/include -I$(OONFBASE)/src-api -I$(RIOTLIBS)

include $(RIOTBASE)/Makefile.include
