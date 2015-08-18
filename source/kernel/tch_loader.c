/*
 * tch_loader.c
 *
 *  Created on: 2015. 7. 18.
 *      Author: innocentevil
 */




#include "tch_loader.h"

struct proc_header default_prochdr = { 0 };


struct proc_header* tch_procLoadFromFile(struct tch_file* filp,const char* argv[]){
	struct proc_header* header = kmalloc(sizeof(struct proc_header));
}

struct proc_header* tch_procExec(const char* path,const char* argv[]){

}
