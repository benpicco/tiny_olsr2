MODULE:= $(shell basename $(CURDIR))
INCLUDES = -I./include -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/core/include -I$(RIOTBASE)/drivers/cc110x_ng/include -I$(RIOTBASE)/sys/net/include \
	   -I$(RIOTBASE)/boards/$(BOARD)/include -I$(RIOTBASE)/cpu/$(BOARD)/include

include $(RIOTBASE)/Makefile.base
