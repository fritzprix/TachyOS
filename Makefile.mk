

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
CONFIG_PY:= $(ROOT_DIR)/tools/kconfigpy/jconfigpy.py

OBJS_DIR_DEBUG:= $(ROOT_DIR)/DEBUG
OBJS_DIR_RELEASE:= $(ROOT_DIR)/RELEASE

KCONFIG_ENTRY:= $(ROOT_DIR)/config.json
KCONFIG_TARGET:= $(ROOT_DIR)/.config

VPATH= source/kernel/ source/hal/ST_Micro/STM32F40_41xxx

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

DEBUG_TARGET=TachyOS_dbg_$(MAJOR).$(MINOR).elf
RELEASE_TARGET=TachyOS_rel_$(MAJOR).$(MINOR).elf

PHONY=config all debug release clean

.SILENT : $(KCONFIG_TARGET)

all : debug

config : $(KCONFIG_TARGET)

$(KCONFIG_TARGET) :
	$(PY) $(CONFIG_PY) -c -i $(KCONFIG_ENTRY) -o $(KCONFIG_TARGET)
	
debug: $(OBJS_DIR_DEBUG) $(DEBUG_TARGET)

$(DEBUG_TARGET) : $(OBJ-y) 
	$(CC) $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(LIBS) $(LDFLAG_KERNEL)
	
$(OBJS_DIR_DEBUG): 
	@echo $(CFLAG_COMMON)
	$(MKDIR) $@
	
%.ko:%.c 
	$(CC) $< $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(LIBS) $(LDFLAG_KERNEL) -o $@

%.sko:%.S
	$(CC) $< -c $(CFLAG_DEBUG) $(CFLAG_KERNEL) $(LIBS) $(LDFLAG_KERNEL) $(ASFLAG_KERNEL) -o $@


clean:
	rm -rf $(OBJ-y) $(DEBUG_TARGET) $(RELEASE_TARGET) $(KCONFIG_TARGET) 
 
.PHONY = $(PHONY)
