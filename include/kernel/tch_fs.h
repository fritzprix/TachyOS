/*
 * tch_fs.h
 *
 *  Created on: Jun 27, 2015
 *      Author: innocentevil
 */

#ifndef TCH_FS_H_
#define TCH_FS_H_

#include "cdsl_rbtree.h"

struct tch_file_operations;
struct tch_file {
	uint32_t					flag;
	uint32_t					status;
	rb_treeNode_t				rbnode;
	void*						handle;				//
	struct tch_file_operations*	ops;
};

struct tch_file_operations {
	int (*open)(struct tch_file* filp);
	size_t (*read)(struct tch_file* filp,char* bp,size_t len);
	size_t (*write)(struct tch_file* filp,const char* bp,size_t len);
	int (*close)(struct tch_file* filp);
	size_t (*seek)(struct tch_file* filp,size_t offset,int whence);
};


#endif /* TCH_FS_H_ */
