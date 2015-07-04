/*
 * tch_err.h
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#ifndef TCH_ERR_H_
#define TCH_ERR_H_


#define KERNEL_PANIC(filename,msg)	tch_kernel_panic(filename,__LINE__,msg)

typedef struct tch_errorDescriptor {
	struct tm	   	when;
	int            	no;
	char*			msg;
	tch_threadId   	who;
}tch_errorDescriptor;

extern void tch_kernel_raise_error(tch_threadId who,int errno,const char* msg);			//managable error which is cuased by thread level erroneous behavior
extern void tch_kernel_panic(const char* floc,int errorno, const char* msg);								//error which can not be handled by kernel (typically caused by kernel bug or boot environments)


#endif /* TCH_ERR_H_ */
