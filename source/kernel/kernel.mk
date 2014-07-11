include $(CURDIR)/COMMON.mk

KERNEL_SRCS=\
	    tch_ipc.c\
	    tch_mpool.c\
	    tch_mtx.c\
	    tch_sched.c\
	    tch_sig.c\
	    tch_sys.c\
	    tch_thread.c\
	    tch_vtimer.c

KERNEL_OBJS=$(KERNEL_SRCS:%.c=$(GEN_DIR)/kernel/%.o)
           
OBJS += $(KERNEL_OBJS)

#ROOT_DIR= $(CURDIR)
#KERNEL_SRC_DIR=$(ROOT_DIR)/source/kernel
#PORT_SRC_DIR=$(ROOT_DIR)/source/port/$(ARCH)/$(CPU)
#HAL_SRC_DIR=$(ROOT_DIR)/source/hal/$(HW_PLF)

#KERNEL_HDR_DIR=$(ROOT_DIR)/include/kernel
#PORT_HDR_DIR=$(ROOT_DIR)/include/port/$(ARCH)/$(CPU)
#PORT_COMMON_HDR_DIR=$(ROOT_DIR)/include/port/$(ARCH)
#HAL_VND_HDR_DIR=$(ROOT_DIR)/include/hal/$(HW_PLF)
#HAL_COMMON_HDR_DIR=$(ROOT_DIR)/include/hal

#include "kernel/tch_kernel.h"
#include "hal/tch_hal.h"
#include "port/tch_portcfg.h"
#include "port/ARM/acm4f/tch_port.h"
#include "port/ARM/acm4f/core_cm4.h"
#include "lib/tch_lib.h"

$(GEN_DIR)/kernel/%.o:$(KERNEL_SRC_DIR)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compilere'
	$(CC) $< -c $(CFLAG) $(INC) -o $@
	@echo 'Finished building: $<'
	@echo ' '