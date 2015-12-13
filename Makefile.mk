

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################


-include .config
-include version.mk

# setup root directory of source tree
ROOT_DIR := $(CURDIR)

# setup tools for 'make' works
CC:=$(CROSS_COMPLIE)gcc
CXX:=$(CROSS_COMPILE)g++
PY:=python
MKDIR:=mkdir
CONFIG_PY:= $(ROOT_DIR)/tools/kconfigpy/jconfigpy.py

OBJS_DIR_DEBUG:= $(ROOT_DIR)/debug
OBJS_DIR_RELEASE:= $(ROOT_DIR)/release

KCONFIG_ENTRY:= $(ROOT_DIR)/config.json
KCONFIG_TARGET:= $(ROOT_DIR)/.config

VPATH= source/kernel/

CLFAG_KERNEL=$(CFLAG_COMMON)
CLFAG_APP:=$(CFLAG_COMMON)
CLFAG_MODULE=$(CFLAG_COMMON)
CFLAG_DEBUG:=-O0 -g3
CFLAG_RELEASE:=-O2 -g0

LDFLAG_KERNEL=$(LDFLAG_COMMON)
LDFLAG_APP=$(LDFLAG_COMMON)
LDFLAG_MODULE=$(LDFLAG_COMMON)

DEBUG_TARGET=TachyOS_dbg_$(MAJOR).$(MINOR).elf
RELEASE_TARGET=TachyOS_rel_$(MAJOR).$(MINOR).elf

PHONY=config all debug release clean

.SILENT : $(KCONFIG_TARGET)

all : debug

config : $(KCONFIG_TARGET)

$(KCONFIG_TARGET) :
	$(PY) $(CONFIG_PY) -c -i $(KCONFIG_ENTRY) -o $(KCONFIG_TARGET)
	
debug: $(OBJS_DIR_DEBUG) $(DEBUG_TARGET)

$(DEBUG_TARGET) : 
	$(CC) $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(LIBS) $(LDFLAG_KERNEL) 
 
.PHONY = $(PHONY)
