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

struct proc_header {
	void* entry;
	struct mem_region* text_region;
	struct mem_region* bss_region;
	struct mem_region* data_region;
};

extern struct proc_header* tch_procLoadFromFile(struct tch_file* filp,const char* argv[]);
extern struct proc_header* tch_procExec(const char* path,const char* argv[]);


#endif /* TCH_LOADER_H_ */
