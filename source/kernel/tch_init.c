/*
 * tch_init.c
 *
 *  Created on: 2015. 8. 4.
 *      Author: innocentevil
 */

#include "tch_port.h"

#include "kernel/tch_boot.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_err.h"


extern int _sisrv;				// isr vector base address from linker script


__attribute__((naked)) void __init(void* arg){

	tch_port_setIsrVectorMap((uint32_t) &_sisrv);
	if(arg && (((bootp) arg)->boot_magic == BOOT_MAGIC)){
		/** has boot loader
		 * assumed state
		 * 1. memory peripheral initialized
		 * 2. clock configured properly
		 */
		tch_port_setKerenlSP((uword_t) tch_kernelMemInit(((bootp) arg)->section_tbl));
	}else{
		// has no boot loader
		//
#if CONFIG_USE_EXTMEM
		tch_boot_setSystemMem();
#endif
		tch_boot_setSystemClock();
		tch_port_setKerenlSP((uword_t) tch_kernelMemInit((struct section_descriptor**) default_sections));
	}

	tch_kernelInit(arg);
}

