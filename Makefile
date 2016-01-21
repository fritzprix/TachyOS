

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################


-include .config
-include version.mk

# setup root directory of source tree
ROOT_DIR := $(CURDIR)
CONFIG_DIR :=$(ROOT_DIR)/source/arch/$(ARCH)/configs
AUTOGEN_DIR :=$(ROOT_DIR)/include/kernel/autogen

# setup tools for 'make' works
CC:=$(CROSS_COMPILE)gcc
CXX:=$(CROSS_COMPILE)g++
SIZE:=$(CROSS_COMPILE)size
PY:=python
MKDIR:=mkdir
CONFIG_PY:= $(ROOT_DIR)/tools/jconfigpy/jconfigpy.py

OBJS_DIR_DEBUG:= $(ROOT_DIR)/DEBUG
OBJS_DIR_RELEASE:= $(ROOT_DIR)/RELEASE

OBJS_DEBUG=$(OBJ-y:%=$(OBJS_DIR_DEBUG)/%)
OBJS_RELEASE=$(OBJ-y:%=$(OBJS_DIR_RELEASE)/%)

KCONFIG_ENTRY:=$(ROOT_DIR)/config.json
KCONFIG_TARGET:=$(ROOT_DIR)/.config
KCONFIG_AUTOGEN:=$(AUTOGEN_DIR)/autogen.h

# SRC-y and INC-y are source and header directory list for each made from recipe file(recipe.mk)
VPATH= $(SRC-y)
INC=$(INC-y:%=-I%)

# Assembler flag used in kernel source build
ASFLAG_KERNEL:= -nostdinc

CFLAG_KERNEL=$(CFLAG_COMMON)
CFLAG_APP=$(CFLAG_COMMON)
CFLAG_MODULE=$(CFLAG_COMMON)
CFLAG_DEBUG:=-O0 -g3 -D__DBG
CFLAG_RELEASE:=-O2 -g0

LDFLAG_KERNEL=$(LDFLAG_COMMON)
LDFLAG_APP=$(LDFLAG_COMMON)
LDFLAG_MODULE=$(LDFLAG_COMMON)

LDFLAG_KERNEL+=$(DEF:%=-defsym,%)
DEF-y+=$(DEF)
DEFS=$(DEF-y:%=-D%)

DEBUG_OBJS=$(OBJ-y:%=DEBUG/%)
RELEASE_OBJS=$(OBJ-y:%=RELEASE/%)

DEBUG_TARGET=TachyOS_dbg_$(MAJOR).$(MINOR).elf
RELEASE_TARGET=TachyOS_rel_$(MAJOR).$(MINOR).elf


PHONY=config all debug release clean config_clean
.SILENT : $(SILENT)

SILENT=cofig $(OBJ-y:%=DEBUG/%) $(OBJ-y:%=RELEASE/%) $(DEBUG_TARGET) $(RELEASE_TARGET)


all : debug

ifeq ($(DEFCONF),)
config : $(AUTOGEN_DIR)
	$(PY) $(CONFIG_PY) -c -i $(KCONFIG_ENTRY) -o $(KCONFIG_TARGET) -g $(KCONFIG_AUTOGEN)
else
config : $(AUTOGEN_DIR)
	$(PY) $(CONFIG_PY) -s -i $(CONFIG_DIR)/$(DEFCONF) -t $(KCONFIG_ENTRY) -o $(KCONFIG_TARGET) -g $(KCONFIG_AUTOGEN)
endif


debug: $(OBJS_DIR_DEBUG) $(DEBUG_TARGET)


release: $(OBJS_DIR_RELEASE) $(RELEASE_TARGET)


$(DEBUG_TARGET) : $(DEBUG_OBJS)
	@echo 'building elf image.. $@'
	$(CC) $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(DEBUG_OBJS) $(LDFLAG_KERNEL:%=-Wl,%) -o $@
	$(SIZE) $@
	@echo 'build complete!!'

	
$(RELEASE_TARGET) : $(RELEASE_OBJS)
	@echo 'building elf image.. $@'
	$(CC) $(CLFAG_RELEASE) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(RELEASE_OBJS) $(LDFLAG_KERNEL:%=-Wl,%) -o $@
	$(SIZE) $@
	@echo 'build complete!!'

	

$(OBJS_DIR_DEBUG) $(OBJS_DIR_RELEASE) $(AUTOGEN_DIR):
	$(MKDIR) $@
	
DEBUG/%.uo:%.c
	@echo 'compile.. $@'
	$(CC) $< -c $(CFLAG_DEBUG) $(CFLAG_APP) $(INC) $(DEFS) $(LIBS)  $(LDFLAG_APP:%=-Wl,%) -o $@

DEBUG/%.ko:%.c
	@echo 'compile.. $@'
	$(CC) $< -c $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(LDFLAG_KERNEL:%=-Wl,%) -o $@

DEBUG/%.sko:%.S
	@echo 'compile.. $@'
	$(CC) $< -c $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(LDFLAG_KERNEL:%=-Wl,%) $(ASFLAG_KERNEL) -o $@
	
RELEASE/%.uo:%.c
	@echo 'compile.. $@'
	$(CC) $< -c $(CFLAG_RELEASE) $(CFLAG_APP) $(INC) $(DEFS) $(LIBS)  $(LDFLAG_APP:%=-Wl,%) -o $@	
	
RELEASE/%.ko:%.c
	@echo 'compile.. $@'
	$(CC) $< -c $(CFLAG_RELEASE) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(LDFLAG_KERNEL:%=-Wl,%) -o $@
	
RELEASE/%.sko:%.S
	@echo 'compile.. $@'
	$(CC) $< -c $(CFLAG_RELEASE) $(CFLAG_KERNEL) $(INC) $(DEFS) $(LIBS) $(LDFLAG_KERNEL:%=-Wl,%) $(ASFLAG_KERNEL) -o $@
	

clean:
	rm -rf $(OBJ-y) $(DEBUG_TARGET) $(RELEASE_TARGET) $(RELEASE_OBJS) $(DEBUG_OBJS)

config_clean:
	rm -rf $(KCONFIG_TARGET) $(KCONFIG_AUTOGEN)

.PHONY = $(PHONY)


