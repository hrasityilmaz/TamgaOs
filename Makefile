BOARD ?= k64f
DEBUG ?= 0

.PHONY: all flash clean

all:
	$(MAKE) -C kernels/arm all BOARD=$(BOARD) DEBUG=$(DEBUG)

flash:
	$(MAKE) -C kernels/arm flash BOARD=$(BOARD) DEBUG=$(DEBUG)

clean:
	$(MAKE) -C kernels/arm clean BOARD=$(BOARD) DEBUG=$(DEBUG)