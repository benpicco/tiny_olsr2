ifneq (,$(findstring udp_ping,$(USEMODULE)))
	DIRS += udp_ping
endif
ifneq (,$(findstring olsr2,$(USEMODULE)))
	DIRS += olsr2
endif

all:
	mkdir -p $(BINDIR)
	@for i in $(DIRS) ; do $(MAKE) -C $$i ; done ;

clean:
