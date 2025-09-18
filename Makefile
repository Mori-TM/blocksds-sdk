# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

# Try to find the SDK version if it wasn't provided
SDK_VERSION	?= $(shell git describe --tags --exact-match --dirty 2>/dev/null)

.PHONY: all clean examples install libs sys templates tests tools

all: libs sys tools

CP		:= cp
INSTALL		:= install
MAKE		:= make
RM		:= rm -rf

examples:
	+$(MAKE) -C examples

# libnds depends on grit and bin2c
libs: tools
	+$(MAKE) -C libs

# The default ARM7 depends on some libraries
sys: libs
	+$(MAKE) -C sys

templates:
	+$(MAKE) -C templates

tests:
	+$(MAKE) -C tests

tools:
	+$(MAKE) -C tools SDK_VERSION=$(SDK_VERSION)

# Default installation path of BlocksDS core components
INSTALLDIR	?= /opt/blocksds/core/
INSTALLDIR_ABS	:= $(abspath $(INSTALLDIR))

install: all
	@echo "  INSTALL $(INSTALLDIR_ABS)"
	@test $(INSTALLDIR_ABS)
	$(RM) $(INSTALLDIR_ABS)
	$(INSTALL) -d $(INSTALLDIR_ABS)
	+$(MAKE) -C libs install INSTALLDIR=$(INSTALLDIR_ABS)/libs
	+$(MAKE) -C sys install INSTALLDIR=$(INSTALLDIR_ABS)/sys
	+$(MAKE) -C tools install INSTALLDIR=$(INSTALLDIR_ABS)/tools
	+$(CP) -r sys/cmake $(INSTALLDIR_ABS)/cmake
	+$(CP) -r licenses $(INSTALLDIR_ABS)
	@if [ `git rev-parse HEAD 2> /dev/null` ]; then \
		git rev-parse HEAD > $(INSTALLDIR)/version.txt ; \
	fi

clean:
	+$(MAKE) -C examples clean
	+$(MAKE) -C libs clean
	+$(MAKE) -C sys clean
	+$(MAKE) -C tests clean
	+$(MAKE) -C templates clean
	+$(MAKE) -C tools clean
