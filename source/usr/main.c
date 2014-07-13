/*
 * main.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 2. 9.
 *      Author: innocentevil
 */

#include "main.h"
#include "tch.h"


typedef struct person{
	enum {male,female}sex;
	int age;
}person;

typedef struct classroom {
	person students[50];
}classroom;

void* main(void* arg) {
	tch* api = (tch*) arg;
	person* p = api->Mem->alloc(sizeof(person));
	api->Mem->free(p);
	while(1){
		api->Thread->sleep(1000);
		person* p = api->Mem->alloc(sizeof(person));
		classroom* class = api->Mem->alloc(sizeof(classroom));
		api->Mem->free(class);
	}
	return 0;
}

