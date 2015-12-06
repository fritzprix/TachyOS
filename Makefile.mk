

#######################################################
#        Tachyos Root Makefile                        #
#        Author : Doowoong,Lee                        #
#######################################################


-include .config

# setup root directory of source tree
ROOT_DIR := $(CURDIR)

# setup tools for 'make' works
CC=$(CROSS_COMPLIE)gcc
CXX=$(CROSS_COMPILE)g++
PY=python
MKDIR=mkdir
CONFIG_PY=$(ROOT_DIR)/tools/kconfigpy/jconfig.py

OBJS_DIR_DEBUG=$(ROOT_DIR)/debug
OBJS_DIR_RELEASE=$(ROOT_DIR)/release

KCONFIG_ENTRY=$(ROOT_DIR)/config.json
KCONFIG_TARGET=$(ROOT_DIR)/.config

#include makefile from target source directory
include source/arch/$(CONFIG_ARCH_VENDOR)/$(ARCH_NAME)/
include source/hal/$(SOC_VENDOR)/$(SOC_NAME)/

CLFAG_KERNEL=
CLFAG_APP=
CLFAG_MODULE=

LDFLAG_KERNEL=
LDFLAG_APP=
LDFLAG_MODULE=

PHONY=config all debug release clean

.SILENT : $(KCONFIG_TARGET)

config : $(KCONFIG_TARGET)

$(KCONFIG_TARGET) : 
	$(PY) $(CONFIG_PY) -i $(KCONFIG_ENTRY) -o $(KCONFIG_TARGET)
	
	
all : debug

debug: 
