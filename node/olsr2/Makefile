MODULE:= $(shell basename $(CURDIR))
INCLUDES = -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/core/include -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/drivers/cc110x_ng/include \
	-I$(RIOTBASE)/sys/net/include \
	-I$(RIOTBASE)/boards/$(BOARD)/include -I$(RIOTBASE)/cpu/$(BOARD)/include \
	-I./include -I$(OONFBASE)/src-api

include $(RIOTBASE)/Makefile.base
