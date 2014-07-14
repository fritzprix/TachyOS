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

int main(void* arg) {
	tch* api = (tch*) arg;
	person* p = new person();
	api->Mem->free(p);
	while(1){
		api->Thread->sleep(1000);
		classroom* clp = new classroom();
		delete clp;
	}
	return 0;
}

