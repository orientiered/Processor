#WIN for windows, LINUX for linux
SYSTEM = LINUX
BUILD = DEBUG

ifeq ($(origin CC),default)
	CC=g++
endif

all:spu asm
.PHONY: spu
spu:
	$(MAKE) -f MakefileSPU CC=$(CC) SYSTEM=$(SYSTEM) BUILD=$(BUILD)
.PHONY: asm
asm:
	$(MAKE) -f MakefileASM CC=$(CC) SYSTEM=$(SYSTEM) BUILD=$(BUILD)
clean:
	$(MAKE) -f MakefileSPU clean
	$(MAKE) -f MakefileASM clean
