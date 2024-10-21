BUILD = DEBUG

ifeq ($(origin CC),default)
	CC=g++
endif

all:spu asm
.PHONY: spu
spu:
	cd spu && $(MAKE) CC=$(CC) BUILD=$(BUILD)
.PHONY: asm
asm:
	cd asm && $(MAKE) CC=$(CC) BUILD=$(BUILD)
clean:
	cd spu && $(MAKE) clean
	cd asm && $(MAKE) clean
