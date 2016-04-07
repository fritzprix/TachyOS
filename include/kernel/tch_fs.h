/*
 * tch_fs.h
 *
 *  Created on: Jun 27, 2015
 *      Author: innocentevil
 */

#ifndef TCH_FS_H_
#define TCH_FS_H_

#include "tch_ktypes.h"

struct tch_file_operations;
typedef int32_t ssize_t;


struct tch_dentry {

};

typedef struct tch_file {
	void*						data;
	struct tch_file_operations*	ops;
} file;

typedef struct tch_file_operations {
	int (*open)(struct tch_file* filp);
	ssize_t (*read)(struct tch_file* filp,char* bp,size_t len);
	ssize_t (*write)(struct tch_file* filp,const char* bp,size_t len);
	int (*close)(struct tch_file* filp);
	ssize_t (*seek)(struct tch_file* filp,size_t offset,int whence);
} file_operations_t;





#endif /* TCH_FS_H_ */
