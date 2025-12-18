PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../../..

TARGET  = tegra-bpmp
SRC_CC  = main.cc \
	  client.cc \
	  client_collection.cc \
	  client_factory.cc \
	  tegra_bpmp.cc
SRC_DATA = bpmp.lua

REQUIRES_LIBS = libio-vbus lua++ libstdc++ libtegra-bpmp

include $(L4DIR)/mk/prog.mk
