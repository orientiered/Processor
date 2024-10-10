#WIN for windows, LINUX for linux
SYSTEM = LINUX
BUILD = DEBUG

ifeq ($(origin CC),default)
	CC=g++
endif

all:spu
.PHONY: spu
spu:
	$(MAKE) -f MakefileSPU CC=$(CC) SYSTEM=$(SYSTEM) BUILD=$(BUILD)

clean:
	$(MAKE) -f MakefileSPU clean
