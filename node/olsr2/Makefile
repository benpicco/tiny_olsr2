MODULE:= $(shell basename $(CURDIR))
INCLUDES = -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/core/include -I$(RIOTBASE)/drivers/include -I$(RIOTBASE)/drivers/cc110x_ng/include \
	-I$(RIOTBASE)/sys/net/include \
	-I$(RIOTBOARD)/$(BOARD)/include -I$(RIOTCPU)/$(CPU)/include \
	-I./include -I$(OONFBASE)/src-api

include $(RIOTBASE)/Makefile.base
