

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################


-include .config
-include version.mk

# setup root directory of source tree
ROOT_DIR := $(CURDIR)

# setup tools for 'make' works
CC:=$(CROSS_COMPILE)gcc
CXX:=$(CROSS_COMPILE)g++
PY:=python
MKDIR:=mkdir
CONFIG_PY:= $(ROOT_DIR)/tools/jconfigpy/jconfigpy.py

OBJS_DIR_DEBUG:= $(ROOT_DIR)/DEBUG
OBJS_DIR_RELEASE:= $(ROOT_DIR)/RELEASE

OBJS_DEBUG=$(OBJ-y:%=$(OBJS_DIR_DEBUG)/%)
OBJS_RELEASE=$(OBJ-y:%=$(OBJS_DIR_RELEASE)/%)

KCONFIG_ENTRY:=$(ROOT_DIR)/config.json
KCONFIG_TARGET:=$(ROOT_DIR)/.config
KCONFIG_AUTOGEN:=$(ROOT_DIR)/include/kernel/autogen/autogen.h

# SRC-y and INC-y are source and header directory list for each made from recipe file(recipe.mk)
VPATH= $(SRC-y)
INC=$(INC-y:%=-I%)

# Assembler flag used in kernel source build
ASFLAG_KERNEL:=-x assembler-with-cpp\
				-nostdinc

CFLAG_KERNEL=$(CFLAG_COMMON)
CFLAG_APP=$(CFLAG_COMMON)
CFLAG_MODULE=$(CFLAG_COMMON)
CFLAG_DEBUG:=-O0 -g3
CFLAG_RELEASE:=-O2 -g0

LDFLAG_KERNEL=$(LDFLAG_COMMON)
LDFLAG_APP=$(LDFLAG_COMMON)
LDFLAG_MODULE=$(LDFLAG_COMMON)

LDFLAG_KERNEL+=$(DEF:%=-defsym,%)
DEF-y+=$(DEF)
DEFS=$(DEF-y:%=-D%)

DEBUG_TARGET=TachyOS_dbg_$(MAJOR).$(MINOR).elf
RELEASE_TARGET=TachyOS_rel_$(MAJOR).$(MINOR).elf

PHONY=config all debug release clean config_clean

.SILENT : $(KCONFIG_TARGET)

all : debug

config :
	$(PY) $(CONFIG_PY) -c -i $(KCONFIG_ENTRY) -o $(KCONFIG_TARGET) -g $(KCONFIG_AUTOGEN)

defconfig:


debug: $(OBJS_DIR_DEBUG) $(DEBUG_TARGET)

$(DEBUG_TARGET) : $(OBJ-y)
	$(CC) $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(LDFLAG_KERNEL:%=-Wl,%)

$(OBJS_DIR_DEBUG):
	@echo $(CFLAG_COMMON)
	$(MKDIR) $@

%.ko:%.c
	$(CC) $< $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(LDFLAG_KERNEL:%=-Wl,%) -c -o $@

%.sko:%.S
	$(CC) $< -c $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(LDFLAG_KERNEL:%=-Wl,%) $(ASFLAG_KERNEL) -o $@


clean:
	rm -rf $(OBJ-y) $(DEBUG_TARGET) $(RELEASE_TARGET)

config_clean:
	rm -rf $(KCONFIG_TARGET) $(KCONFIG_AUTOGEN)

.PHONY = $(PHONY)
