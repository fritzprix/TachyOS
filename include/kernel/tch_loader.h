/*
 * tch_loader.h
 *
 *  Created on: 2015. 7. 18.
 *      Author: innocentevil
 */

#ifndef TCH_LOADER_H_
#define TCH_LOADER_H_

#include "tch_mm.h"
#include "tch_fs.h"
#include "tch_segment.h"

#define PROCTYPE_DYNAMIC				((uint32_t) 0)			// process which is separtely built from kernel
#define PROCTYPE_STATIC					((uint32_t) 1)			// process which is statically linked to kernel binary

#define PROCTYPE_MSK					((uint32_t) 1)

#define HEADER_ROOT_THREAD				((uint32_t) 0)
#define HEADER_CHILD_THREAD				((uint32_t) 2)
#define HEADER_TYPE_MSK					((uint32_t) 2)

struct proc_header {
	uint32_t						flag;
	uint32_t						permission;
	uint32_t						req_stksz;
	uint32_t						req_heapsz;
	void							*entry;
	struct mem_region				*text_region;
	struct mem_region				*bss_region;
	struct mem_region				*data_region;
	char							*argv;			// null separetaed strings
	uint16_t						argv_sz;		// size of argv strings in character
};

extern struct proc_header default_prochdr;
extern struct proc_header* tch_procExecFromFile(struct tch_file* file,const char* argv[]);
extern struct proc_header* tch_procExec(const char* path,const char* argv[]);


#endif /* TCH_LOADER_H_ */
